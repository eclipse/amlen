/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */
package com.ibm.ima.ra.inbound;

import java.util.Timer;
import java.util.TimerTask;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.Queue;
import javax.jms.Session;
import javax.jms.Topic;
import javax.jms.XAConnection;
import javax.resource.ResourceException;
import javax.resource.spi.endpoint.MessageEndpoint;
import javax.resource.spi.endpoint.MessageEndpointFactory;
import javax.resource.spi.work.ExecutionContext;
import javax.resource.spi.work.Work;
import javax.resource.spi.work.WorkEvent;
import javax.resource.spi.work.WorkException;
import javax.resource.spi.work.WorkManager;

import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaConstants;
import com.ibm.ima.jms.impl.ImaDestination;
import com.ibm.ima.jms.impl.ImaSession;
import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.jms.impl.ImaXASession;
import com.ibm.ima.ra.ImaResourceAdapter;
import com.ibm.ima.ra.common.ImaResourceException;
import com.ibm.ima.ra.common.ImaXARProxy;

public class ImaMessageEndpointNonAsf extends ImaMessageEndpoint {
    private final ImaDestination   destination;
    private final String           subscriptionName;
    private final boolean          durable;
    private final boolean          shared;
    private final boolean          useLocalTransactions;

    private final NonAsfWorkImpl[] works;
    private final String           name;
    
    private ImaConnection xarConn = null; /* Connection used to provide a session for the XA proxy */
    
    // private Long minWaitTime = 10L; /* Min wait time betw scheduled tasks */
    private Long             maxWaitTime = 60000L; /* Max wait time betw scheduled tasks */

    public ImaMessageEndpointNonAsf(ImaResourceAdapter ra, MessageEndpointFactory mef, ImaActivationSpec spec,
            WorkManager wm, Timer timer) throws ResourceException {
        super(ra, mef, spec, wm, timer);
        subscriptionName = spec.getSubscriptionName();
        durable = ImaActivationSpec.DURABLE.equalsIgnoreCase(spec.getSubscriptionDurability());
        shared = ImaActivationSpec.SHARED.equalsIgnoreCase(spec.getSubscriptionShared());
        useLocalTransactions = !useGlobalTransactions && spec.enableRollback();
        destination = spec.getDestinationObject();
        int numOfWorks = 1;
        if (destination instanceof Queue) {
            numOfWorks = spec.concurrentConsumers();
        } else {
            if (shared)
                numOfWorks = spec.concurrentConsumers();
        }
        name = destination.toString();
        works = new NonAsfWorkImpl[numOfWorks];
        
        if (Boolean.parseBoolean(config.getIgnoreFailuresOnStart())) {
            TimerTask tt = new InitAllTimerTask(0);
            tt.run();
            return;
        }
        try {
            init();
        } catch (JMSException e) {
            throw new ImaResourceException("CWLNC2146", e, "Endpoint activation failed due to an exception when initializing work for the specified activation specification {0}.",
                    spec);
        }
    }

    protected synchronized void init() throws JMSException {
        if (isClosed.get())
            return;
        try {
            if (isTraceable(6))
                trace(6, "ImaMessageEndpoint: init start (" + config + ")");
            super.init();
            for (int i = 0; i < works.length; i++) {
                works[i] = new NonAsfWorkImpl(i);
            }
            connection.start();
        } catch (JMSException e) {
            for (int i = 0; i < works.length; i++) {
                works[i] = null;
            }
            if (connection != null) {
                try {
                    connection.close();
                } catch (Exception ex) {
                    // Ignore
                }
                connection = null;
            }
            throw e;
        }
        if (isTraceable(5))
            trace(5, "ImaMessageEndpoint: connection started " + connection + " (" + config + ")");
        for (int i = 0; i < works.length; i++) {
            scheduleWork(works[i], 1);
        }

    }

    public void onException(JMSException jex) {
        traceException(2, jex);
        onError(null, jex);
    }

