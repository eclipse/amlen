/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.jms.impl;

import java.io.Serializable;

import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaPropertiesImpl;

import javax.jms.BytesMessage;
import javax.jms.ConnectionMetaData;
import javax.jms.Destination;
import javax.jms.ExceptionListener;
import javax.jms.JMSConsumer;
import javax.jms.JMSContext;
import javax.jms.JMSException;
import javax.jms.JMSProducer;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.ObjectMessage;
import javax.jms.Queue;
import javax.jms.QueueBrowser;
import javax.jms.Session;
import javax.jms.StreamMessage;
import javax.jms.TemporaryQueue;
import javax.jms.TemporaryTopic;
import javax.jms.XAJMSContext;

import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.transaction.xa.XAResource;


public class ImaJmsContext extends ImaReadonlyProperties implements JMSContext, XAJMSContext {

    private static final long serialVersionUID = -1479670495752388862L;
    
    ImaSession session;
    ImaConnection connect;
    boolean  autostart = true;
    boolean  managed;
    int      mode;
    boolean  isClosed;

    /*
     * Construct a JMSContext including the connection.
     */
    public ImaJmsContext(ImaProperties props, String userid, String password, int mode, int traceLevel, boolean managed) {
        super(new ImaPropertiesImpl(ImaJms.PTYPE_Connection));
        ((ImaPropertiesImpl)this.props).props.putAll(((ImaPropertiesImpl)props).props);
        this.managed = managed;
        this.mode = mode;
        try {
            connect = (ImaConnection) new ImaConnection(props, userid, password, ImaJms.Common, traceLevel);
        } catch (JMSException jex) {
            isClosed = true;
            throw ImaRuntimeException.mapException(jex);
        }
        if (mode != AUTO_ACKNOWLEDGE && mode != SESSION_TRANSACTED &&
            mode != CLIENT_ACKNOWLEDGE && mode != DUPS_OK_ACKNOWLEDGE) {
            ImaJmsRuntimeException jex = new ImaJmsRuntimeException("CWLNC0000", "The session mode is not valid.");
            connect.traceException(2, jex);
            throw jex;
        }
        if (ImaTrace.isTraceable(6)) {
            ImaTrace.trace("JMSContext created: context=" + toString(ImaConstants.TS_All));
        }
        props.remove("ObjectType");
        
        if (mode >= 0) {
            putInternal("sessionMode", mode);
        }
    }
    
    
    /*
     * Construct a JMSContext from an existing JMSContext.
     */
    public ImaJmsContext(ImaJmsContext context, int mode) {
        super(new ImaPropertiesImpl(ImaJms.PTYPE_Connection));
        this.connect = context.connect;
        this.managed = context.managed;
        this.mode = mode;
        System.out.println("in ImsJmsContext");
        if (mode != AUTO_ACKNOWLEDGE && mode != SESSION_TRANSACTED &&
            mode != CLIENT_ACKNOWLEDGE && mode != DUPS_OK_ACKNOWLEDGE) {
            ImaJmsRuntimeException jex = new ImaJmsRuntimeException("CWLNC0000", "The session mode is not valid.");
            connect.traceException(2, jex);
            throw jex;
        } 
        if (mode >= 0) {
            putInternal("sessionMode", mode);
        } else {
            this.props.remove("sessionMode");
        }
    }
    
    
    /* 
     * Check if the session is closed.
     * Also create a session at this point if required.  This is done here so that you can
     * set the clientID after creating the JMSContext.
     */
    void checkClosed() {
        if (session == null && !isClosed) {
            try {
                boolean transacted = false;
                int     ackmode = Session.AUTO_ACKNOWLEDGE;
                switch (mode) {
                default:
                case JMSContext.AUTO_ACKNOWLEDGE:                                                      break;
                case JMSContext.SESSION_TRANSACTED:    if (!managed) transacted = true;                break;
                case JMSContext.DUPS_OK_ACKNOWLEDGE:   ackmode = Session.DUPS_OK_ACKNOWLEDGE;          break;
                case JMSContext.CLIENT_ACKNOWLEDGE:    ackmode = Session.CLIENT_ACKNOWLEDGE;           break;
                }
                session = (ImaSession) connect.createSessionInternal(transacted, ackmode, false);
            } catch (JMSException jex) {
                jex.printStackTrace(System.out);
                isClosed = true;
                throw ImaRuntimeException.mapException(jex);
            }
        }
        if (isClosed || session.isClosed.get()) {
            ImaIllegalStateRuntimeException jex = new ImaIllegalStateRuntimeException("CWLNC0008", "The connection is closed");
            connect.traceException(2, jex);
            throw jex;
        }
    }
    


