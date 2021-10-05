/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

import java.util.zip.CRC32;

import javax.jms.BytesMessage;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.ObjectMessage;
import javax.jms.Queue;
import javax.jms.StreamMessage;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaProperties;

/**
 * Implement the MessgeProducer interface for the IBM MessageSight JMS client.
 *
 * A message producer can either set the destination when it is created, or specify it on
 * each send.  
 */
public class ImaProducer extends ImaReadonlyProperties {  

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    ImaSession session;                               /* The parent session */
    ImaConnection connect;
    final int                 producerid;
    final Destination dest;                                 /* The default destination */
    int deliverymode = Message.DEFAULT_DELIVERY_MODE; /* Default the delivery mode */
    int priority = Message.DEFAULT_PRIORITY;          /* Default the priority */
    long expire  = Message.DEFAULT_TIME_TO_LIVE;      /* Default the time to live */
    boolean disableMessageID = false;                 
    boolean disableTimestamp = false;
    boolean isClosed;   
    long    deliveryDelay = 0;
    char [] msgidBuffer = new char[19];
    int msgidCount = 0;
    long msgidTime = 0L;
    int     domain;
    private ImaProducerAction   sendMsgAction;        /* For non-anonymous producers */
    private ImaProducerAction   sendAnonMsgAction;    /* For anonymous msgs */
    
    static char [] base32 = {
        '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
        'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
    };
    
    
    /*
     * Create a message producer
     */
    public ImaProducer(ImaSession session, Destination dest, int domain) throws JMSException {
        super(null);
        this.session = session;
        this.connect = session.connect;
        this.dest = dest;
        this.domain = domain;
        /*
         * Get the default settings from the ConnectionFactory properties
         */
        int val;
        ImaProperties pdest = (ImaProperties) session.connect;
        val = pdest.getInt("DisableMessageID", -1);
        if (val >= 0)
            disableMessageID = val != 0;
        val = pdest.getInt("DisableMessageTimestamp", -1);
        if (val >= 0)
            disableTimestamp = val != 0;
        priority = session.defaultPriority;
        expire = pdest.getInt("DefaultTimeToLive", 0) * 1000;
        
        if (dest != null) {
            session.checkDestination(dest);
            this.props = ((ImaDestination)dest).getCurrentProperties();
            this.props.remove("SubscriptionShared");
            
            /*
             * Fill in the properties with actual values
             */
            switch (domain) {
            case ImaJms.Topic: putInternal("ObjectType", "topic");  break;
            case ImaJms.Queue: putInternal("ObjectType", "queue");  break;
            default:           putInternal("ObjectType", "common"); break;
            }
                    
            ImaAction act = new ImaSessionAction(ImaAction.createProducer, this.session);
            act.outBB = ImaUtils.putByteValue(act.outBB, (byte)domain);
            act.setHeaderCount(1);
            act.outBB = ImaUtils.putMapValue(act.outBB, this);
            synchronized (session) {
                session.checkClosed();
                act.request();
                putInternal("Connection", connect);
                putInternal("Session", session);
                if (act.rc == 0) {
                    producerid = ImaUtils.getInteger(act.inBB);
                    session.producerList.put(new Integer(producerid), this);
                } else {
                    switch (act.rc) {
                    case ImaReturnCode.IMARC_NotAuthorized:
                    case ImaReturnCode.IMARC_NotAuthenticated:
                        connect.trace(2, "Failed to create producer: reason=Authorization failed. producer="
                                + toString(ImaConstants.TS_All));
                        ImaJmsSecurityException jex = new ImaJmsSecurityException("CWLNC0207", "A client request to MessageSight failed due to an authorization failure.");
                        connect.traceException(2, jex);
                        throw jex;

                    case ImaReturnCode.IMARC_DestNotValid:
                        connect.trace(2, "Failed to create producer: reason=Destination not valid. producer="
                                + toString(ImaConstants.TS_All));
                        ImaJmsExceptionImpl jex2 = new ImaJmsExceptionImpl("CWLNC0217",
                                "A request to create a producer or a consumer failed because the destination {0} is not valid.", ((ImaProperties) dest).get("Name").toString());
                        connect.traceException(2, jex2);
                        throw jex2;

                    case ImaReturnCode.IMARC_BadSysTopic:
                        connect.trace(2, "Failed to create producer: reason=The topic is a system topic. producer="
                                + toString(ImaConstants.TS_All));
                        ImaJmsExceptionImpl jex4 = new ImaJmsExceptionImpl("CWLNC0225",
                                "A request to create a message producer or a durable subscription, or a request to publish a message failed because the destination {0} is a system topic.", ((ImaProperties) dest).get("Name").toString());
                        connect.traceException(2, jex4);
                        throw jex4;
                        
                    default:
                        connect.trace(2, "Failed to create producer: rc=" + act.rc + " producer="
                                + toString(ImaConstants.TS_All));
                        ImaJmsExceptionImpl jex3 = new ImaJmsExceptionImpl("CWLNC0203",
                                "A request to create a producer failed with MessageSight return code = {0}.", act.rc);
                        connect.traceException(2, jex3);
                        throw jex3;
                    }
                }
            }
        } else {
            this.props = new ImaPropertiesImpl(ImaJms.PTYPE_Connection);
            producerid = 0;
        }
        initMsgID();
        this.props.put("isClosed", false);
        if (connect.isTraceable(6)) {
            connect.trace("Producer created: producer=" + toString(ImaConstants.TS_All));
        }
    }

    
    /* 
     * Close the MessageProducer.
     * @see javax.jms.MessageProducer#close()
     */
    public void close() throws JMSException {
        if (!isClosed) {
            ImaConnectionFactory.checkAllowed(connect);
            session.closeProducer(this);
            closeInternal();
        }
    }
    
