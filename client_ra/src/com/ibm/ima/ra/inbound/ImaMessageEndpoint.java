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

import java.lang.reflect.Method;
import java.util.Timer;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import javax.jms.ExceptionListener;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageListener;
import javax.resource.ResourceException;
import javax.resource.spi.RetryableUnavailableException;
import javax.resource.spi.UnavailableException;
import javax.resource.spi.endpoint.MessageEndpoint;
import javax.resource.spi.endpoint.MessageEndpointFactory;
import javax.resource.spi.work.Work;
import javax.resource.spi.work.WorkEvent;
import javax.resource.spi.work.WorkListener;
import javax.resource.spi.work.WorkManager;

import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.jms.impl.ImaTraceable;
import com.ibm.ima.ra.ImaResourceAdapter;
import com.ibm.ima.ra.common.ImaResourceException;
import com.ibm.ima.ra.common.ImaXARProxy;

public abstract class ImaMessageEndpoint implements ExceptionListener, WorkListener, ImaTraceable {
    private static final Method            onMessageMethod;
    static {
        Class<?>[] params = { Message.class };
        Method method = null;
        try {
            method = MessageListener.class.getMethod("onMessage", params);
        } catch (NoSuchMethodException ex) {
        }
        onMessageMethod = method;
    }
    protected final ImaResourceAdapter     adapter;
    protected final MessageEndpointFactory endpointFactory;
    protected final WorkManager            workManager;
    protected final boolean                useGlobalTransactions;
    protected ImaConnection                connection = null;
    protected final AtomicBoolean          isClosed = new AtomicBoolean(false);
    protected final AtomicBoolean          isPaused = new AtomicBoolean(false);
    protected final ImaActivationSpec      config;
    protected final WorkListener           workListener;
    protected final FailedDeliveryHandler  fdHandler;
    final Timer                            timer;
    

    public ImaMessageEndpoint(ImaResourceAdapter ra, MessageEndpointFactory mef, ImaActivationSpec spec,
            WorkManager wm, Timer timer)
            throws ResourceException {
        config = spec;
        adapter = ra;
        endpointFactory = mef;
        workManager = wm;
        workListener = this;
        fdHandler = FailedDeliveryHandler.createDeliveryHandler(this, spec.maxDeliveryFailures);
        connection = spec.getLoopbackConn();
        this.timer = timer;
        
        if (onMessageMethod == null) {
            ImaResourceException re = ImaResourceException.onMessageNotFound(null);
            if (connection != null)
                connection.traceException(1, re);
            else
                ImaTrace.traceException(1, re);
            throw re;
        }
        try {
            useGlobalTransactions = mef.isDeliveryTransacted(onMessageMethod);
        } catch (NoSuchMethodException e) {
            ImaResourceException re = ImaResourceException.onMessageNotFound(e);
            if (connection != null)
                connection.traceException(1, re);
            else
                ImaTrace.traceException(1, re);
            throw re;
        }
        
    }

    protected void init() throws JMSException {
        if (isClosed.get() || (connection != null))
            return;
        connection = adapter.getConnectionPool().getConnection(config);
        // We need to explicitly set the trace level in case it's different
        // from the value used when the connection in the pool was created.
        int connTraceLevel = Integer.valueOf(config.getTraceLevel());
        if (connTraceLevel == -1)
            connTraceLevel = 4;
        connection.setTraceLevel(connTraceLevel + 10);
        connection.setExceptionListener(this);
        if (isTraceable(5))
            trace(5, "ImaMessageEndpoint: connection created: " + connection.toString());
    }

    public synchronized void stop() {
        if (isClosed.compareAndSet(false, true)) {
            if (isTraceable(5))
                trace(5, "Endpoint " + this + " is going to be stopped.");
            doStop();
            if (connection != null) {
                try {
                    connection.close();
                } catch (JMSException e) {
                    traceException(e);
                }
            }
        }
    }

    boolean pause() {
        return isPaused.compareAndSet(false, true);
    }

    protected abstract void doStop();

    protected abstract void onError(WorkImpl work, Exception jex);

    @Override
    public void workAccepted(WorkEvent arg0) {
        // System.err.println("ImaMessageEndpoint: workAccepted.");
    }

    @Override
    public void workRejected(WorkEvent arg0) {
        // System.err.println("ImaMessageEndpoint: workRejected.");
    }

    @Override
    public void workStarted(WorkEvent arg0) {
        // System.err.println("ImaMessageEndpoint: workStarted.");
    }
    
