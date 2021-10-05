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

package com.ibm.ima.jms.impl;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.Queue;
import javax.jms.QueueReceiver;
import javax.jms.Session;
import javax.jms.Topic;
import javax.jms.TopicSubscriber;

import com.ibm.ima.jms.ImaProperties;

/**
 * Implement the MessageConsumer interface for the IBM MessageSight JMS client.
 *
 */
public class ImaMessageConsumer extends ImaReadonlyProperties 
        implements MessageConsumer, TopicSubscriber, QueueReceiver, ImaConsumer {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    ImaSession session;                /* The parent session */
    ImaConnection connect;             /* The connection which is the parent of the session */
    int consumerid;
    String  selector;                  /* The selector as a string */
    ImaMessageSelector select;         /* The compiled selector */
    boolean nolocal;                   /* The noLocal setting */
    boolean durable;                   /* The durable setting */
    String  subscriptionname;          /* The name of the durable or shared subscription */
    MessageListener  listener;         /* The message listener */
    ImaDestination dest;               /* The destination to listen to */
    volatile boolean isClosed;         /* A flag to indicate that the consumer is closed */
    volatile boolean isStopped = true;         /* A flag to indicate that the consumer is closed */
    boolean isPrimary;
    boolean isActive;
    ImaMessageConsumer next;
    boolean disableACK = false;
    ImaProperties topic = null;
    int     domain;                    /* Topic or Queue */
    volatile ImaReceiveAction     rcvAction = null;
    private ImaConsumerAction    resumeAction = null;
    /* Asynchronously received messages for sync */
    LinkedBlockingQueue<ImaMessage>receivedMessages = new LinkedBlockingQueue<ImaMessage>();
    volatile long lastDeliveredMessage = 0;
    int fullSize;              
    int emptySize;
    int shared = 0;
    private int msgCount = 0; 
    private final boolean[] suspendFlags = new boolean[2];
    final ImaDeliveryThread   deliveryThread;
    private final Lock mutex = new ReentrantLock();    
    static HashMap<Integer, ImaAction>  pendingActions = new HashMap<Integer, ImaAction>();
    
    /*
     * Create a message consumer.
     * We have combined all of the various creator together in this method.
     */
    public ImaMessageConsumer(ImaSession session, Destination dest, String selector, boolean nolocal, boolean durable, String subscriptionname, int domain)
        throws JMSException {
        super(null);
        this.domain = domain;
        /*
         * Set class variables
         */
        if (!(dest instanceof ImaDestination)) {
        	ImaInvalidDestinationException jex = new ImaInvalidDestinationException("CWLNC0013", "An operation that requires an IBM MessageSight destination failed because a non-MessageSight destination ({0}) was used.", dest);
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        if (((ImaProperties)dest).get("Name") == null) {
        	ImaInvalidDestinationException jex = new ImaInvalidDestinationException("CWLNC0007", "An operation that requires the destination name to be set failed because the destination name is not set.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }    
        deliveryThread = session.getDeliveryThread();
        this.session = session;
        this.connect = session.connect;
        this.dest = (ImaDestination)dest;
        this.selector = selector;
        this.nolocal = nolocal;
        this.durable = durable;
        this.subscriptionname = subscriptionname;
        
        /* 
         * Compile selector
         */
        if (selector != null && selector.length()>0) {
            select = new ImaMessageSelector(selector);
        }    
        
        /*
         * Check for temporary destination
         */
        if (dest instanceof ImaTemporaryTopic) {
            ImaTemporaryTopic tdest = (ImaTemporaryTopic)dest;
            if (tdest.connect != connect) {
            	ImaInvalidDestinationException jex = new ImaInvalidDestinationException("CWLNC0004", 
                     "A method called to create a message consumer failed because it specified a temporary topic or a temporary queue that is associated with a different connection.");   
                ImaTrace.traceException(2, jex);
                throw jex;
            }
        }
        if (dest instanceof ImaTemporaryQueue) {
            ImaTemporaryQueue tdest = (ImaTemporaryQueue)dest;
            if (tdest.connect != connect) {
            	ImaInvalidDestinationException jex = new ImaInvalidDestinationException("CWLNC0004", 
                     "A method called to create a message consumer failed because it specified a temporary topic or a temporary queue that is associated with a different connection.");   
                ImaTrace.traceException(2, jex);
                throw jex;
            }
        }
        
        /*
         * Get the current properties for the message consumer
         */
        topic = this.dest.getCurrentProperties();
        this.props = topic;

        switch (domain) {
        case ImaJms.Topic_False:
        case ImaJms.Topic_Shared:
        case ImaJms.Topic_SharedNonDurable:
        case ImaJms.Topic_GlobalShared:
        case ImaJms.Topic_GlobalNoConsumer:
	    case ImaJms.Topic:     props.put("ObjectType", "topic");  break;
	    case ImaJms.Queue:     props.put("ObjectType", "queue");  break;
	    default:               props.put("ObjectType", "common"); break;
        }
        
        /*
         * Only set DisableACK property if there is a value to inherit from connection via
         * session or if it has been set on the destination.
         */
        if (((ImaProperties)session).exists("DisableACK") || (((ImaProperties)topic).exists("DisableACK")))
        	putInternal("DisableACK",topic.getBoolean("DisableACK", session.disableACK));
        
        disableACK = this.getBoolean("DisableACK", false);

        /*
         * Parse the subscription shared 
         */
        shared = 0;
        if (connect.client.server_version >= ImaConnection.Client_version_1_1) {
            switch (domain) {
            case ImaJms.Queue:           
            case ImaJms.Topic_False:            props.remove("SubscriptionShared");                         break;
            case ImaJms.Topic_Shared:           props.put("SubscriptionShared", "True");       shared = 1;  break;
            case ImaJms.Topic_SharedNonDurable: props.put("SubscriptionShared", "NonDurable"); shared = 2;  break;
            case ImaJms.Topic_GlobalShared:     props.put("SubscriptionShared", "Global");     shared = 1;  break;
            case ImaJms.Topic_GlobalNoConsumer: props.put("SubscriptionShared", "NoConsumer"); shared = 1;  break;
            
            default:              
                String sharedE = this.getString("SubscriptionShared");
                if (durable) {
                    if (sharedE != null) {
                        shared = ImaJms.SubSharedEnum.getValue(sharedE);
                        if (shared == ImaEnumeration.UNKNOWN) {
                            shared = 0;
                            props.remove("SubscriptionShared");  /* Ignore unknown value */ 
                        } else {
                            props.put("SubscriptionShared", ImaJms.SubSharedEnum.getName(shared));
                        }
                    }   

                } else {
                    if (sharedE != null)
                        props.remove("SubscriptionShared");       /* Ignored if not durable */
                }
                break;
            }    
        } else {
            props.remove("SubscriptionShared");       /* Ignored if not durable */
        }
        
        /* 
         * A durable subscription cannot be made if the clientID was not specified.
         */
        if (shared==0 && durable && connect.generated_clientid) {
            ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0014", 
               "A method called to create a durable subscription failed because the connection has a generated client ID.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }

        /*
         * Determine cache size
         */
        int defaultCacheSize = session.fullSize;
        if (((domain == ImaJms.Queue) && (session.ackmode == Session.AUTO_ACKNOWLEDGE)) || shared > 0)
            defaultCacheSize = 0; 
        fullSize = topic.getInt("ClientMessageCache", defaultCacheSize);
        if (fullSize < 0)
        	fullSize = 0;
        emptySize = fullSize/4;

        /*
         * Process other properties
         */
        if (subscriptionname != null) 
            topic.put("subscriptionName", subscriptionname);
        topic.put("isClosed", false);
        if (shared == 0)
            topic.put("noLocal", nolocal);
        putInternal("Connection", connect);
        putInternal("Session", session);
        putInternal("ClientMessageCache", fullSize);
        suspendFlags[0] = false;
        suspendFlags[1] = false;
        /*
         * For a temporary consumer, keep track of the consumers
         */
        if (dest instanceof ImaTemporaryTopic) {
            ((ImaTemporaryTopic)dest).addConsumer(this);
        } 
        if (dest instanceof ImaTemporaryQueue) {
            ((ImaTemporaryQueue)dest).addConsumer(this);
        } 
    }
    
    
    /*
     * @see com.ibm.ima.jms.impl.ImaConsumer#getDest()
     */
    public ImaDestination getDest() {
    	return dest;
    }
    
    /* 
     * Close the consumer.
     * If it is already closed, do nothing.
     * Notify the session that we are closing.
     * @see javax.jms.MessageConsumer#close()
     */
    public void close() throws JMSException {
        if (!isClosed) {
            session.closeConsumer(this);
            closeInternal();
        }
    }
    
    
    /*
     * Internal close without notification to the session.
     * This is used to implement the close method, and to implement the close of the parent session.
     */
    public void closeInternal() throws JMSException {
        synchronized (this) {
            if (!isClosed) {
                /* 
                 * If we could have unacked messages, ack them now 
                 */
            	if (ImaTrace.isTraceable(6)) { 
            		ImaTrace.trace("Consumer is closed: consumer=" + toString(ImaConstants.TS_All) + 
            				" session=" + session.toString(ImaConstants.TS_All));
            	}
            	isClosed = true;
                ((ImaPropertiesImpl)this.props).put("isClosed", true);
                connect.consumerMap.remove(Integer.valueOf(consumerid));
                if (rcvAction != null){
                    rcvAction.stopDelivery(false);
                }
                ImaAction act = new ImaConsumerAction(ImaAction.closeConsumer, session, consumerid);
                if ((session.ackmode == Session.CLIENT_ACKNOWLEDGE) || 
                   (disableACK && !session.transacted)){
                    act.setHeaderCount(0);
                } else {
               		act.outBB = ImaUtils.putLongValue(act.outBB, lastDeliveredMessage);
                    act.setHeaderCount(1);                	
                }
                act.request(); 
                if (dest instanceof ImaTemporaryTopic) {
                    ((ImaTemporaryTopic)dest).removeConsumer(this);
                }
                if (dest instanceof ImaTemporaryQueue) {
                    ((ImaTemporaryQueue)dest).removeConsumer(this);
                }
                if (rcvAction == null)
                	receivedMessages.add(new ConsumerClosedMessage());
            }
        }    
    }

    public void markClosed() {
        synchronized (this) {
            if (!isClosed) {
                if (ImaTrace.isTraceable(6)) { 
                    ImaTrace.trace("Marking consumer closed: consumer=" + toString(ImaConstants.TS_All) + 
                            " session=" + session.toString(ImaConstants.TS_All));
                }
                isClosed = true;
                ((ImaPropertiesImpl)this.props).putInternal("isClosed", true);
                connect.consumerMap.remove(Integer.valueOf(consumerid));
                if (rcvAction != null) {
                    rcvAction.stopDelivery(true);
                } else {
                    receivedMessages.add(new ConsumerClosedMessage());
                }
                isStopped = true;

                if (dest instanceof ImaTemporaryTopic) {
                    ((ImaTemporaryTopic)dest).removeConsumer(this);
                }
                if (dest instanceof ImaTemporaryQueue) {
                    ((ImaTemporaryQueue)dest).removeConsumer(this);
                }
            }
        }    
    }
    
    /* 
     * Internal check if the session is closed.
     */
    void checkClosed() throws JMSException {
        if (isClosed) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0010", "A call to a MessageConsumer object method failed because the consumer is closed.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }

    
    /* 
     * Get the message listener.
     * @see javax.jms.MessageConsumer#getMessageListener()
     */
    public MessageListener getMessageListener() throws JMSException {
        checkClosed();
        return listener;
    }

    
    /* 
     * Get the message selector.
     * @see javax.jms.MessageConsumer#getMessageSelector()
     */
    public String getMessageSelector() throws JMSException {
        checkClosed();
        return selector;
    }
   
    
    /* 
     * Receive without a timeout.
     * We implement this with a periodic wait with timeout so that we can check if the
     * message consumer has been closed.
     * @see javax.jms.MessageConsumer#receive()
     */
    public Message receive() throws JMSException {
        return receive(0);
    }

    /*
     * Wait if the connection is stopped.  
     * This is used to implement the receive with timeout when the connection is stopped.
     */
    private synchronized boolean waitIfStopped(long endTime){
        if (!isStopped)
            return false;
        for( long currTime = System.currentTimeMillis(); ((currTime < endTime) && isStopped); currTime = System.currentTimeMillis()){
            long waitTime = endTime - currTime;
            try {
                wait(waitTime);
            } catch (InterruptedException e) {
            }
        }
        return isStopped;
    }

    /*
     * Send an automatic ACK
     */
    private final boolean sendAutoAck() throws JMSException {
        if (!disableACK && (session.ackmode == Session.AUTO_ACKNOWLEDGE)){
            JMSException jex = session.acknowledgeInternalSync(this);
            if (jex != null)
                throw jex;
            return (fullSize != 0);
        }
        return true;
    }
    
    /*
     * Internal common code for receive.
     */
    private Message receiveInternal(long endTime) throws JMSException {
        checkClosed();
        ImaMessage result = null;
		boolean sendResume = (lastDeliveredMessage != 0) ? sendAutoAck() : true;
        if(waitIfStopped(endTime))
            return null;
        while (!isClosed) {        	
        	try {
        	    if (receivedMessages.isEmpty()){
                    long currTime = System.currentTimeMillis();
                    long waitTime = endTime - currTime;
                    if (waitTime <= 0) {
                        waitTime = 0;
                    }
        	        result  = receivedMessages.poll(waitTime,TimeUnit.MILLISECONDS);
        	    } else {
        	        result  = receivedMessages.poll();
        	    }
        	    if (result == null)
        	        return null;
                if (result instanceof ConsumerClosedMessage) {
                    if (isClosed)
                        return null;
                    if(waitIfStopped(endTime))
                        return null;
                    continue;
                }
                if ((lastDeliveredMessage - result.ack_sqn) >= 0) {
                	/* Typically, this means we are in session recover and we 
                	 * need to discard the messages in the cache so that the server
                	 * can redliver them.  The discardInternal identifier makes the trace messages
                	 * much easier to understand when trouble shooting issues.
                	 */
                    cachedMessageRemoved("discardInternal", result, sendResume);
                    sendResume = true;
                    continue;
                }
                lastDeliveredMessage = result.ack_sqn;
                session.updateDeliveredInt(this);
                cachedMessageRemoved("receiveInternal", result, sendResume);
                return result;
			} catch (InterruptedException e) {
			    continue;
			}
        }
    	return null;
    }
    /* 
     * Receive with a specified timeout in milliseconds.
     * @see javax.jms.MessageConsumer#receive(long)
     */
    public Message receive(long timeout) throws JMSException {
    	long endTime = (timeout > 0) ? (System.currentTimeMillis()+timeout) : Long.MAX_VALUE;    	
    	return receiveInternal(endTime);
    }
    
    /* 
     * Receive with no wait.
     * @see javax.jms.MessageConsumer#receiveNoWait()
     */
    public Message receiveNoWait() throws JMSException {
        checkClosed();
        return receiveInternal(System.currentTimeMillis());
    }
    
    
    /* 
     * Set the message listener.
     * @see javax.jms.MessageConsumer#setMessageListener(javax.jms.MessageListener)
     */
    public void setMessageListener(MessageListener listener) throws JMSException {
        checkClosed();
        synchronized (session) {
            if (this.listener == null) {
                if (listener == null)
                    return;
                this.listener = listener;
                ImaReceiveAction rxAct = new ImaReceiveAction(this, listener);
                ImaAction act = new ImaMsgListenerAction(this, rxAct);
                act.request();
            } else {
                if (listener == null) {
                    ImaAction act = new ImaMsgListenerAction(this);
                    act.request();
                }else{
                    rcvAction = new ImaReceiveAction(this, listener);
                }
                this.listener = listener;
            }
        }
    }

    
    /*
     * Get the noLocal setting. 
     * @see javax.jms.TopicSubscriber#getNoLocal()
     */
    public boolean getNoLocal() throws JMSException {
        checkClosed();
        return nolocal;
    }

    
    /*
     * Get the topic object.
     * @see javax.jms.TopicSubscriber#getTopic()
     */
    public Topic getTopic() throws JMSException {
        checkClosed();
        return (Topic)dest;
    }


    /*
     * Get the queue object
     * @see javax.jms.QueueReceiver#getQueue()
     */
    public Queue getQueue() throws JMSException {
        checkClosed();
        return (Queue)dest;
    }
   
    /*
     * Detailed toString
     * @see java.lang.Object#toString()
     */
    public String toString() {
    	return toString(ImaConstants.TS_Common);
    }

    /*
     * Get the class name as part of detailed toString
     * @see com.ibm.ima.jms.impl.ImaReadonlyProperties#getClassName()
     */
    public String getClassName() {
    	return "ImaMessageConsumer";
    }

	/*
	 * Get info as part of detailed toString.
	 * @see com.ibm.ima.jms.impl.ImaReadonlyProperties#getInfo()
	 */
    public String getInfo() {
    	return consumerid >= 0 ? ("consumerid="+consumerid) : null;
    }

    private final void cachedMessageRemoved(String methodName, ImaMessage msg, boolean sendResume){
        mutex.lock();
        try{
            msgCount--;
            if (ImaTrace.isTraceable(9)) {
                ImaTrace.trace("Remove message from consumer client cache: method=" + methodName + " connect =" 
                		+ connect + " consumer="
                        + consumerid + " sendResume=" + sendResume + " msgCount=" + msgCount + " message="
                        + msg.toString(ImaUtils.TS_All));
            }
            if (!suspendFlags[0])
                return;
            if (msgCount == 0) {
                suspendFlags[0] = false;
                suspendFlags[1] = false;
            } else {
                if (!suspendFlags[1])
                    return;
                if (msgCount > emptySize)
                    return;
                suspendFlags[1] = false;
            }
        } finally {
            if (msgCount == 0)
                sendResume = true;
            mutex.unlock();
        }
        if (sendResume){
            if (resumeAction == null)
                resumeAction = new ImaConsumerAction(ImaAction.startConsumer, session, consumerid);
            else
                resumeAction.reset(ImaAction.startConsumer);     
            try {
                resumeAction.request(false);
                if (ImaTrace.isTraceable(8)) {
                    ImaTrace.trace("Start consumer: consumer=" + toString(ImaConstants.TS_Common));
                }
            } catch (JMSException e) {
                ImaTrace.trace(2, "Unable to send resume request to server: consumer="
                        + toString(ImaConstants.TS_All) + " ack_sqn="
                        + lastDeliveredMessage);
                ImaTrace.traceException(2, e);
                connect.raiseException(e);
            }                               
        }
    }
    
    private final void cachedMessageAdded(ImaMessage msg){
        mutex.lock();
        try{
            msgCount++;
            if (msg.suspend){
                suspendFlags[0] = true;
                suspendFlags[1] = true;
            }
            if (ImaTrace.isTraceable(9)) {
                ImaTrace.trace("Add message to consumer client cache: connect = "
                		+ connect + " consumer=" + consumerid + 
                        " msgCount=" + msgCount + " ackSqn=" + msg.ack_sqn);
            }                                   
        } finally {
            mutex.unlock();
        }
    }
    
    /*
     * Handle a message
     * @see com.ibm.ima.jms.impl.ImaConsumer#handleMessage(java.nio.ByteBuffer)
     */
	public void handleMessage(ByteBuffer bb) throws JMSException {
	    if (isClosed)
	        return;
		final ImaMessage msg = ImaMessage.makeMessage(bb, session, this);
		cachedMessageAdded(msg);
		if (rcvAction != null) {
	        deliveryThread.addTask(rcvAction, msg);
			return;
		}
		receivedMessages.add(msg);
	}

	/*
	 * Processing on asynchronous delivery
	 */
	void onAsyncDelivery(ImaMessage msg){
        boolean sendResume = true;
        if (!disableACK && (session.ackmode == Session.AUTO_ACKNOWLEDGE)){
            JMSException jex = session.acknowledgeInternalSync(this);
            if (jex == null) {
                sendResume = (fullSize != 0);
            } else {
                connect.raiseException(jex);
                sendResume = true;
            }
        }
        cachedMessageRemoved("onAsyncDelivery", msg, sendResume);
        return;
	}
	

	/*
	 * Start delivery.
	 */
	synchronized void startDelivery(){
	    if (!isStopped)
	        return;
	    isStopped = false;
        if (rcvAction != null){
            rcvAction.startDelivery();
            return;
        }
        notifyAll();
	}
	
	/*
	 * Stop delivery.
	 */
	synchronized void stopDelivery(){
	    if (isStopped || isClosed)
	        return;
	    isStopped = true;
	    if (rcvAction != null){
	        rcvAction.stopDelivery(false);
	        return;
	    }
        receivedMessages.add(new ConsumerClosedMessage());
        notifyAll();
	}
	
	/*
	 * Message to put on the client cache to indicate the close of a consumer
	 */
    static final class ConsumerClosedMessage extends ImaMessage {
    	
		public ConsumerClosedMessage() {
			super((ImaSession)null);
		}
    }

}