    /*
     * This internal close is called by the session close.
     * If the close is initiated by the session, we do not need to inform the session we are closing.
     */
    void closeInternal() throws JMSException {    
        synchronized (session.producer_lock) {
            if (!isClosed) {
                isClosed = true;
                this.props.put("isClosed", true);
                if (dest != null) {
                    ImaAction act = new ImaProducerAction(ImaAction.closeProducer, session, producerid);
                    act.request();
                } 
            }    
        }
    }
    
    void markClosed() {
        /* Defect 40238: Remove lock.  It is not needed for the markClosed()
         * case and it creates deadlocks in some cases.
         */
        if (!isClosed) {
            isClosed = true;
            ((ImaPropertiesImpl)this.props).putInternal("isClosed", true); 
        }  
    }

    
    /* 
     * Check if the session is closed
     */
    final void checkClosed() throws JMSException {
        if (isClosed) {
            ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0011", "A call to a MessageProducer object method failed because the producer is closed.");
            connect.traceException(2, jex);
            throw jex;
        }
    }
    
    /* 
     * Set the delivery mode.
     * 
     * @see javax.jms.MessageProducer#getDeliveryMode()
     */
    public final int getDeliveryMode() throws JMSException {
        checkClosed();
        return deliverymode;
    }

    
    /* 
     * Get the destination header field.
     * 
     * @see javax.jms.MessageProducer#getDestination()
     */
    public final Destination getDestination() throws JMSException {
        checkClosed();
        return dest;
    }

    
    /* 
     * Get the disable message ID setting.
     * @see javax.jms.MessageProducer#getDisableMessageID()
     */
    public final boolean getDisableMessageID() throws JMSException {
        checkClosed();
        return disableMessageID;
    }

    
    /* 
     * Get the disable timestamp setting.
     * 
     * @see javax.jms.MessageProducer#getDisableMessageTimestamp()
     */
    public final boolean getDisableMessageTimestamp() throws JMSException {
        checkClosed();
        return disableTimestamp;
    }

    
    /* 
     * Get the default priority.
     * 
     * @see javax.jms.MessageProducer#getPriority()
     */
    public final int getPriority() throws JMSException {
        checkClosed();
        return priority;
    }

    
    /* 
     * Get the default time to live.
     * 
     * @see javax.jms.MessageProducer#getTimeToLive()
     */
    public final long getTimeToLive() throws JMSException {
        checkClosed();
        return expire;
    }
    
    
    /*
     * Get the topic.
     * 
     * @see javax.jms.TopicPublisher#getTopic()
     */
    public final Topic getTopic() throws JMSException {
        checkClosed();
        return (Topic)dest;
    }
    

