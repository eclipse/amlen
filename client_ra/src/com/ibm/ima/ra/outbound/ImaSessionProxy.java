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

import java.io.Serializable;
import java.util.ArrayList;

import javax.jms.BytesMessage;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.MessageProducer;
import javax.jms.ObjectMessage;
import javax.jms.Queue;
import javax.jms.QueueBrowser;
import javax.jms.QueueReceiver;
import javax.jms.QueueSender;
import javax.jms.QueueSession;
import javax.jms.Session;
import javax.jms.StreamMessage;
import javax.jms.TemporaryQueue;
import javax.jms.TemporaryTopic;
import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.jms.TopicPublisher;
import javax.jms.TopicSession;
import javax.jms.TopicSubscriber;

import com.ibm.ima.jms.ImaSubscription;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaIllegalStateException;
import com.ibm.ima.jms.impl.ImaJmsExceptionImpl;
import com.ibm.ima.jms.impl.ImaMessageConsumer;
import com.ibm.ima.jms.impl.ImaMessageProducer;
import com.ibm.ima.jms.impl.ImaQueueBrowser;
import com.ibm.ima.jms.impl.ImaSession;
import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.jms.impl.ImaXASession;

/**
 * Proxy for a JMS Session to enforce the one-session-per-connection rule.
 */
final class ImaSessionProxy extends ImaProxy implements Session, QueueSession, TopicSession, ImaProxyContainer, ImaSubscription {

    // ==========================================================================

    /** the JMS Session wrapped by this class */
    private final ImaSession          jmsSession;

    /** the Proxys created by this object */
    private final ArrayList<ImaProxy> proxyObjects = new ArrayList<ImaProxy>();
    
    private final int                 ackMode;

    private final int                 sessionType;
    
    private ImaMessageProducer        producer     = null;

    // ==========================================================================

    /**
     * Constructor taking a reference to the owning ConnectionWrapper, and a reference to a JMS Session.
     * 
     * @param s
     * @param w
     * @throws JMSException
     */
    ImaSessionProxy(ImaConnectionProxy c, ImaSession s, int ackMode) throws JMSException {
        super(c);
        if (s == null) {
            ImaJmsExceptionImpl ex = new ImaJmsExceptionImpl("CWLNC2563", "Failed to create a Java EE client application session because the physical session is not available. This indicates that there is no physical connection available.");
            traceException(1, ex);
            throw ex;
        }
        jmsSession = s;
        this.ackMode = ackMode;
        if (s instanceof ImaXASession) {
            sessionType = 2;
            return;
        }
        if (s.getTransacted()) {
            sessionType = 1;
            return;
        }
        sessionType = 0;
    }

