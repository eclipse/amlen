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

import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageProducer;
import javax.jms.Queue;
import javax.jms.QueueSender;
import javax.jms.Topic;
import javax.jms.TopicPublisher;

import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaIllegalStateException;
import com.ibm.ima.jms.impl.ImaJmsExceptionImpl;
import com.ibm.ima.jms.impl.ImaMessageProducer;
import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.jms.impl.ImaUnsupportedOperationException;

/*
 * Proxy a JMS client MessageProducer.
 * 
 * Since in the outbound RA it is very common to create a producer for each message, 
 * we do not actually create a JMS client object on either the client or server, but
 * use a single JMS client MessageProducer and always specify the destination on send.
 * 
 * To do that, we need to keep all producer state within this producer proxy object.
 */
final class ImaMessageProducerProxy extends ImaProxy implements MessageProducer, TopicPublisher, QueueSender {

    /** the Producer wrapped by this class */
    private final ImaMessageProducer producer;

    private final Destination        destination;
    
    /** the session proxy associated with this producer proxy */
    private final ImaSessionProxy    sessProxy;

    private long                     ttl;
    private int                      deliveryMode;
    private int                      priority;
    private boolean                  disableTimestamp;
    private boolean                  disableMessageID;

    /**
     * Constructor
     * 
     * @throws JMSException
     */
    ImaMessageProducerProxy(ImaSessionProxy s, ImaMessageProducer p, Destination dest) throws JMSException {
        super(s);
        producer = p;
        destination = dest;
        sessProxy = s;
        ttl = Message.DEFAULT_TIME_TO_LIVE;
        deliveryMode = Message.DEFAULT_DELIVERY_MODE;
        priority = Message.DEFAULT_PRIORITY;
        disableTimestamp = false;
        disableMessageID = false;
    }

    /*
     * @see javax.jms.MessageProducer#setDisableMessageID(boolean)
     */
    public void setDisableMessageID(boolean disableMsgId) throws JMSException {
        checkOpen();
        disableMessageID = disableMsgId;
    }

    /*
     * @see javax.jms.MessageProducer#getDisableMessageID()
     */
    public boolean getDisableMessageID() throws JMSException {
        checkOpen();
        return disableMessageID;
    }

    /*
     * @see javax.jms.MessageProducer#setDisableMessageTimestamp(boolean)
     */
    public void setDisableMessageTimestamp(boolean disableTimestamp) throws JMSException {
        checkOpen();
        this.disableTimestamp = disableTimestamp;
    }

    /*
     * @see javax.jms.MessageProducer#getDisableMessageTimestamp()
     */
    public boolean getDisableMessageTimestamp() throws JMSException {
        checkOpen();
        return disableTimestamp;
    }

    /**
     * @see javax.jms.MessageProducer#setDeliveryMode(int)
     */
    public void setDeliveryMode(int deliveryMode) throws JMSException {
        checkOpen();
        if (deliveryMode != DeliveryMode.NON_PERSISTENT && deliveryMode != DeliveryMode.PERSISTENT) {
            ImaJmsExceptionImpl ex = new ImaJmsExceptionImpl("CWLNC0035", "A call to the setDeliveryMode() method on a producer failed because the input value is not a valid delivery mode. Valid delivery modes are DeliveryMode.NON_PERSISTENT and DeliveryMode.PERSISTENT.");
            traceException(1, ex);
            throw ex;
        }
        this.deliveryMode = deliveryMode;
    }

    /**
     * @see javax.jms.MessageProducer#getDeliveryMode()
     */
    public int getDeliveryMode() throws JMSException {
        checkOpen();
        return deliveryMode;
    }

    /*
     * @see javax.jms.MessageProducer#setPriority(int)
     */
    public void setPriority(int priority) throws JMSException {
        checkOpen();
        if (priority < 0 || priority > 9) {
            ImaJmsExceptionImpl ex = new ImaJmsExceptionImpl("CWLNC0036", "A call to the setPriority() method on a Message object or on a MessageProducer object failed because the input value is not a valid priority ({0}).  Valid priority values are integers in the range 0 to 9.", priority);
            traceException(1, ex);
            throw ex;
        }
        this.priority = priority;
    }

    /*
     * @see javax.jms.MessageProducer#getPriority()
     */
    public int getPriority() throws JMSException {
        checkOpen();
        return priority;
    }

    /*
     * @see javax.jms.MessageProducer#setTimeToLive(long)
     */
    public void setTimeToLive(long ttl) throws JMSException {
        checkOpen();
        this.ttl = ttl;
    }

    /*
     * @see javax.jms.MessageProducer#getTimeToLive()
     */
    public long getTimeToLive() throws JMSException {
        checkOpen();
        return ttl;
    }

    /*
     * @see javax.jms.MessageProducer#getDestination()
     */
    public Destination getDestination() throws JMSException {
        checkOpen();
        return destination;
    }