    /*
     * Publish on the default topic.
     * This is the same a send.
     * 
     * @see javax.jms.TopicPublisher#publish(javax.jms.Message)
     */
    public void publish(Message msg) throws JMSException {
        send(msg, this.deliverymode, this.priority, this.expire);
    }

    
    /*
     * Publish on a specified topic.
     * This is the same as send.
     * 
     * @see javax.jms.TopicPublisher#publish(javax.jms.Topic, javax.jms.Message)
     */
    public void publish(Topic topic, Message msg) throws JMSException {
        send((Destination)topic, msg, this.deliverymode, this.priority, this.expire);       
    }

    
    /*
     * Publish a message with explicit parameters.
     * This is the same as send.
     * 
     * @see javax.jms.TopicPublisher#publish(javax.jms.Message, int, int, long)
     */
    public void publish(Message msg, int delmode, int priority, long ttl) throws JMSException {
        send(msg, delmode, priority, ttl);        
    }

    
    /*
     * Topic based send. 
     * @see javax.jms.TopicPublisher#publish(javax.jms.Topic, javax.jms.Message, int, int, long)
     */
    public void publish(Topic topic, Message msg, int delmode, int priority, long ttl) throws JMSException {
        send((Destination)topic, msg, delmode, priority, ttl);        
    }
    
    
    /* 
     * Simple send of the message.
     * 
     * @see javax.jms.MessageProducer#send(javax.jms.Message)
     */
    public void send(Message msg) throws JMSException {
        send(msg, this.deliverymode, this.priority, this.expire);
    }

    
    /* 
     * Send with an explicit destination.
     * 
     * @see javax.jms.MessageProducer#send(javax.jms.Destination, javax.jms.Message)
     */
    public void send(Destination dest, Message msg) throws JMSException {
        send(dest, msg, this.deliverymode, this.priority, this.expire);
    }
    
    /*
     * Check when the method does not have a destination.
     */
    void checkNoDest() {
        if (this.dest == null) {
            ImaUnsupportedOperationException uex = new ImaUnsupportedOperationException("CWLNC0048", 
                             "A call to send() or publish() on a producer object failed because the destination is not set. The destination must be set."); 
            connect.traceException(2, uex);
            throw uex;
        }
    };    
    
    /* 
     * Send with explicit parameters.
     * 
     * @see javax.jms.MessageProducer#send(javax.jms.Message, int, int, long)
     */
    public void send(Message msg, int delmode, int priority, long ttl) throws JMSException {
        checkClosed();
        checkNoDest();
        checkDeliveryMode(msg, delmode);
        if (priority != Message.DEFAULT_PRIORITY)
            checkPriority(priority);
        sendM(this.dest, msg, delmode, priority, ttl, disableTimestamp, disableMessageID, deliveryDelay);
    }

    
    /*
     * Queue based send.
     * @see javax.jms.QueueSender#send(javax.jms.Queue, javax.jms.Message)
     */
    public void send(Queue queue, Message msg) throws JMSException {
        send((Destination)queue, msg, this.deliverymode, this.priority, this.expire);
    }


    /*
     * Queue based send.
     * @see javax.jms.QueueSender#send(javax.jms.Queue, javax.jms.Message, int, int, long)
     */
    public void send(Queue queue, Message msg, int delmode, int priority, long ttl) throws JMSException {
        send((Destination)queue, msg, delmode, priority, ttl);
    }  
    

    /*
     * Check the send
     */
    void checkDeliveryMode(Message message, int delmode) throws JMSException {
        if (delmode != DeliveryMode.PERSISTENT && delmode != DeliveryMode.NON_PERSISTENT) {
            ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0055", "A call to send() or publish() failed because the delivery mode is not valid. Valid delivery modes are DeliveryMode.NON_PERSISTENT and DeliveryMode.PERSISTENT.");
            connect.traceException(2, jex);
            throw jex;
        }
    
        if (message == null) {
            ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0042", "A call to send() or publish() on a message producer failed because the message is null. The message cannot be null.");
            connect.traceException(2, jex);
            throw jex;
        }
    }
     
    /*
     * Check when the method has a destination.
     */
    void checkHasDest() {
        if (this.dest != null) {
            ImaUnsupportedOperationException uex = new ImaUnsupportedOperationException("CWLNC0049", "A call to send() or publish() on a MessageProducer object failed because it specified a destination, but the destination was previously set on the producer.");
            connect.traceException(2, uex);
            throw uex; 
        }
    }
    