    /* 
     * @see javax.jms.JMSContext#acknowledge()
     */
    public void acknowledge() {
        checkClosed();
        try {
            session.acknowledge(false);
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /* 
     * @see javax.jms.JMSContext#close()
     */
    public void close() {
        if (ImaConnection.currentConnection.get() == connect) {
            throw new ImaIllegalStateRuntimeException("CWLNC0000", "Cannot close context at this time.");
        }
        synchronized (this) {
            isClosed = true;
            putInternal("isClosed", true);
            if (session != null) {
                try {
                    session.closeInternal();
                    session = null;
                    if (connect.sessionsMap.isEmpty())
                        connect.close();
                } catch (JMSException jex) {
                    throw ImaRuntimeException.mapException(jex);
                }
            }  
        }    
        if (ImaTrace.isTraceable(6)) {
            ImaTrace.trace("JMSContext closed: context=" + toString(ImaConstants.TS_All));
        }
    }
    
    
    /*
     * Close the connection on finalize of the application failed to close it.
     * @see java.lang.Object#finalize()
     */
    protected void finalize() {
        try {
            connect.close();
        } catch (JMSException jex) {
        }
    }

    /* 
     * @see javax.jms.JMSContext#commit()
     */
    public void commit() {
        checkClosed();
        try {
            session.commit();
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /* 
     * @see javax.jms.JMSContext#createBrowser(javax.jms.Queue)
     */
    public QueueBrowser createBrowser(Queue queue) {
        checkClosed();
        try {
            return session.createBrowser(queue);
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /* 
     * @see javax.jms.JMSContext#createBrowser(javax.jms.Queue, java.lang.String)
     */
    public QueueBrowser createBrowser(Queue queue, String selector) {
        checkClosed();
        try {
            return session.createBrowser(queue, selector);
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /* 
     * @see javax.jms.JMSContext#createBytesMessage()
     */
    public BytesMessage createBytesMessage() {
        checkClosed();
        return (BytesMessage)new ImaBytesMessage(session);
    }

    /* 
     * @see javax.jms.JMSContext#createConsumer(javax.jms.Destination)
     */
    public JMSConsumer createConsumer(Destination dest) {
        checkClosed();
        return new ImaJmsConsumer(this, dest, null, false, false, null, false);
    }

    /* 
     * @see javax.jms.JMSContext#createConsumer(javax.jms.Destination, java.lang.String)
     */
    public JMSConsumer createConsumer(Destination dest, String selector) {
        checkClosed();
        return new ImaJmsConsumer(this, dest, null, false, false, selector, false);
    }

    /* 
     * @see javax.jms.JMSContext#createConsumer(javax.jms.Destination, java.lang.String, boolean)
     */
    public JMSConsumer createConsumer(Destination dest, String selector, boolean nolocal) {
        checkClosed();
        return new ImaJmsConsumer(this, dest, null, false, false, selector, nolocal);
    }

    /* 
     * @see javax.jms.JMSContext#createContext(int)
     */
    public JMSContext createContext(int mode) {
        checkClosed();
        System.out.println("in createContext");
        return new ImaJmsContext(this, mode);
    }

    /* 
     * @see javax.jms.JMSContext#createDurableConsumer(javax.jms.Topic, java.lang.String)
     */
    public JMSConsumer createDurableConsumer(Topic topic, String name) {
        checkClosed();
        return new ImaJmsConsumer(this, topic, name, true, false, null, false);
    }

    /* 
     * @see javax.jms.JMSContext#createDurableConsumer(javax.jms.Topic, java.lang.String, java.lang.String, boolean)
     */
    public JMSConsumer createDurableConsumer(Topic topic, String name, String selector, boolean nolocal) {
        checkClosed();
        return new ImaJmsConsumer(this, topic, name, true, false, selector, nolocal);
    }

    /* 
     * @see javax.jms.JMSContext#createMapMessage()
     */
    public MapMessage createMapMessage() {
        checkClosed();
        return (MapMessage)new ImaMapMessage(session);
    }

    /* 
     * @see javax.jms.JMSContext#createMessage()
     */
    public Message createMessage() {
        checkClosed();
        return (Message)new ImaMessage(session);
    }

    /* 
     * @see javax.jms.JMSContext#createObjectMessage()
     */
    public ObjectMessage createObjectMessage() {
        checkClosed();
        return (ObjectMessage)new ImaObjectMessage(session);
    }

    /*
     * @see javax.jms.JMSContext#createObjectMessage(java.io.Serializable)
     */
    public ObjectMessage createObjectMessage(Serializable obj) {
        checkClosed();
        ImaObjectMessage omsg = new ImaObjectMessage(session);
        if (obj != null) {
            try {
                omsg.setObject(obj);
            } catch (JMSException jex) {
                throw ImaRuntimeException.mapException(jex);
            }
        }
        return omsg;
    }

    /* 
     * @see javax.jms.JMSContext#createProducer()
     */
    public JMSProducer createProducer() {
        checkClosed();
        return new ImaJmsProducer(this);
    }

    /* 
     * @see javax.jms.JMSContext#createQueue(java.lang.String)
     */
    public Queue createQueue(String queueName) {
        checkClosed();
        return new ImaQueue(queueName);
    }

    /* 
     * @see javax.jms.JMSContext#createSharedConsumer(javax.jms.Topic, java.lang.String)
     */
    public JMSConsumer createSharedConsumer(Topic topic, String name) {
        checkClosed();
        return new ImaJmsConsumer(this, topic, name, false, true, null, false);
    }

    /* 
     * @see javax.jms.JMSContext#createSharedConsumer(javax.jms.Topic, java.lang.String, java.lang.String)
     */
    public JMSConsumer createSharedConsumer(Topic topic, String name, String selector) {
        checkClosed();
        return new ImaJmsConsumer(this, topic, name, false, true, selector, false);
    }

    /*
     * @see javax.jms.JMSContext#createSharedDurableConsumer(javax.jms.Topic, java.lang.String)
     */
    public JMSConsumer createSharedDurableConsumer(Topic topic, String name) {
        checkClosed();
        return new ImaJmsConsumer(this, topic, name, true, true, null, false);
    }

    /* 
     * @see javax.jms.JMSContext#createSharedDurableConsumer(javax.jms.Topic, java.lang.String, java.lang.String)
     */
    public JMSConsumer createSharedDurableConsumer(Topic topic, String name, String selector) {
        checkClosed();
        return new ImaJmsConsumer(this, topic, name, true, true, selector, false);
    }

    /* 
     * @see javax.jms.JMSContext#createStreamMessage()
     */
    public StreamMessage createStreamMessage() {
        checkClosed();
        return new ImaStreamMessage(session);
    }

    /* 
     * @see javax.jms.JMSContext#createTemporaryQueue()
     */
    public TemporaryQueue createTemporaryQueue() {
        checkClosed();
        try {
            return session.createTemporaryQueue();
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /* 
     * @see javax.jms.JMSContext#createTemporaryTopic()
     */
    public TemporaryTopic createTemporaryTopic() {
        checkClosed();
        try {
            return session.createTemporaryTopic();
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /* 
     * @see javax.jms.JMSContext#createTextMessage()
     */
    public TextMessage createTextMessage() {
        checkClosed();
        ImaTextMessage tmsg = new ImaTextMessage(session);
        setTextMessage(tmsg, null);
        return tmsg;
    }
    /* 
     * @see javax.jms.TextMessage#setText(java.lang.String)
     */
    void setTextMessage(ImaTextMessage msg, String text) {
        checkClosed();
        if (text == null) {
            msg.body = null;
            msg.text = null;
        } else {
            int utflen = ImaUtils.sizeUTF8(text);
            if (utflen < 0) {
                ImaMessageFormatRuntimeException jex = new ImaMessageFormatRuntimeException("CWLNC0039", "The UTF-16 string encoding is not valid");
                ImaTrace.traceException(2, jex);
                throw jex;
            }

            msg.body = ImaUtils.makeUTF8(text, utflen, null);
            msg.text = text;
        }   
    }

    /*
     * @see javax.jms.JMSContext#createTextMessage(java.lang.String)
     */
    public TextMessage createTextMessage(String text) {
        checkClosed();
        ImaTextMessage tmsg = new ImaTextMessage(session);
        setTextMessage(tmsg, text);
        return tmsg;
    }

    /* 
     * @see javax.jms.JMSContext#createTopic(java.lang.String)
     */
    public Topic createTopic(String topicName) {
        checkClosed();
        return new ImaTopic(topicName);
    }

    /* 
     * @see javax.jms.JMSContext#getAutoStart()
     */
    public boolean getAutoStart() {
        checkClosed();
        return autostart;
    }

    /* 
     * @see javax.jms.JMSContext#getClientID()
     */
    public String getClientID() {
        checkClosed();
        return connect.clientid;
    }

    /* 
     * @see javax.jms.JMSContext#getExceptionListener()
     */
    public ExceptionListener getExceptionListener() {
        checkClosed();
        return connect.exlisten;
    }

    /* 
     * @see javax.jms.JMSContext#getMetaData()
     */
    public ConnectionMetaData getMetaData() {
        checkClosed();
        return connect.metadata;
    }

    /* 
     * @see javax.jms.JMSContext#getSessionMode()
     */
    public int getSessionMode() {
        checkClosed();
        return mode;
    }

    /* 
     * @see javax.jms.JMSContext#getTransacted()
     */
    public boolean getTransacted() {
        checkClosed();
        return mode == 0;
    }

    /* 
     * @see javax.jms.JMSContext#recover()
     */
    public void recover() {
        checkClosed();
        try {
            session.recover();
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /* 
     * @see javax.jms.JMSContext#rollback()
     */
    public void rollback() {
        checkClosed();
        try {
            session.rollback();
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /* 
     * @see javax.jms.JMSContext#setAutoStart(boolean)
     */
    public void setAutoStart(boolean autostart) {
        checkClosed();
        if (managed) {
            ImaIllegalStateRuntimeException isx = new ImaIllegalStateRuntimeException("CWLNA0000",
                    "AutoStart cannot be set in a container managed context");
            connect.traceException(isx);
            throw(isx);
        }    
        this.autostart = autostart;
    }

    /* 
     * @see javax.jms.JMSContext#setClientID(java.lang.String)
     */
    public void setClientID(String clientid) {
        if (session != null) {
            throw new ImaIllegalStateRuntimeException("CWLNC0000", "Cannot set the clientID on an active context.");
        }
        try {
            connect.setClientID(clientid);
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /* 
     * @see javax.jms.JMSContext#setExceptionListener(javax.jms.ExceptionListener)
     */
    public void setExceptionListener(ExceptionListener listener) {
        checkClosed();
        try {
            connect.setExceptionListener(listener);
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /* 
     * @see javax.jms.JMSContext#start()
     */
    public void start() {
        checkClosed();
        try {
            connect.start();
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /* 
     * @see javax.jms.JMSContext#stop()
     */
    public void stop() {
        checkClosed();
        try {
            connect.stop();
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /* 
     * @see javax.jms.JMSContext#unsubscribe(java.lang.String)
     */
    public void unsubscribe(String name) {
        checkClosed();
        try {
            session.unsubscribe(name);
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /*
     * 
     * @see javax.jms.XAJMSContext#getContext()
     */
    public JMSContext getContext() {
        // TODO Auto-generated method stub
        return null;
    }

    /*
     * 
     * @see javax.jms.XAJMSContext#getXAResource()
     */
    public XAResource getXAResource() {
        // TODO Auto-generated method stub
        return null;
    }
    
    /*
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return toString(ImaConstants.TS_Common);
    }
    
    /*
     * Class name for toString 
     */
    public String getClassName() {
        return "ImaJMSContext";
    }

}
