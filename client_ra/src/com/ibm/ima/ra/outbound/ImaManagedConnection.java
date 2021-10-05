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

package com.ibm.ima.ra.outbound;

import java.io.PrintWriter;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.HashSet;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.jms.ConnectionMetaData;
import javax.jms.ExceptionListener;
import javax.jms.JMSException;
import javax.jms.Session;
import javax.resource.ResourceException;
import javax.resource.spi.ConnectionEvent;
import javax.resource.spi.ConnectionEventListener;
import javax.resource.spi.ConnectionRequestInfo;
import javax.resource.spi.LocalTransaction;
import javax.resource.spi.ManagedConnection;
import javax.resource.spi.ManagedConnectionMetaData;
import javax.security.auth.Subject;
import javax.transaction.xa.XAResource;

import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaJmsExceptionImpl;
import com.ibm.ima.jms.impl.ImaMessageProducer;
import com.ibm.ima.jms.impl.ImaSession;
import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.jms.impl.ImaTraceable;
import com.ibm.ima.jms.impl.ImaXASession;
import com.ibm.ima.ra.common.ImaCommException;
import com.ibm.ima.ra.common.ImaResourceException;
import com.ibm.ima.ra.common.ImaXAObserver;
import com.ibm.ima.ra.common.ImaXARProxy;