    synchronized protected void onError(WorkImpl work, Exception jex) {
        trace(4, "ImaMessageEndpoint.onError: work=" + String.valueOf(work) + 
                " isClosed=" + isClosed.get() + " connection=" + String.valueOf(connection) + " (" + config + ")");
        if (isClosed.get())
            return;
        if ((work != null) && (work.workConnection != connection))
            return;
        if (connection == null)
            return;
        if (isConnectionClosed()) {
            connection = null;
            doStop();
            if (isClosed.get()) /* doStop may release the lock so check if this endpoint is closed again. */
                return;
            trace(4, "ImaMessageEndpoint.onError: schedule reconnect (" + config + ")");
            TimerTask tt = new InitAllTimerTask(1);
            timer.schedule(tt, 1);
            return;
        }
        if (work instanceof NonAsfWorkImpl) {
            NonAsfWorkImpl w = (NonAsfWorkImpl) work;
            works[w.index] = null;
            trace(4, "ImaMessageEndpoint.onError: schedule recreate work task idx=" + w.index + " (" + config + ")");
            TimerTask tt = new InitWorkTimerTask(1, w.index);
            timer.schedule(tt, 1);
        }
    }

    private final void scheduleWork(final NonAsfWorkImpl w, final long waitTime) {
        if (isClosed.get() || isPaused.get() || w.isClosed() || (works[w.index] != w))
            return;
        try {
            workManager.scheduleWork(w, Long.MAX_VALUE, new ExecutionContext(), this);
        } catch (WorkException e) {
            traceException(2, e);
            TimerTask tt = new TimerTask() {
                public void run() {
                    long newWaitTime = (waitTime < maxWaitTime) ? waitTime * 2 : waitTime;
                    scheduleWork(w, newWaitTime);
                }
            };
            trace(4, "ImaMessageEndpoint.scheduleWork: failed for idx=" + w.index
                    + ". Going to retry in " + waitTime + "ms. (" + config + ")");
            timer.schedule(tt, waitTime);
        }
    }

    public void workCompleted(WorkEvent we) {
        Work w = we.getWork();
        if (w instanceof NonAsfWorkImpl) {
            scheduleWork((NonAsfWorkImpl) w, 1);
        }
    }

    protected void doStop() {
        if (isTraceable(6))
            trace(6, "ImaMessageEndpoint.doStop: entry (" + config + ")");
        for (int i = 0; i < works.length; i++) {
            if (works[i] != null) {
                works[i].close();
                works[i] = null;
            }
        }
        if (isTraceable(6))
            trace(6, "ImaMessageEndpoint.doStop: exit (" + config + ")");
    }
    
    public final class NonAsfWorkImpl extends WorkImpl {
        private ImaSession            session;
        private final MessageConsumer consumer;
        private ImaXARProxy           xar;
        private final int             index;

        NonAsfWorkImpl(int ind) throws JMSException {
            super();
            if (ImaTrace.isTraceable(5)) {
                ImaTrace.trace(5, "Creating ImaMessageEndpoint " + name + " index=" + ind + " durable=" + durable
                        + " shared=" + shared);
            }
            index = ind;
            if (useGlobalTransactions) {
                session = (ImaSession) ((XAConnection) connection).createXASession();
                xar = new ImaXARProxy(config, (ImaXASession) session, this, config.getTraceLevel());
            } else {
                String ackmode = config.getAcknowledgeMode();
                int    ack = ImaActivationSpec.DUPSOK_ACK.equalsIgnoreCase(ackmode) ? 
                    Session.DUPS_OK_ACKNOWLEDGE : Session.AUTO_ACKNOWLEDGE;
                session = (ImaSession) connection.createSession(useLocalTransactions, ack);
                xar = null;
            }    
            if (isTraceable(5))
                trace(5, "ImaMessageEndpoint: session created: " + session + " (" + config + ")");
            if (!(durable || shared) || (destination instanceof Queue)) {
                consumer = session.createConsumer(destination, config.getMessageSelector());
            } else if (durable) {
                if (!shared)
                    consumer = session.createDurableSubscriber((Topic) destination, subscriptionName, config.getMessageSelector(), false);
                else
                    consumer = ((ImaSession)session).createSharedDurableConsumer((Topic) destination, subscriptionName, config.getMessageSelector());
            } else {
                consumer = ((ImaSession)session).createSharedConsumer((Topic) destination, subscriptionName, config.getMessageSelector());
            }
            if (isTraceable(5))
                trace(5, "ImaMessageEndpoint: consumer created: " + consumer + " (" + config + ")");
        }
        
        /* 
         * (non-Javadoc)
         * @see com.ibm.ima.ra.inbound.ImaMessageEndpoint.WorkImpl#run()
         * 
         * Defect 42505: JBOSS requires the run method to exist in the class
         * that invokes it.
         */
        public void run() {
            super.run();
        }