    /* 
     * Send a message.
     * All other sends with destination call this implementation.
     * 
     * @see javax.jms.MessageProducer#send(javax.jms.Destination, javax.jms.Message, int, int, long)
     */
    public void send(Destination dest, Message msg, int delmode, int priority, long ttl) throws JMSException {
        checkClosed();
        session.checkDestination(dest);
        checkHasDest();
        checkDeliveryMode(msg, delmode);
        if (priority != Message.DEFAULT_PRIORITY)
            checkPriority(priority);
        sendM(dest, msg, delmode, priority, ttl, disableTimestamp, disableMessageID, deliveryDelay);
    }  
    
    /* 
     * Send a message with all values specified.
     * This is called by the RA
     */
    public void send(Destination dest, Message msg, int delmode, int priority, long ttl,
            boolean disableTimestamp, boolean disableMessageID, long deliveryDelay) throws JMSException {
        checkClosed();
        session.checkDestination(dest);
        checkDeliveryMode(msg, delmode);
        if (priority != Message.DEFAULT_PRIORITY)
            checkPriority(priority);
        sendM(dest, msg, delmode, priority, ttl, disableTimestamp, disableMessageID, deliveryDelay);
    }  
    
    /*
     * Implementation of send.
     * The parameters should be checked at this point
     */
    void sendM(Destination dest, Message msg, int delmode, int priority, long ttl, 
            boolean disableTimestamp, boolean disableMessageID, long deliveryDelay) throws JMSException {
        ImaMessage lmsg = null;
        ImaProducerAction lmsgAction = null;
        Boolean disableACK = null;
        /*
         * If this is an ImaMessage, use it as it is.
         * Otherwise, create an ImaMessage with its properties and content.
         */
        if (msg instanceof ImaMessage) {
            lmsg = (ImaMessage)msg;
        } else if (msg instanceof BytesMessage) {
            lmsg = (ImaMessage)new ImaBytesMessage((BytesMessage)msg, session);
        } else if (msg instanceof MapMessage) {
            lmsg = (ImaMessage)new ImaMapMessage((MapMessage)msg, session);
        } else if (msg instanceof ObjectMessage) {
            lmsg = (ImaMessage)new ImaObjectMessage((ObjectMessage)msg, session);
        } else if (msg instanceof StreamMessage) {
            lmsg = (ImaMessage)new ImaStreamMessage((StreamMessage)msg, session); 
        } else if (msg instanceof TextMessage) {
            lmsg = (ImaMessage)new ImaTextMessage((TextMessage)msg, session); 
        } else {
            lmsg = (ImaMessage)new ImaMessage(session, ImaMessage.MTYPE_Message, msg);
        }

        /*
         * As we modify the input message, we must make sure that we lock the message from when 
         * the header fields are modified, until we build the JMS header.  Of course if the
         * application is sending the message from multiple threads, it cannot look at the
         * results of the send.
         */
        synchronized(msg) {
            /*
             * Complete the message
             */
            long ts;
            boolean expireSet = false;
            lmsg.setJMSDeliveryMode(delmode);
            lmsg.setJMSPriority(priority);
            lmsg.setJMSTimestamp(0);
            lmsg.setJMSExpiration(0);
            ts = 0;
            
            if (!disableTimestamp) {
                ts = System.currentTimeMillis();
                lmsg.setJMSTimestamp(ts);
                if (ttl > 0) {
                    lmsg.setJMSExpiration(ts + ttl);
                    expireSet = true;
                }    
                if (deliveryDelay > 0) { 
                    lmsg.setJMSDeliveryTime(ts + deliveryDelay);
                    expireSet = true;
                }    
            } else {
                ts = 0;
                lmsg.setJMSTimestamp(0);
                lmsg.setJMSExpiration(0);
            }
            lmsg.setJMSDestination(dest);
            if (!disableMessageID || deliveryDelay > 0) {
                lmsg.setJMSMessageID(makeMsgID());    
            } else {
                lmsg.setJMSMessageID(null);
            }
            
            /* 
             * Set send fields in the original message as well 
             */
            if (lmsg != msg) {
                msg.setJMSDeliveryMode(delmode);
                msg.setJMSPriority(priority);
                if (!disableTimestamp) {
                    msg.setJMSTimestamp(ts);
                    msg.setJMSExpiration(ttl > 0 ? ts + ttl : 0);
                } else {
                    msg.setJMSTimestamp(0);
                    msg.setJMSExpiration(0);
                }
                try { msg.setJMSDestination(dest); } catch (JMSException jmse) {}
                msg.setJMSMessageID(lmsg.getJMSMessageID());
            }
            ((ImaMessage)lmsg).reset();
        
            /*
             * The JMS specification requires that this be called only in a single thread,
             * and therefore we should not need to lock, and indeed we have not locked the
             * previous work here.  However, since this code should be synchronized, if
             * should not cost us much to do so here, and it will prevent almost all
             * consequences of violating the JMS session threading model.
             */
            synchronized (session.producer_lock) {
                final int actionType;
                final boolean wait;
                checkClosed();
                /*
                 * If the message is non-persistent, or in a transaction, then send without wait
                 */
                if (this.dest != null) {
                    if (((ImaDestination)this.dest).exists("DisableACK")) {
                        disableACK = ((ImaDestination)this.dest).getBoolean("DisableACK", false);
                    }
                    if ((lmsg.getJMSDeliveryMode() == DeliveryMode.PERSISTENT) || 
                       (session.transacted && !session.asyncSendAllowed)) {
                        actionType = ImaAction.messageWait;
                        wait = true;
                    } else {
                        wait = false;
                        actionType = ImaAction.message;
                    }
                    if (sendMsgAction == null) {
                        sendMsgAction = new ImaProducerAction(actionType, session, producerid);
                    } else {
                        sendMsgAction.reset(actionType, producerid);
                    }
                    if (connect.isloopbackclient)
                        sendMsgAction.senddestname = this.dest.toString();
                    lmsgAction = sendMsgAction;
                } else {
                    if (((ImaDestination)dest).exists("DisableACK")) {
                        disableACK = ((ImaDestination)dest).getBoolean("DisableACK", false);
                    }
                    if ((lmsg.getJMSDeliveryMode() == DeliveryMode.PERSISTENT)
                            || (session.transacted && !session.asyncSendAllowed)) {
                        actionType = ImaAction.messageNoProdWait;
                        wait = true;
                    } else {
                        actionType = ImaAction.messageNoProd;
                        wait = false;
                    }
                    domain = ((ImaPropertiesImpl) dest).getString("ObjectType")
                            .equals("topic") ? ImaJms.Topic : ImaJms.Queue;
                    String destname = (domain == ImaJms.Topic ? ((Topic) dest)
                            .getTopicName() : ((Queue) dest).getQueueName());
                    if (sendAnonMsgAction == null)
                        sendAnonMsgAction = new ImaProducerAction(actionType,
                                session, domain, destname);
                    else
                        sendAnonMsgAction.reset(actionType, domain, destname);
                    if (connect.isloopbackclient)
                        sendAnonMsgAction.senddestname = destname;
                    lmsgAction = sendAnonMsgAction;
                }
                //System.out.println("  actionType = " + actionType);
                
                // If disableACK is set on destination, it overrides the value set
                // on the ConnectionFactory.  Otherwise, the value from the
                // ConnectionFactory is used when sending messages.
                if (disableACK == null)
                    disableACK = this.session.disableACK;
                lmsgAction.setMessageHdr(lmsg.msgtype, lmsg.deliverymode, lmsg.priority, disableACK, 
                        expireSet, lmsg.retain); 
                
                lmsgAction.outBB = ImaUtils.putMapValue(lmsgAction.outBB, lmsg.props, lmsg, dest); 
                lmsgAction.request(lmsg.body, wait|connect.isloopbackclient);
                if (wait && (lmsgAction.rc != 0)) {
                    if (this.dest != null) 
                        dest = this.dest;
                    switch(lmsgAction.rc) {               
                    case ImaReturnCode.IMARC_MsgTooBig:
                        ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0215", "A call to send() or publish() to {0} {1} from producer {2} failed because the message size exceeds the maximum size permitted by MessageSight endpoint {3}.", 
                                (domain == ImaJms.Topic)?"topic":"queue",
                                (domain == ImaJms.Topic)?((Topic)dest).getTopicName():((Queue)dest).getQueueName(), 
                                (this.dest != null) ? producerid:"\"null\"",
                                connect.client.host+":"+connect.client.port);
                        connect.traceException(2, jex);
                        throw jex;
                    case ImaReturnCode.IMARC_DestinationFull:
                        ImaJmsExceptionImpl jex2 = new ImaJmsExceptionImpl("CWLNC0218", "A call to send() or publish() to {0} {1} from producer {2} failed because the destination is full.",
                                (domain == ImaJms.Topic)?"topic":"queue",
                                (domain == ImaJms.Topic)?((Topic)dest).getTopicName():((Queue)dest).getQueueName(), 
                                (this.dest != null) ? producerid:"\"null\"");
                        connect.traceException(2, jex2);
                        throw jex2;
                    case ImaReturnCode.IMARC_DestNotValid:
                        ImaInvalidDestinationException jex3 = new ImaInvalidDestinationException("CWLNC0219", "A call to send() or publish() to {0} {1} from producer {2} failed because the destination is not valid.",
                                (domain == ImaJms.Topic)?"topic":"queue",
                                (domain == ImaJms.Topic)?((Topic)dest).getTopicName():((Queue)dest).getQueueName(), 
                                (this.dest != null) ? producerid:"\"null\"");
                        connect.traceException(2, jex3);
                        throw jex3;
                    case ImaReturnCode.IMARC_BadSysTopic:
                        ImaJmsExceptionImpl jex6 = new ImaJmsExceptionImpl("CWLNC0225", "A request to create a message producer or a durable subscription, or a request to publish a message failed because the destination {0} is a system topic.",
                                (dest instanceof Topic) ? ((Topic)dest).getTopicName() : "");
                        connect.traceException(2, jex6);
                        throw jex6;
                    case ImaReturnCode.IMARC_NotAuthorized:
                    case ImaReturnCode.IMARC_NotAuthenticated: 
                        ImaJmsSecurityException jex7 = new ImaJmsSecurityException("CWLNC0207", "A client request to MessageSight failed due to an authorization failure.");
                        connect.traceException(2, jex7);
                        throw jex7;
                    case ImaReturnCode.IMARC_ServerCapacity:
                        ImaJmsExceptionImpl jex5 = new ImaJmsExceptionImpl("CWLNC0223", "A call to send() or publish() a persistent JMS message to {0} {1} from producer {2} failed because MessageSight could not store the message.",
                                (domain == ImaJms.Topic)?"topic":"queue",
                                (domain == ImaJms.Topic)?((Topic)dest).getTopicName():((Queue)dest).getQueueName(), 
                                (this.dest != null) ? producerid:"\"null\"",
                                lmsgAction.rc);
                        connect.traceException(2, jex5);
                        throw jex5;
                    default:
                        ImaJmsExceptionImpl jex4 = new ImaJmsExceptionImpl("CWLNC0216", "A call to send() or publish() to {0} {1} from producer {2} failed with MessageSight return code = {3}.", 
                                (domain == ImaJms.Topic)?"topic":"queue",
                                (domain == ImaJms.Topic)?((Topic)dest).getTopicName():((Queue)dest).getQueueName(), 
                                (this.dest != null) ? producerid:"\"null\"",
                                lmsgAction.rc);
                        connect.traceException(2, jex4);
                        throw jex4;
                    }
                }
            }   
        }
    }
    
    /*
     * Make a message ID.
     * The message ID consists of the required characters "ID:" followed by 40 bit
     * timestamp in base 32, a four digit producer unique string, and a four base32 counter.
     */
    String makeMsgID() {
        int count = msgidCount++;
        if ((count&0xFFFFF) != ((count-1)&0xFFFFF)) {   /* 20 bits in 4 base 32 digits */
            long now = System.currentTimeMillis();
            if (now != msgidTime) {
                msgidTime = now;
                now >>= 2;
                for (int i=10; i>2; i--) {
                    msgidBuffer[i] = base32[(int)(now&0x1f)]; 
                    now >>= 5;
                }
            }
        } 
        for (int i=18; i>14; i--) {
            msgidBuffer[i] = base32[count&0x1f];
            count >>= 5;
        }
        return new String(msgidBuffer);    
    }
    
    
    /*
     * Initialize the message ID
     */
    void initMsgID() {
        msgidBuffer[0] = 'I';
        msgidBuffer[1] = 'D';
        msgidBuffer[2] = ':';
        
        CRC32 crc = new CRC32();
        crc.update(session.connect.clientid.getBytes());
        long now = System.currentTimeMillis();
        crc.update((int)((now/77)&0xff));
        crc.update((int)((now/79731)&0xff));
        crc.update(hashCode()/77);
        long crcval = crc.getValue();
        for (int i=11; i<15; i++) {
            msgidBuffer[i] = base32[(int)(crcval&0x1F)];
            crcval >>= 5;
        }
        msgidCount = 0;
        makeMsgID();
    }
    
    
    /*
     * Get the value of a hex digit.  
     * We do not do error checking and any invalid character gives the value zero.
     */
    int getHexVal(char ch) {
        if (ch >= '0' && ch <= '9')
            return ch-'0';
        if (ch >= 'A' && ch <= 'F')
            return ch-'A'+10;
        if (ch >= 'a' && ch <= 'f')
            return ch-'a'+10;
        return 0;
    }
    
    
    /*
     * Convert a hex bitmap to a binary byte array.
     */
    byte [] toBitmap(String s) {
        if (s == null)
            return new byte[0];
        if ((s.length()&1) == 1)
            s = s+'0';
        byte [] b = new byte[s.length()/2];
        for (int i=0; i<s.length(); i+=2) {
            b[i/2] = (byte)((getHexVal(s.charAt(i))<<4) + getHexVal(s.charAt(i+1)));
        }
        return b;
    }
    
    
    /* 
     * Set the delivery mode parameter.
     * 
     * @see javax.jms.MessageProducer#setDeliveryMode(int)
     */
    public void setDeliveryMode(int delmode) throws JMSException {
        checkClosed();
        if (delmode != DeliveryMode.NON_PERSISTENT && delmode != DeliveryMode.PERSISTENT) {
            ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0035", "A call to the setDeliveryMode() method on a producer failed because the input value is not a valid delivery mode. Valid delivery modes are DeliveryMode.NON_PERSISTENT and DeliveryMode.PERSISTENT.");
            connect.traceException(2, jex);
            throw jex;
        }
        deliverymode = delmode;
    }

    
    /* 
     * Set the disable message ID setting.
     * 
     * @see javax.jms.MessageProducer#setDisableMessageID(boolean)
     */
    public void setDisableMessageID(boolean msgid) throws JMSException {
        checkClosed();
        disableMessageID = msgid;
    }

    
    /* 
     * Set the disable timestamp setting.
     * 
     * @see javax.jms.MessageProducer#setDisableMessageTimestamp(boolean)
     */
    public void setDisableMessageTimestamp(boolean msgts) throws JMSException {
        checkClosed();
        disableTimestamp = msgts;
    }

    /*
     * 
     */
    void checkPriority(int priority) throws JMSException {
        if (priority < 0 || priority > 9) {
            ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0036", "A call to the setPriority() method on a Message object or on a MessageProducer object failed because the input value is not a valid priority ({0}).  Valid priority values are integers in the range 0 to 9.", priority);
            connect.traceException(2, jex);
            throw jex;
        }
    }
    
    /* 
     * Set the default priority.
     * 
     * @see javax.jms.MessageProducer#setPriority(int)
     */
    public void setPriority(int priority) throws JMSException {
        checkClosed();
        checkPriority(priority);
        this.priority = priority;
    }

    
    /* 
     * Set the default time to live.
     * 
     * @see javax.jms.MessageProducer#setTimeToLive(long)
     */
    public void setTimeToLive(long ttl) throws JMSException {
        checkClosed();
        this.expire = ttl;
    }

    
    /*
     * Get the queue name.
     * 
     * @see javax.jms.QueueSender#getQueue()
     */
    public Queue getQueue() throws JMSException {
        checkClosed();
        return (Queue)dest;
    }


    /*
     * @see javax.jms.MessageProducer#getDeliveryDelay()
     * @since JMS 2.0
     */
    public long getDeliveryDelay() throws JMSException {
        checkClosed();
        return deliveryDelay;
    }

    /*
     * @see javax.jms.MessageProducer#setDeliveryDelay(long)
     * @since JMS 2.0
     */
    public void setDeliveryDelay(long deliveryDelay) throws JMSException {
        checkClosed();
        this.deliveryDelay = deliveryDelay;
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
        return "ImaMessageProducer";
    }
}