final class ImaManagedConnection implements ManagedConnection, ExceptionListener, ImaProxyContainer,
        ImaXAObserver, ImaTraceable {
    static {
        ImaTrace.init(true);
    }
    
    private final ImaManagedConnectionFactory         factory;
    private final Subject                             subject;
    private final ConnectionRequestInfo               requestInfo;
    private ImaConnection                             jmsConnection;
    private final ImaSession[]                        jmsSessions       = new ImaSession[3]; /* 0 - Non transacted session
                                                                                              * 1 - Locally transacted session
                                                                                              * 2 - XA Session */

    private final ImaMessageProducer[]                jmsProducers       = new ImaMessageProducer[3]; /* One producer per physical session */
    private final HashSet<ImaConnectionProxy>         connectionHandles = new HashSet<ImaConnectionProxy>();
    private final ConcurrentLinkedQueue<ConnectionEventListener> eventListeners    = new ConcurrentLinkedQueue<ConnectionEventListener>();
    private final String                              userName;
    private AtomicBoolean                             destroyed         = new AtomicBoolean(false);
    private int                                       transactionActive = 0;
    private String                                    clientID             = null;
    private ConnectionMetaData                        metaData             = null;
    private PrintWriter                               logWriter         = null;
    private volatile ConnectionEvent                  connectionErrorEvent = null;
    private boolean                                   connectionStarted    = false;

    /**
	 * 
	 */
    ImaManagedConnection(ImaManagedConnectionFactory cf, Subject subj, ConnectionRequestInfo cri, PrintWriter writer,
            ImaConnection con) throws ResourceException {
        factory = cf;
        subject = subj;
        requestInfo = cri;
        jmsConnection = con;
        userName = cf.getUser();
        if (writer != null) {
            setLogWriter(writer);           
        }
        try {
            jmsConnection.setExceptionListener(this);
        } catch (JMSException e) {
            ImaResourceException re = new ImaResourceException("CWLNC2160", e, "Managed connection initialization failed due to an exception. Connection configuration: {0}", factory);
            traceException(1, re);
            throw re;
        }
        if (isTraceable(5))
            trace(5, "ImaManagedConnection object " + this + " was created using " + factory);
    }

    boolean equalSubjects(final Subject subj) {
        PrivilegedAction<Boolean> pa = new PrivilegedAction<Boolean>() {
            @Override
            public Boolean run() {
                if (subject != null) {
                    if (subject.equals(subj))
                        return Boolean.TRUE;
                    return Boolean.FALSE;
                }
                return (subj == null) ? Boolean.TRUE : Boolean.FALSE;
            }
        };
        Boolean result = AccessController.doPrivileged(pa);
        return result.booleanValue();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnection#getConnection(javax.security.auth .Subject,
     * javax.resource.spi.ConnectionRequestInfo)
     */
    public Object getConnection(Subject subj, ConnectionRequestInfo cri) throws ResourceException {
        if ((requestInfo != null) && (!requestInfo.equals(cri))) {
            ImaResourceException re = ImaResourceException.managedConnCredentialMismatch("ConnectionRequestInfo", this);
            traceException(1, re);
            throw re;
        }
        if (!equalSubjects(subj)) {
            ImaResourceException re = ImaResourceException.managedConnCredentialMismatch("Subject", this);
            traceException(1, re);
            throw re;
        }
        synchronized (this) {
            if (jmsConnection == null) {
                ImaCommException ce = new ImaCommException("CWLNC2260", "Failed to create a connection handle for a managed connection because there is no physical connection to IBM MessageSight available.  Managed connection: {0}. This typically means that a previously existing physical connection is no longer available.  This can occur if the client application is not correctly configured or if the application does not correctly manage transactions when connections are broken unexpectedly.", (Throwable)null, this);
                traceException(1, ce);
                throw ce;
            }
            if (!connectionStarted) {
                try {
                    jmsConnection.start();
                    clientID = jmsConnection.getClientID();
                    metaData = jmsConnection.getMetaData();
                } catch (JMSException e) {
                    ImaResourceException re = new ImaResourceException("CWLNC2160", e,
                            "Managed connection initialization failed due to an exception. Connection configuration: {0}", factory);
                    traceException(1, re);
                    throw re;
                }
                connectionStarted = true;
            }
            ImaConnectionProxy cw = new ImaConnectionProxy(this, clientID, metaData);
            connectionHandles.add(cw);
            return cw;
        }       
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnection#destroy()
     */
    public void destroy() throws ResourceException {
        if (destroyed.compareAndSet(false, true)) {
            synchronized (this) {
                cleanup();
                try {
                    if (jmsConnection != null)
                        jmsConnection.close();
                    jmsConnection = null;
                } catch (JMSException e) {
                    ImaResourceException re = new ImaResourceException("CWLNC2163", e, "The physical connection to MessageSight failed to close due to an exception when destroying a managed connection. Managed connection: {0}.", this);
                    traceException(1, re);
                    throw re;
                }
            }
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnection#cleanup()
     */
    public void cleanup() throws ResourceException {
        synchronized (this) {
            for (ImaConnectionProxy cw : connectionHandles) {
                try {
                    cw.closeInternal(jmsConnection == null);
                } catch (JMSException e) {
                    traceException(5, e);
                }
            }
            connectionHandles.clear();
            transactionActive = 0;
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnection#associateConnection(java.lang.Object )
     */
    public void associateConnection(Object obj) throws ResourceException {
        if (obj instanceof ImaConnectionProxy) {
            ImaConnectionProxy cw = (ImaConnectionProxy) obj;
            synchronized (this) {
                if (connectionHandles.contains(cw))
                    return;
                connectionHandles.add(cw);
            }
            cw.reassociate(this);
        } else {
            ImaResourceException re = new ImaResourceException("CWLNC2164", "Failed to associate connection handle {0} with a managed connection because {2} is not an IBM MessageSight connection proxy object. This might indicate a problem with the application server. Managed connection: {1}", obj, this, obj.getClass().getName());
            traceException(1, re);
            throw re;
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnection#addConnectionEventListener(javax .resource.spi.ConnectionEventListener)
     */
    public void addConnectionEventListener(final ConnectionEventListener listener) {
        eventListeners.add(listener);
        if (connectionErrorEvent != null) {
            Timer timer = factory.getTimer();
            TimerTask tt = new TimerTask() {
                public void run() {
                    listener.connectionErrorOccurred(connectionErrorEvent);
                }
            };
            timer.schedule(tt, 0);
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnection#removeConnectionEventListener(javax
     * .resource.spi.ConnectionEventListener)
     */
    public void removeConnectionEventListener(ConnectionEventListener listener) {
        eventListeners.remove(listener);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnection#getXAResource()
     */
    public XAResource getXAResource() throws ResourceException {;
        try {
            return new ImaXARProxy(factory, (ImaXASession) createPhysicalSession(true, true), this, factory.getTraceLevel());
        } catch (JMSException e) {
            onException(e);
            ImaResourceException re = new ImaResourceException("CWLNC2165", e, "The XAResource for a managed connection failed to be created due to an exception. Managed connection: {0}.", this);
            traceException(1, re);
            throw re;
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnection#getLocalTransaction()
     */
    public LocalTransaction getLocalTransaction() throws ResourceException {
        return new LocalTransactionImpl();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnection#getMetaData()
     */
    public ManagedConnectionMetaData getMetaData() throws ResourceException {
        return new ImaManagedConnectionMetaData(userName, metaData);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnection#setLogWriter(java.io.PrintWriter)
     */
    public void setLogWriter(PrintWriter writer) throws ResourceException {
        if (jmsConnection != null) {
            jmsConnection.setPrintWriter(writer);
            logWriter = writer;
            trace(5, "logWriter set to "+logWriter);
        } else {
            ImaTrace.trace(1, "Failed to set logWriter to " + writer +". Log writer is currently "+logWriter +".");
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.resource.spi.ManagedConnection#getLogWriter()
     */
    public PrintWriter getLogWriter() throws ResourceException {
        return logWriter;
    }

    boolean inTransaction() {
        return (transactionActive != 0);
    }

    ConnectionRequestInfo getConnectionRequestInfo() {
        return requestInfo;
    }

    Subject getSubject() {
        return subject;
    }

    ImaConnection getJmsConnection() {
        return jmsConnection;
    }

    /**
     * Fired by a handle to indicate that the application server is associating it with a new ManagedConnection, so
     * remove it from this one
     * 
     * @param handle
     */
    void disassociate(ImaConnectionProxy handle) {
        synchronized(this) {
            // remove it from the list
            connectionHandles.remove(handle);
        }
    }

    private void notifyListeners(ConnectionEvent ce) {
        switch (ce.getId()) {
        case ConnectionEvent.CONNECTION_CLOSED:
            for (ConnectionEventListener cel : eventListeners) {
                cel.connectionClosed(ce);
            }
            break;
        case ConnectionEvent.LOCAL_TRANSACTION_STARTED:
            for (ConnectionEventListener cel : eventListeners) {
                cel.localTransactionStarted(ce);
            }
            break;
        case ConnectionEvent.LOCAL_TRANSACTION_COMMITTED:
            for (ConnectionEventListener cel : eventListeners) {
                cel.localTransactionCommitted(ce);
            }
            break;
        case ConnectionEvent.LOCAL_TRANSACTION_ROLLEDBACK:
            for (ConnectionEventListener cel : eventListeners) {
                cel.localTransactionRolledback(ce);
            }
            break;
        default:
            for (ConnectionEventListener cel : eventListeners) {
                cel.connectionErrorOccurred(ce);
            }
            break;
        }
    }

    public void onClose(ImaProxy p) {
        synchronized (this) {
            if (!connectionHandles.remove(p)) {
                // TODO: ????
                return;
            }
        }
        ConnectionEvent ce = new ConnectionEvent(this, ConnectionEvent.CONNECTION_CLOSED);
        ce.setConnectionHandle(p);
        notifyListeners(ce);
    }

    /*
     * (non-Javadoc)
     * 
     * @see com.ibm.ima.ra.outbound.ImaProxyContainer#onError(com.ibm.ima.ra.outbound.ImaProxy, javax.jms.JMSException)
     */
    public void onError(ImaProxy proxy, JMSException jex) {
        synchronized (this) {
            if (!connectionHandles.contains(proxy)) {
                // TODO: ????
                return;
            }
        }
        onException(jex);
    }

    public void onException(JMSException jex) {
        synchronized (this) {
            if (jmsConnection == null)
                return;
            if (!((ImaProperties) jmsConnection).getBoolean("isClosed", false)) {
                if (isTraceable(5))
                    trace(5, "Connection remains open after exception " + jex.getMessage());
                return;
            }
            jmsConnection = null;
        }
        // TODO: cleanup() ????

        // Physical JMS connection is broken - set it to null and notify listeners
        connectionErrorEvent = new ConnectionEvent(this, ConnectionEvent.CONNECTION_ERROR_OCCURRED, jex);
        notifyListeners(connectionErrorEvent);
    }

    public void finalize() {
        try {
            destroy();
        } catch (ResourceException e) {
        }
    }

    ImaSession getPhysicalSession() throws JMSException {
        return createPhysicalSession(false, false);
    }

    synchronized ImaMessageProducer getPhysicalProducer(int sessionType) throws JMSException {
        if (jmsProducers[sessionType] != null)
            return jmsProducers[sessionType];
        ImaSession sess = jmsSessions[sessionType];
        if (sess == null) {
            ImaJmsExceptionImpl ex = new ImaJmsExceptionImpl("CWLNC2566", "Failed to create Java EE client application producer because the physical JMS session is not available. This indicates that there is no physical connection available.");
            traceException(1, ex);
            throw ex;
        }
        jmsProducers[sessionType] = (ImaMessageProducer) sess.createProducer(null);
        return jmsProducers[sessionType];
    }

    
    synchronized ImaSession createPhysicalSession(boolean transacted, boolean xa) throws JMSException {
        if (jmsConnection == null) {
            return null;
        }
        if (xa) {
            if (jmsSessions[2] == null) {
                jmsSessions[2] = (ImaXASession) jmsConnection.createXASession();
            }
            return jmsSessions[2];
        }
        if (transacted) {
            if (jmsSessions[1] == null) {
                jmsSessions[1] = (ImaSession) jmsConnection.createSession(transacted, Session.AUTO_ACKNOWLEDGE);
            }
            return jmsSessions[1];
        }
        if (transactionActive == 2) /* We are in the middle of global transaction */
            return jmsSessions[2];
        if (transactionActive == 1) /* We are in the middle of local transaction */
            return jmsSessions[1];

        if (jmsSessions[0] == null) {
            jmsSessions[0] = (ImaSession) jmsConnection.createSession(false, Session.AUTO_ACKNOWLEDGE);
        }
        return jmsSessions[0];
    }

    /*
     * (non-Javadoc)
     * 
     * @see com.ibm.ima.ra.common.ImaXAObserver#onXAEvent(com.ibm.ima.ra.common.ImaXAObserver.XAEvent)
     */
    public void onXAEvent(XAEvent event) {
        switch (event) {
        case START:
            transactionActive = 2;
            break;
        case END:
            // case COMMIT:
            // case ROLLBACK:
            // case FORGET:
            transactionActive = 0;
        default:
            break;
        }
    }

    private final class LocalTransactionImpl implements LocalTransaction {
        ImaSession session;
        LocalTransactionImpl() {
        }

        /*
         * (non-Javadoc)
         * 
         * @see javax.resource.cci.LocalTransaction#begin()
         */
        public void begin() throws ResourceException {
            try {
                session = createPhysicalSession(true, false);
                if (session == null) {
                    ImaCommException ce = new ImaCommException("CWLNC2261", "A call to begin() for a transaction on a managed connection failed because a physical session is not available. This indicates that there is no physical connection available. Managed connection: {0}.", (Throwable)null, this);
                    traceException(1, ce);
                    throw ce;
                }
                transactionActive = 1;
                // ConnectionEvent ce = new ConnectionEvent(ImaManagedConnection.this,
                // ConnectionEvent.LOCAL_TRANSACTION_STARTED);
                // notifyListeners(ce);
            } catch (JMSException e) {
                ImaResourceException re = new ImaResourceException("CWLNC2167", e, "A call to begin() for a transaction on a managed connection failed due to an exception when creating the physical session. Managed connection: {0}.", this);
                traceException(1, re);
                throw re;
            }

        }

        /*
         * (non-Javadoc)
         * 
         * @see javax.resource.cci.LocalTransaction#commit()
         */
        public void commit() throws ResourceException {
            try {
                if (session != null) {
                    transactionActive = 0;
                    session.commit();
                    // ConnectionEvent ce = new ConnectionEvent(ImaManagedConnection.this,
                    // ConnectionEvent.LOCAL_TRANSACTION_COMMITTED);
                    // notifyListeners(ce);
                } else {
                    ImaResourceException re = new ImaResourceException("CWLNC2168", "A call to commit() for a transaction on a managed connection failed because the local transaction was not started. The client application called commit() without first calling begin(). Managed connection: {0}.", this);
                    traceException(1, re);
                    throw re;
                }
            } catch (JMSException e) {
                onException(e);
                ImaResourceException re = new ImaResourceException("CWLNC2169", e, "A call to commit() for a transaction on a managed connection failed due to an exception when committing the physical session. Managed connection: {0}", this);
                traceException(1, re);
                throw(re);
            } finally {
                session = null;
            }
        }

        /*
         * (non-Javadoc)
         * 
         * @see javax.resource.cci.LocalTransaction#rollback()
         */
        public void rollback() throws javax.resource.ResourceException {
            try {
                if (session != null) {
                    transactionActive = 0;
                    session.rollback();
                    // ConnectionEvent ce = new ConnectionEvent(ImaManagedConnection.this,
                    // ConnectionEvent.LOCAL_TRANSACTION_ROLLEDBACK);
                    // notifyListeners(ce);
                } else {
                    ImaResourceException re = new ImaResourceException("CWLNC2170", "A call to rollback() for a transaction on a managed connection failed because the local transaction was not started. The client application called rollback() without first calling begin(). Managed connection: {0}.", this);
                    traceException(1, re);
                    throw re;
                }
            } catch (JMSException e) {
                onException(e);
                ImaResourceException re = new ImaResourceException("CWLNC2171", e, "A call to rollback() for a transaction on a managed connection failed due to an exception when rolling back the physical session. Managed connection: {0}.", this);
                traceException(1, re);
                throw re;
            } finally {
                session = null;
            }
        }

    }
    
    public String toString() {
        StringBuffer sb = new StringBuffer("ImaManagedConnection={");
        int count = 0;
        /*
         * Defect 44432.  Remove printing of connection handles.  
         * We cannot safely include this with synchronization.
         * If we determine that we need to see the connection proxies 
         * in toString(), consider using  ConcurrentHashMap<K,V> instead.
         */

        /*
         * Once set, factory should never be null again.
         */
        if(factory != null)
            sb.append("Factory=" + factory.getImaFactory().toString() + " ");
        else
            sb.append("Factory=null");
        ImaProperties props = jmsConnection;
        if (props != null) {
            sb.append("JMSConnection=" + props +" ");
            sb.append("JMSConnectionStatus=" + (!((props).getBoolean("isClosed", false))?"active":"closed"));
        } else {
            sb.append("JMSConnection=null");
        }
        sb.append("}");
        return sb.toString();
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
        if (jmsConnection != null)          
            return ((ImaConnection)jmsConnection).isTraceable(level);
        return false;
    }

    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#trace(int, java.lang.String)
     */
    public void trace(int level, String message) {
        if (jmsConnection != null)
            ((ImaConnection)jmsConnection).trace(level, message);
        else
            ImaTrace.trace(level, message);   
    }

    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#trace(java.lang.String)
     */
    public void trace(String message) {
        if (jmsConnection != null)
            ((ImaConnection)jmsConnection).trace(message);
        else
            ImaTrace.trace(message);
    }

    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#traceException(java.lang.Throwable)
     */
    public JMSException traceException(Throwable ex) {
        if (jmsConnection != null)
            return ((ImaConnection)jmsConnection).traceException(ex);
        return ImaTrace.traceException(ex);
    }

    /*
     * @see com.ibm.ima.jms.impl.ImaTraceable#traceException(int, java.lang.Throwable)
     */
    public JMSException traceException(int level, Throwable ex) {
        if (jmsConnection != null)
            return ((ImaConnection)jmsConnection).traceException(level, ex);
        return ImaTrace.traceException(level, ex);
    }
    
    public boolean equals(Object o) {
        return super.equals(o);
    }
    
    public int hashCode() {
        return super.hashCode();
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