        /*
         * (non-Javadoc)
         * 
         * @see javax.resource.spi.work.Work#release()
         */
        public void release() {

        }
        
        public ImaSession refreshXaSession() throws JMSException {
            if (useGlobalTransactions) {
            	if ((xarConn == null) || xarConn.getBoolean("isClosed", true))
                	xarConn = adapter.getConnectionPool().getConnection(config);
                return (ImaSession)((XAConnection) xarConn).createXASession();
            } 
            return (ImaSession)xarConn.createSession();
        }

        protected void closeImpl() {
            try {
                ImaTrace.trace(5, "ImaMessageEndpoint closing work: " + this.toString() + " (" + config + ")");
                session.close();
                session = null;
                if (xar != null)
                    xar.close();
                try {
                	if (xarConn != null) {
                		xarConn.close();
                	}
                } catch (Exception ex) {
                	// Ignore
                } finally {
                	xarConn = null;
                }
            } catch (JMSException e) {
                traceException(2, e);
            }
        }

        protected final void doDelivery(MessageEndpoint endpoint) throws JMSException {
            try {
                Message m = consumer.receive(1000);
                if (m == null)
                    return;
                if (isTraceable(9))
                    trace(9, "ImaMessageEndpoint: doDelivery. msg= " + m + " (" + config + ")");
                inOnMessage = true;
                ((MessageListener) endpoint).onMessage(m);
                inOnMessage = false;
                fdHandler.onDeliverySuccess();
            } catch (RuntimeException re) {
                traceException(1, re);
                if (useLocalTransactions)
                    session.rollback();
                throw re;
            }
            if (useLocalTransactions)
                session.commit();
        }

        /*
         * (non-Javadoc)
         * 
         * @see com.ibm.ima.ra.inbound.ImaMessageEndpoint.WorkImpl#getXAResource()
         */
        protected ImaXARProxy getXAResource() {
            return xar;
        }

        public String toString() {
            StringBuffer sb = new StringBuffer("NonAsfWorkImpl{");
            sb.append("index=").append(index).append(" state=").append(state);
            sb.append(' ').append(workConnection.toString(ImaConstants.TS_All));
            sb.append(' ').append(session.toString(ImaConstants.TS_All)).append(' ');
            sb.append(consumer.toString());
            return sb.toString();
        }
    	
    }

    /*
     * Timer task that reinitialize (reconnect and recreate work) tasks and submits them to the workManager.
     */
    private final class InitAllTimerTask extends TimerTask {
        final long waitTime;

        InitAllTimerTask(long waitTime) {
            this.waitTime = waitTime;
        }

        public void run() {
            try {
                if (isTraceable(9))
                    trace(9, "InitAllTimerTask.run(" + name + "): waitTime=" + waitTime);
                init();
            } catch (JMSException e) {
                if (waitTime == 0) {
                    ImaTrace.trace(1, "Ignoring error " + e + " because ignoreFailuresOnStart is set to true");
                }
                long newWaitTime = (waitTime < maxWaitTime) ? ((waitTime + 1) * 3 / 2) : waitTime;
                if (isTraceable(9))
                    trace(9, "InitAllTimerTask.run(" + name + "): reschedule newWaitTime=" + newWaitTime);
                TimerTask tt = new InitAllTimerTask(newWaitTime);
                timer.schedule(tt, newWaitTime);
            }
        }
    }

    private final class InitWorkTimerTask extends TimerTask {
        final long waitTime;
        final int  index;

        InitWorkTimerTask(long waitTime, int idx) {
            this.waitTime = waitTime;
            index = idx;
        }

        public void run() {
            synchronized (ImaMessageEndpointNonAsf.this) {
                try {
                    if (connection == null)
                        return;
                    if (works[index] != null)
                        return;
                    works[index] = new NonAsfWorkImpl(index);
                    scheduleWork(works[index], 1);
                } catch (JMSException e) {
                    if (isConnectionClosed()) {
                        onError(null, e);
                        return;
                    }
                    long newWaitTime = (waitTime < maxWaitTime) ? ((waitTime + 1) * 3 / 2) : waitTime;
                    TimerTask tt = new InitWorkTimerTask(newWaitTime, index);
                    timer.schedule(tt, newWaitTime);
                }
            }
        }
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