    /* ******************************************************
     * 
     * Implement trace on the connection object
     *  
     ****************************************************** */
    
    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#isTraceable(int)
     */
    public boolean isTraceable(int level) {
        if (connection != null)          
            return ((ImaConnection)connection).isTraceable(level);
        return false;
    }

    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#trace(int, java.lang.String)
     */
    public void trace(int level, String message) {
        if (connection != null)
            ((ImaConnection)connection).trace(level, message);
        else
            ImaTrace.trace(level, message);
    }

    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#trace(java.lang.String)
     */
    public void trace(String message) {
        if (connection != null)
            ((ImaConnection)connection).trace(message);
        else
            ImaTrace.trace(message);
    }

    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#traceException(java.lang.Throwable)
     */
    public JMSException traceException(Throwable ex) {
        if (connection != null)
            return ((ImaConnection)connection).traceException(ex);
        return ImaTrace.traceException(ex);
    }

    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#traceException(int, java.lang.Throwable)
     */
    public JMSException traceException(int level, Throwable ex) {
        if (connection != null)
            return ((ImaConnection)connection).traceException(level, ex);
        return ImaTrace.traceException(level, ex);
    }

    boolean isConnectionClosed() {
        ImaProperties props = connection;
        if (props == null)
            return true;
        return props.getBoolean("isClosed", false);
    }

    protected abstract class WorkImpl implements Work {
        static final int    SCHEDULED   = 0;
        static final int    RUNNING     = 1;
        static final int    CLOSED      = 2;
        static final int    CLOSE_WAIT  = 3;
        final AtomicInteger state       = new AtomicInteger(SCHEDULED);
        final ImaConnection workConnection;
        protected boolean inOnMessage = false;

        WorkImpl() {
            workConnection = connection;
        }

        protected abstract ImaXARProxy getXAResource();

        boolean isClosed() {
            int currState = state.get();
            return ((currState == CLOSED) || (currState == CLOSE_WAIT));
        }

        public void run() {
            MessageEndpoint endpoint = null;
            Exception deliveryEx = null;
            
            if (isClosed.get() || isPaused.get())
                return;
            if (!state.compareAndSet(SCHEDULED, RUNNING))
                return;
            try {
                if (useGlobalTransactions) {
                	endpoint = endpointFactory.createEndpoint(getXAResource());
                } else {
                    endpoint = endpointFactory.createEndpoint(null);
                }
                doDelivery(endpoint);
            } catch (Exception e) {
                if (inOnMessage && !(e instanceof JMSException)) {
                    inOnMessage = false;
                    if (!isClosed.get()) {
                        fdHandler.onDeliveryFailure(e);
                    }
                } else {
                	deliveryEx = e;
                }
            } finally {
            	if (endpoint != null) {
                	try {
                        endpoint.release();
                	} catch (Exception ex) {
                		trace(1, "Endpoint release failed: " + ex.getClass().getName() + (ex.getMessage() != null ? " - " + ex.getMessage() : ""));
                	}
                }
                if (deliveryEx != null)
                    handleError(deliveryEx);
                if (state.compareAndSet(RUNNING, SCHEDULED))
                    return;
                synchronized (ImaMessageEndpoint.this) {
                    if (state.compareAndSet(CLOSE_WAIT, CLOSED)) {
                        ImaMessageEndpoint.this.notifyAll();
                    }
                }
            }
        }

        /**
         * Domain-specific implementation of message delivery
         * 
         * @param endpoint
         * @throws JMSException
         */
        protected abstract void doDelivery(MessageEndpoint endpoint) throws JMSException;

        void close() {
            int currState = state.getAndSet(CLOSE_WAIT);
            switch (currState) {
            case CLOSED:
                state.set(CLOSED);
                return;
            case CLOSE_WAIT:
                return;
            case SCHEDULED:
                state.set(CLOSED);
                return;
            }
            while (state.get() != CLOSED) {
                try {
                	// TODO: Add trace to print currState value
                    ImaMessageEndpoint.this.wait();
                } catch (InterruptedException e) {
                    traceException(e);
                }
            }
            closeImpl();
        }

        void handleError(Exception e) {
            if (!state.compareAndSet(RUNNING, CLOSED))
                return;
            traceException(1, e);
            if (!(e instanceof UnavailableException)) {
                onError(this, e);
                return;
            }
            if (e instanceof RetryableUnavailableException) {
                try { Thread.sleep(1000); } catch(InterruptedException ex) {}
                synchronized (ImaMessageEndpoint.this) {
                    if (isClosed.get())
                        return;
                    state.set(RUNNING);
                }
                return;
            }
            trace(1, "The endpoint factory is not available.");
        }
        protected abstract void closeImpl();
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