    /*
     * @see com.ibm.ima.ra.outbound.ImaProxy#closeImpl(boolean)
     */
    protected void closeImpl(boolean connClosed) throws JMSException {
    }
    
    /*
     * Check when the method does not have a destination.
     */
    void checkNoDest() {
        if (this.destination == null) {
            throw new ImaUnsupportedOperationException("CWLNC0048", 
                 "A call to send() or publish() on a MessageProducer object failed because the destination is not set. The destination must be set."); 
        }
    };
    
    /*
     * Check when the method has a destination.
     */
    void checkHasDest() {
        if (this.destination != null) {
            throw new ImaUnsupportedOperationException("CWLNC0049", "A call to send() or publish() on a MessageProducer object failed because it specified a destination, but the destination was previously set on the producer.");
        }
    }

    /*
     * @see javax.jms.MessageProducer#send(javax.jms.Message)
     */
    public void send(Message msg) throws JMSException {
        send(msg, deliveryMode, priority, ttl);
    }

    /**
     * @see javax.jms.MessageProducer#send(javax.jms.Message, int, int, long)
     */
    public void send(Message msg, int deliveryMode, int priority, long timeToLive) throws JMSException {
        checkOpen();
        checkNoDest();
        try {
            producer.send(destination, msg, deliveryMode, priority, timeToLive, disableTimestamp, disableMessageID, 0L);
        } catch (JMSException jex) {
            onError(jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.MessageProducer#send(javax.jms.Destination, javax.jms.Message)
     */
    public void send(Destination dest, Message msg) throws JMSException {
        send(dest, msg, deliveryMode, priority, ttl);
    }

    /*
     * @see javax.jms.MessageProducer#send(javax.jms.Destination, javax.jms.Message, int, int, long)
     */
    public void send(Destination dest, Message msg, int deliveryMode, int priority, long timeToLive)
            throws JMSException {
        checkOpen();
        checkHasDest();
        try {
            producer.send(dest, msg, deliveryMode, priority, timeToLive, disableTimestamp, disableMessageID, 0L);
        } catch (JMSException jex) {
            onError(jex);
            throw jex;
        }
    }

    /*
     * @see javax.jms.QueueSender#getQueue()
     */
    public Queue getQueue() throws JMSException {
        checkOpen();
        return (Queue) destination;
    }

    /**
     * @see javax.jms.QueueSender#send(javax.jms.Queue, javax.jms.Message)
     */
    public void send(Queue dest, Message msg) throws JMSException {
        send((Destination) dest, msg, deliveryMode, priority, ttl);
    }

    /**
     * @see javax.jms.QueueSender#send(javax.jms.Queue, javax.jms.Message, int, int, long)
     */
    public void send(Queue dest, Message msg, int deliveryMode, int priority, long timeToLive) throws JMSException {
        send((Destination) dest, msg, deliveryMode, priority, timeToLive);
    }

    /*
     * @see javax.jms.TopicPublisher#getTopic()
     */
    public Topic getTopic() throws JMSException {
        checkOpen();
        return (Topic) destination;
    }

    /*
     * @see javax.jms.TopicPublisher#publish(javax.jms.Message)
     */
    public void publish(Message msg) throws JMSException {
        send(msg, deliveryMode, priority, ttl);
    }

    /*
     * @see javax.jms.TopicPublisher#publish(javax.jms.Message, int, int, long)
     */
    public void publish(Message msg, int deliveryMode, int priority, long timeToLive) throws JMSException {
        send(msg, deliveryMode, priority, timeToLive);
    }

    /*
     * @see javax.jms.TopicPublisher#publish(javax.jms.Topic, javax.jms.Message)
     */
    public void publish(Topic dest, Message msg) throws JMSException {
        send((Destination) dest, msg, deliveryMode, priority, ttl);
    }

    /*
     * @see javax.jms.TopicPublisher#publish(javax.jms.Topic, javax.jms.Message, int, int, long)
     */
    public void publish(Topic dest, Message msg, int deliveryMode, int priority, long timeToLive) throws JMSException {
        send((Destination) dest, msg, deliveryMode, priority, timeToLive);
    }
    
    /*
     * Check for failed physical connection so app server is notified
     * 
     * @param ex the JMSExcetion that triggered the error
     */
    public void onError(JMSException ex) {
        sessProxy.onError(this, ex);
    }

    /*
     * Utility method to do state checking
     * 
     * @throws javax.jms.IllegalStateException
     */
    protected void checkOpen() throws javax.jms.IllegalStateException {
        if (isClosed.get()) {
            ImaIllegalStateException ex = new ImaIllegalStateException("CWLNC0011", "A call to a MessageProducer object method failed because the producer is closed.");
            traceException(1, ex);
            throw ex;
        }
    }
    
    /*
     * Implement trace exception
     */
    private void traceException(int level, Throwable ex) {
        ImaConnection conn = null;
        if (producer != null)
            conn = (ImaConnection)producer.get("Connection");
        if (conn != null)
            conn.traceException(1, ex);
        else
            ImaTrace.traceException(1, ex);
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