    /*
     * @see javax.jms.Session#createBytesMessage()
     */
    public BytesMessage createBytesMessage() throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createBytesMessage();
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createMapMessage()
     */
    public MapMessage createMapMessage() throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try{
            return jmsSession.createMapMessage();
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createMessage()
     */
    public Message createMessage() throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createMessage();
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createObjectMessage()
     */
    public ObjectMessage createObjectMessage() throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createObjectMessage();
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createObjectMessage(java.io.Serializable)
     */
    public ObjectMessage createObjectMessage(Serializable arg0) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createObjectMessage(arg0);
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createStreamMessage()
     */
    public StreamMessage createStreamMessage() throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
        return jmsSession.createStreamMessage();
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createTextMessage()
     */
    public TextMessage createTextMessage() throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createTextMessage();
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createTextMessage(java.lang.String)
     */
    public TextMessage createTextMessage(String arg0) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createTextMessage(arg0);
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /**
     * @see javax.jms.Session#getTransacted()
     */
    public boolean getTransacted() throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.getTransacted();
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#getAcknowledgeMode()
     */
    public int getAcknowledgeMode() throws JMSException {
        // check that this Session is still alive
        checkOpen();
        return ackMode;
    }

    /*
     * @see javax.jms.Session#commit()
     */
    public void commit() throws JMSException {
        // TODO: Verify
        // http://docs.oracle.com/javaee/1.4/tutorial/doc/JMS6.html#wp92878
        // In an Enterprise JavaBeans component, you cannot use the Session.commit and Session.rollback methods.
        ImaIllegalStateException ex = ImaIllegalStateException.apiNotAllowedForSessionProxy("commit");
        traceException(1, ex);
        throw ex;
    }

    /*
     * @see javax.jms.Session#rollback()
     */
    public void rollback() throws JMSException {
        // TODO: Verify
        // http://docs.oracle.com/javaee/1.4/tutorial/doc/JMS6.html#wp92878
        // In an Enterprise JavaBeans component, you cannot use the Session.commit and Session.rollback methods.
        ImaIllegalStateException ex = ImaIllegalStateException.apiNotAllowedForSessionProxy("rollback");
        traceException(1, ex);
        throw ex;
    }

    /*
     * Close object which can be subclassed
     */
    protected void closeImpl(boolean connClosed) throws JMSException {
        synchronized (proxyObjects) {
            for (ImaProxy proxy : proxyObjects) {
                proxy.closeInternal(connClosed);
            }
            proxyObjects.clear();
        }
    }

    /*
     * @see javax.jms.Session#recover()
     */
    public void recover() throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            jmsSession.recover();
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#getMessageListener()
     */
    public MessageListener getMessageListener() throws JMSException {
        ImaIllegalStateException ex = ImaIllegalStateException.apiNotAllowedForSessionProxy("getMessageListener");
        traceException(1, ex);
        throw ex;
    }

    /**
     * @see javax.jms.Session#setMessageListener(javax.jms.MessageListener)
     */
    public void setMessageListener(MessageListener l) throws JMSException {
        ImaIllegalStateException ex = ImaIllegalStateException.apiNotAllowedForSessionProxy("setMessageListener");
        traceException(1, ex);
        throw ex;
    }

    /*
     * @see java.lang.Runnable#run()
     */
    public void run() {
        // This is never valid in managed or unmanaged environments
        // can't throw an exception as it's not in the signature so log it
        trace(5, "WARNING: Invalid call to " + this.getClass().getName() + ".run()");
    }

    /*
     * @see javax.jms.Session#createProducer(javax.jms.Destination)
     */
    public MessageProducer createProducer(Destination dest) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        if (dest != null)
            jmsSession.checkDestination(dest);

        try {
            synchronized (this) {
                if (producer == null) {
                    ImaConnectionProxy icp = (ImaConnectionProxy) container;
                    producer = icp.getPhysicalProducer(sessionType);
                }
                ImaMessageProducerProxy p = new ImaMessageProducerProxy(this, producer, dest);
                proxyObjects.add(p);
                return p;
            }
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createConsumer(javax.jms.Destination)
     */
    public MessageConsumer createConsumer(Destination dest) throws JMSException {
        return createConsumer(dest, null, false);
    }

    /*
     * @see javax.jms.Session#createConsumer(javax.jms.Destination, java.lang.String)
     */
    public MessageConsumer createConsumer(Destination dest, String msgSelector) throws JMSException {
        return createConsumer(dest, msgSelector, false);
    }

    /*
     * @see javax.jms.Session#createConsumer(javax.jms.Destination, java.lang.String, boolean)
     */
    public MessageConsumer createConsumer(Destination dest, String msgSelector, boolean noLocal) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            synchronized (this) {
                ImaMessageConsumer consumer = (ImaMessageConsumer) jmsSession.createConsumer(dest, msgSelector, noLocal);
                ImaMessageConsumerProxy c = new ImaMessageConsumerProxy(this, consumer, dest);
                proxyObjects.add(c);
                return c;
            }
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createQueue(java.lang.String)
     */
    public Queue createQueue(String queueName) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createQueue(queueName);
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createTopic(java.lang.String)
     */
    public Topic createTopic(String topicName) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createTopic(topicName);
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createDurableSubscriber(javax.jms.Topic, java.lang.String)
     */
    public TopicSubscriber createDurableSubscriber(Topic topic, String name) throws JMSException {
        return createDurableSubscriber(topic, name, "", false);
    }

    /*
     * @see javax.jms.Session#createDurableSubscriber(javax.jms.Topic, java.lang.String, java.lang.String, boolean)
     */
    public TopicSubscriber createDurableSubscriber(Topic topic, String name, String msgSelector, boolean noLocal)
            throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            synchronized (this) {
                ImaMessageConsumer consumer = (ImaMessageConsumer) jmsSession.createDurableSubscriber(topic, name, msgSelector,
                        noLocal);
                ImaMessageConsumerProxy c = new ImaMessageConsumerProxy(this, consumer, topic);
                proxyObjects.add(c);
                return c;
            }
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createBrowser(javax.jms.Queue)
     */
    public QueueBrowser createBrowser(Queue queue) throws JMSException {
        return createBrowser(queue, null);
    }

    /*
     * @see javax.jms.Session#createBrowser(javax.jms.Queue, java.lang.String)
     */
    public QueueBrowser createBrowser(Queue queue, String msgSelector) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            synchronized (this) {
                ImaQueueBrowser browser = (ImaQueueBrowser) jmsSession.createBrowser(queue, msgSelector);
                ImaQueueBrowserProxy b = new ImaQueueBrowserProxy(this, browser, queue);
                proxyObjects.add(b);
                return b;
            }
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createTemporaryQueue()
     */
    public TemporaryQueue createTemporaryQueue() throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createTemporaryQueue();
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#createTemporaryTopic()
     */
    public TemporaryTopic createTemporaryTopic() throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createTemporaryTopic();
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.Session#unsubscribe(java.lang.String)
     */
    public void unsubscribe(String name) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            jmsSession.unsubscribe(name);
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * Utility method to do state checking
     * 
     * @throws javax.jms.IllegalStateException
     */
    protected void checkOpen() throws javax.jms.IllegalStateException {
        if (isClosed.get()) {
            // build a suitable exception
            ImaIllegalStateException ex = new ImaIllegalStateException("CWLNC0009", "A call to a Session object method failed because the session is closed.");
            traceException(1, ex);
            throw ex;
        }
    }

    /*
     * Method invoked by a Proxy when it is closed, to instruct the SessionProxy to remove it from the List of open
     * objects.
     * 
     * @param proxy that is closing
     */
    public void onClose(ImaProxy proxy) {
        synchronized (proxyObjects) {
            proxyObjects.remove(proxy);
        }
    }
    

    /*
     * @see com.ibm.ima.ra.outbound.ImaProxyContainer#onError(com.ibm.ima.ra.outbound.ImaProxy, javax.jms.JMSException)
     */
    public void onError(ImaProxy proxy, JMSException jex) {
        synchronized (proxyObjects) {
            if (!proxyObjects.contains(proxy))
                return;
        }
        if (isClosed.get())
            return;
        container.onError(this, jex);
    }


    /*
     * @see javax.jms.TopicSession#createPublisher(javax.jms.Topic)
     */
    public TopicPublisher createPublisher(Topic topic) throws JMSException {
        return (TopicPublisher) createProducer(topic);
    }

    /*
     * @see javax.jms.TopicSession#createSubscriber(javax.jms.Topic, java.lang.String, boolean)
     */
    public TopicSubscriber createSubscriber(Topic topic, String msgSelector, boolean noLocal) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            synchronized (this) {
                ImaMessageConsumer consumer = (ImaMessageConsumer) jmsSession.createSubscriber(topic, msgSelector, noLocal);
                ImaMessageConsumerProxy c = new ImaMessageConsumerProxy(this, consumer, topic);
                proxyObjects.add(c);
                return c;
            }
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.TopicSession#createSubscriber(javax.jms.Topic)
     */
    public TopicSubscriber createSubscriber(Topic topic) throws JMSException {
        return createSubscriber(topic, null, false);
    }

    /*
     * @see javax.jms.QueueSession#createReceiver(javax.jms.Queue, java.lang.String)
     */
    public QueueReceiver createReceiver(Queue queue, String msgSelector) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            synchronized (this) {
                ImaMessageConsumer consumer = (ImaMessageConsumer) jmsSession.createReceiver(queue, msgSelector);
                ImaMessageConsumerProxy c = new ImaMessageConsumerProxy(this, consumer, queue);
                proxyObjects.add(c);
                return c;
            }
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.QueueSession#createReceiver(javax.jms.Queue)
     */
    public QueueReceiver createReceiver(Queue queue) throws JMSException {
        return createReceiver(queue, null);
    }

    /*
     * @see javax.jms.QueueSession#createSender(javax.jms.Queue)
     */
    public QueueSender createSender(Queue queue) throws JMSException {
        return (QueueSender) createProducer(queue);
    }

    /*
     * @see com.ibm.ima.jms.ImaSubscription#createSharedConsumer(javax.jms.Topic, java.lang.String)
     */
    public MessageConsumer createSharedConsumer(Topic topic, String name) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createSharedConsumer(topic, name);
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see com.ibm.ima.jms.ImaSubscription#createSharedConsumer(javax.jms.Topic, java.lang.String, java.lang.String)
     */
    public MessageConsumer createSharedConsumer(Topic topic, String name,
            String selector) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createSharedConsumer(topic, name, selector);
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see com.ibm.ima.jms.ImaSubscription#createDurableConsumer(javax.jms.Topic, java.lang.String)
     */
    public MessageConsumer createDurableConsumer(Topic topic, String name)
            throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createDurableConsumer(topic, name);
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see com.ibm.ima.jms.ImaSubscription#createDurableConsumer(javax.jms.Topic, java.lang.String, java.lang.String, boolean)
     */
    public MessageConsumer createDurableConsumer(Topic topic, String name,
            String selector, boolean noLocal) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createDurableConsumer(topic, name, selector, noLocal);
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }

    /*
     * @see com.ibm.ima.jms.ImaSubscription#createSharedDurableConsumer(javax.jms.Topic, java.lang.String)
     */
    public MessageConsumer createSharedDurableConsumer(Topic topic, String name)
            throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createSharedDurableConsumer(topic, name);
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }
    
    public String toString() {
        return jmsSession.toString();
    }

    /*
     * @see com.ibm.ima.jms.ImaSubscription#createSharedDurableConsumer(javax.jms.Topic, java.lang.String, java.lang.String)
     */
    public MessageConsumer createSharedDurableConsumer(Topic topic,
            String name, String selector) throws JMSException {
        // check that this Session is still alive
        checkOpen();
        try {
            return jmsSession.createSharedDurableConsumer(topic, name, selector);
        } catch (JMSException jex) {
            onError(this, jex);
            throw jex;
        }
    }
    
    /*
     * Implement the exception trace
     */
    private void traceException(int level, Throwable ex) {
        ImaConnection conn = null;
        if (jmsSession != null)
                conn = (ImaConnection)jmsSession.get("Connection");
        if (conn != null)
            conn.traceException(1, ex);
        else
            ImaTrace.traceException(1, ex);
    }
    
    private void trace(int level, String msg) {
        ImaConnection conn = null;
        if (jmsSession != null)
            conn = (ImaConnection)jmsSession.get("Connection");
        if (conn != null)
            conn.trace(1, msg);
        else
            ImaTrace.trace(1, msg);
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
