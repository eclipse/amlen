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

import javax.jms.Connection;
import javax.jms.ConnectionConsumer;
import javax.jms.ConnectionMetaData;
import javax.jms.Destination;
import javax.jms.ExceptionListener;
import javax.jms.JMSException;
import javax.jms.Queue;
import javax.jms.QueueConnection;
import javax.jms.QueueSession;
import javax.jms.ServerSessionPool;
import javax.jms.Session;
import javax.jms.Topic;
import javax.jms.TopicConnection;
import javax.jms.TopicSession;
import javax.resource.ResourceException;

import com.ibm.ima.jms.impl.ImaIllegalStateException;
import com.ibm.ima.jms.impl.ImaJms;
import com.ibm.ima.jms.impl.ImaJmsExceptionImpl;
import com.ibm.ima.jms.impl.ImaMessageProducer;
import com.ibm.ima.jms.impl.ImaSession;
import com.ibm.ima.jms.impl.ImaTrace;

/**
 * This class wraps a JMS Connection and redirects calls as appropriate.
 */
class ImaConnectionProxy extends ImaProxy implements Connection, TopicConnection, QueueConnection, ImaProxyContainer {

    /** the SessionProxy created by this object */
    private ImaSessionProxy   sessionProxy     = null;

    private final String             clientID;

    private final ConnectionMetaData metaData;
    
    /**
     * Constructor taking a reference to the owning ManagedConnection, and the wrapped Connection object.
     * 
     * @param mc the ManagedConnection that owns this object
     * @param con the physical JMS Connection to IMA
     */
    ImaConnectionProxy(ImaManagedConnection mc, String cid, ConnectionMetaData connMD) {
        super(mc);
        clientID = cid;
        metaData = connMD;
    }

    /**
     * @see javax.jms.Connection#createSession(boolean, int)
     */
    public Session createSession(boolean transacted, int ackMode) throws JMSException {
        if (sessionProxy == null) {
            sessionProxy = new ImaSessionProxy(this, getPhysicalSession(), ackMode);
            return sessionProxy;
        }
        ImaIllegalStateException ex = new ImaIllegalStateException("CWLNC2561", "Failed to create a new session because the connection already has a session.  Connection: {0} Existing session: {1}.", clientID, sessionProxy);
        traceException(1, ex);
        throw ex;
    }

    /**
     * @see javax.jms.Connection#getClientID()
     */
    public String getClientID() throws JMSException {
        checkOpen();
        return clientID;
    }

    /**
     * @see javax.jms.Connection#setClientID(java.lang.String)
     */
    public void setClientID(String cid) throws JMSException {
        ImaIllegalStateException ex = ImaIllegalStateException.apiNotAllowedForConnectionProxy("setClientID");
        traceException(1, ex);
        throw ex;
    }

    /**
     * @see javax.jms.Connection#getMetaData()
     */
    public ConnectionMetaData getMetaData() throws JMSException {
        checkOpen();
        return metaData;
    }

    /**
     * @see javax.jms.Connection#getExceptionListener()
     */
    public ExceptionListener getExceptionListener() throws JMSException {
        checkOpen();
        return null;
    }

    /**
     * @see javax.jms.Connection#setExceptionListener(javax.jms.ExceptionListener)
     */
    public void setExceptionListener(ExceptionListener el) throws JMSException {
        ImaIllegalStateException ex = ImaIllegalStateException.apiNotAllowedForConnectionProxy("setExceptionListener");
        traceException(1, ex);
        throw ex;
    }

    /**
     * @see javax.jms.Connection#start()
     */
    public void start() throws JMSException {
        checkOpen();
    }

    /**
     * @see javax.jms.Connection#stop()
     */
    public void stop() throws JMSException {
        ImaIllegalStateException ex = ImaIllegalStateException.apiNotAllowedForConnectionProxy("stop");
        traceException(1, ex);
        throw ex;
    }

    /*
     * (non-Javadoc)
     * 
     * @see com.ibm.ima.ra.outbound.ImaProxy#closeImpl()
     */
    protected synchronized void closeImpl(boolean connClosed) throws JMSException {
        if (sessionProxy != null) {
            sessionProxy.closeInternal(connClosed);
        }
        sessionProxy = null;
    }

    /**
     * @see javax.jms.Connection#createConnectionConsumer(javax.jms.Destination, java.lang.String,
     *      javax.jms.ServerSessionPool, int)
     */
    public ConnectionConsumer createConnectionConsumer(Destination dst, String arg1, ServerSessionPool arg2, int arg3)
            throws JMSException {
        ImaJmsExceptionImpl ex = (ImaJmsExceptionImpl)ImaJms.notImplemented("createConnectionConsumer");
        traceException(1, ex);
        throw ex;
    }

    /**
     * @see javax.jms.Connection#createDurableConnectionConsumer(javax.jms.Topic, java.lang.String, java.lang.String,
     *      javax.jms.ServerSessionPool, int)
     */
    public ConnectionConsumer createDurableConnectionConsumer(Topic arg0, String arg1, String arg2,
            ServerSessionPool arg3, int arg4) throws JMSException {
        ImaJmsExceptionImpl ex = (ImaJmsExceptionImpl)ImaJms.notImplemented("createDurableConnectionConsumer");
        traceException(1, ex);
        throw ex;        
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.QueueConnection#createConnectionConsumer(javax.jms.Queue, java.lang.String,
     * javax.jms.ServerSessionPool, int)
     */
    @Override
    public ConnectionConsumer createConnectionConsumer(Queue arg0, String arg1, ServerSessionPool arg2, int arg3)
            throws JMSException {
        ImaJmsExceptionImpl ex = (ImaJmsExceptionImpl)ImaJms.notImplemented("createConnectionConsumer");
        traceException(1, ex);
        throw ex; 
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.QueueConnection#createQueueSession(boolean, int)
     */
    public QueueSession createQueueSession(boolean transacted, int ackMode) throws JMSException {
        return (QueueSession) createSession(transacted, ackMode);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.TopicConnection#createConnectionConsumer(javax.jms.Topic, java.lang.String,
     * javax.jms.ServerSessionPool, int)
     */
    public ConnectionConsumer createConnectionConsumer(Topic arg0, String arg1, ServerSessionPool arg2, int arg3)
            throws JMSException {
        ImaJmsExceptionImpl ex = (ImaJmsExceptionImpl)ImaJms.notImplemented("createConnectionConsumer");
        traceException(1, ex);
        throw ex;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.TopicConnection#createTopicSession(boolean, int)
     */
    public TopicSession createTopicSession(boolean transacted, int ackMode) throws JMSException {
        return (TopicSession) createSession(transacted, ackMode);
    }

    /**
     * Indicate that the application has closed the SessionWrapper
     * 
     * @param s the SessionWrapper being released
     */
    public synchronized void onClose(ImaProxy p) {
        if (sessionProxy == p)
            sessionProxy = null;
    }

    /*
     * (non-Javadoc)
     * 
     * @see com.ibm.ima.ra.outbound.ImaProxyContainer#onError(com.ibm.ima.ra.outbound.ImaProxy, javax.jms.JMSException)
     */
    public void onError(ImaProxy proxy, JMSException jex) {
        if (isClosed.get())
            return;
        synchronized (this) {
            if (sessionProxy != proxy)
                return;
        }
        container.onError(this, jex);
    }

    /**
     * Instruct the connection wrapper to disassociate itself from its owning ManagedConnection and link itself to the
     * supplied ManagedConnection.
     * 
     * @param mc the new ManagedConnection that the ConnectionHandle should use
     * @throws ResourceException if the call fails
     */
    void reassociate(ImaManagedConnection mc) throws ResourceException {
        ImaManagedConnection oldMC = (ImaManagedConnection) container;
        if (oldMC == mc)
            return;
        // disassociate ourself from the existing ManagedConnection
        oldMC.disassociate(this);
        synchronized (this) {
            if (sessionProxy != null) {
                /*
                 * I don't like this but some application do not close connection/session when exception is thrown. I
                 * assume that application server will not call reassociate on a connection that is in use. So we'll
                 * close current session proxy rather than throw an exception.
                 */
                try {
                    sessionProxy.closeInternal(false);
                    sessionProxy = null;
                } catch (JMSException jex) {
                    sessionProxy = null;
                    mc.onError(this, jex);
                }
            }
            container = mc;
            isClosed.set(false);
        }
    }

    /**
     * Utility method to check that the connection hasn't been closed
     * 
     * @throws javax.jms.IllegalStateException
     */
    private void checkOpen() throws javax.jms.IllegalStateException {
        if (isClosed.get()) {
            ImaIllegalStateException ex = new ImaIllegalStateException("CWLNC0008", "A call to a Connection object method failed because the connection is closed.");
            traceException(1, ex);
            throw ex;
        }
    }

    /**
     * finalize method
     */
    public void finalize() {
        try {
            close();
        } catch (JMSException e) {
            ImaTrace.traceException(1, e);
        }
    }

    private final ImaSession getPhysicalSession() throws JMSException {
        ImaManagedConnection mc = (ImaManagedConnection) container;
        return mc.getPhysicalSession();
    }

    final ImaMessageProducer getPhysicalProducer(int sessionType) throws JMSException {
        ImaManagedConnection mc = (ImaManagedConnection) container;
        return mc.getPhysicalProducer(sessionType);
    }
    
    private void traceException(int level, Throwable ex) {
        if (((ImaManagedConnection)container) != null) {
            ((ImaManagedConnection)container).traceException(1, ex);
        } else {
            ImaTrace.traceException(1, ex);
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
