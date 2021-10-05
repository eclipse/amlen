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

import java.io.Serializable;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.HashMap;
import java.util.Iterator;
import java.util.TimerTask;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.ReentrantLock;
import java.util.zip.CRC32;

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

import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.ImaSubscription;

/*
 * Implement the Session interface for the IBM MessageSight JMS client.
 */
public class ImaSession extends ImaReadonlyProperties implements Session, TopicSession, QueueSession, ImaSubscription {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    static final ImaDeliveryThread[] deliveryThreads;
    static int nextThread = 0;
    ImaConnection connect;
    int           sessid;
    final boolean transacted;
    int     ackmode;
    byte [] tempBytes;
    char [] tempChar;
    MessageListener listener;
    int     defaultPriority = 4;
    int     defaultTimeToLive;
    String  ttopicName;
    String  tqueueName;
    int     tdestCount = 0;
    AtomicBoolean                                  isClosed             = new AtomicBoolean(true);
    ConcurrentHashMap<Integer, ImaProducer> producerList         = new ConcurrentHashMap<Integer, ImaProducer>();
    ConcurrentLinkedQueue<ImaConsumer> consumerList = new ConcurrentLinkedQueue<ImaConsumer>();
    HashMap<String, Destination> replyMap = new HashMap<String, Destination>();
    ReentrantLock deliverLock = new ReentrantLock();
    int fullSize = 1000;            
    int emptySize = 20;

    Object  producer_lock = new Object();
    boolean isStopped = true;
    boolean disableACK = false;
    int     domain;
    volatile long    lastDeliveredMessage = 0;        /* Last delivered for async consumers */
    volatile long    lastAckedMessage = 0;            /* Last consumed message for async consumers */
    long [] acksqn;
    int     acksqn_count;
    private ImaSessionAction    commitAction = null;
    private ImaAckAction        asyncAckAction = null;
    private ImaAckAction        syncAckAction = null;
    final private TimerTask     timerTask;
    private ImaDeliveryThread   deliveryThread = null;
    final boolean               asyncSendAllowed;
    final boolean                                    isXA;
    private final ConcurrentLinkedQueue<ImaProducer> noDestProducers      = new ConcurrentLinkedQueue<ImaProducer>();

    /*
     * Create an IMA Session
     */
    ImaSession(ImaConnection connect, ImaProperties props, boolean transacted, int ackmode, int domain, boolean isXA)
            throws JMSException {
        super(new ImaPropertiesImpl(ImaJms.PTYPE_Connection));
        if (props != null)
            ((ImaPropertiesImpl)this.props).props.putAll(((ImaPropertiesImpl)props).props);
        this.connect = connect;
        this.transacted = transacted;
        this.ackmode = ackmode;
        this.tempChar = new char[1000];
        this.isXA = isXA;
        
        /*
         * Set initial values for defaults from the ConnectionFactory properties
         */
        if (transacted) {
            this.ackmode = SESSION_TRANSACTED;
        } else {
            if (ackmode == AUTO_ACKNOWLEDGE || ackmode == CLIENT_ACKNOWLEDGE) {         
                this.ackmode = ackmode;
            } else {
                   this.ackmode = DUPS_OK_ACKNOWLEDGE;
            }
        }
        defaultTimeToLive = this.props.getInt("DefaultTimeToLive", 0);
        if (defaultTimeToLive < 0)
            defaultTimeToLive = 0;
        disableACK = getBoolean("DisableACK", false);
        asyncSendAllowed = this.props.getBoolean("AsyncTransactionSend", this.props.getBoolean("AllowAsynchronousSend", false));
        if (connect.isTraceable(7)) {
        	StringBuffer strbuf = new StringBuffer();
            strbuf.append("Creating session:  ");
            strbuf.append("ackmode=").append(ackmode).append(" ");
            strbuf.append("transacted=").append(transacted).append(" ");
            strbuf.append("domain=").append(connect.domain).append(" ");
            strbuf.append("Properties=").append(this.props.toString());
            connect.trace(strbuf.toString());
            strbuf.setLength(0);
        }
        
        /*
         * Create the session at the server
         */
        ImaAction act = new ImaConnectionAction(ImaAction.createSession, connect.client);
        act.outBB = ImaUtils.putIntValue(act.outBB, connect.domain);    /* val0 = domain */
        act.outBB = ImaUtils.putBooleanValue(act.outBB, transacted);    /* val1 = transacted */
        act.outBB = ImaUtils.putIntValue(act.outBB, this.ackmode);      /* val2 = ackmode  */
        act.setHeaderCount(3);
        act.outBB = ImaUtils.putMapValue(act.outBB, this);
        act.request();
        if (act.rc == 0)
            sessid = ImaUtils.getInteger(act.inBB);
        else{
            if (connect.isTraceable(2)) {
                connect.trace("Failed to create session: rc=" + act.rc + " session=" +
                		toString(ImaConstants.TS_All));
            }
            ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0204","A request to create a session failed with MessageSight return code = {0}.", act.rc);
            connect.traceException(2, jex);
            throw jex;
        }
        /*
         * Make the transaction
         */
        if (transacted && !isXA) {
            act = new ImaSessionAction(ImaAction.createTransaction, this);
            act.request();
        }
        
        /*
         * Set actual properties
         */
        this.props.remove("isStopped");
        this.props.put("DefaultTimeToLive", defaultTimeToLive);
        putInternal("Connection", connect);
        int ackInterval = this.props.getInt("AckInterval", 100);
        if (this.ackmode == DUPS_OK_ACKNOWLEDGE) {
            timerTask = new TimerTask() {                
                @Override
                public void run() {
                    try {
                        acknowledge(true);
                    } catch (JMSException e) {
                    }
                }
            };
            ImaConnection.jmsClientTimer.schedule(timerTask, 100, ackInterval);
        } else {
            timerTask = null;
        }
        fullSize = getInt("ClientMessageCache", fullSize);
        if (fullSize < 0)
        	fullSize = 1;
        emptySize = fullSize / 4;
        isClosed.set(false);
    }

    /*
     * Create a static array of threads
     */
    static {
        int m = 2;
        try {
            String s = null; 
            s = (String) AccessController.doPrivileged(new PrivilegedAction<Object>() {
                public Object run() {
                    return System.getenv("IMADeliveryThreads");
                }
            }); 
            if ((s != null) && (s.length() > 0))
                m = Integer.parseInt(s);
        } catch (Throwable e) {
            m = 2;
        }
        if (m < 1)
            m = 1;
        deliveryThreads = new ImaDeliveryThread[m];
    }
    
    /*
     * Close the session.
     * Notify the connection that the session is closing.
     * @see javax.jms.Session#close()
     */
    public void close() throws JMSException {
        ImaConnectionFactory.checkAllowed(connect);
        if (isClosed.compareAndSet(false, true)) {
            connect.closeSession(this);
            closeInternal();
        }
    }
    
    
    /*
     * Internal close function which does not notify the connection.
     * This is called from the connection to do the close.
     */
    synchronized void closeInternal() throws JMSException {
    	JMSException firstex = null; /* This is the first exception that is handled while assuring that 
    	                              * remaining consumers/producers are closed. Rethrow this exception
    	                              * at the end of the method if no other unhandled exceptions are thrown
    	                              * in the meantime.
    	                              */
        this.props.put("isClosed", true);
        if (timerTask != null) {
            timerTask.cancel();
        }

        /*
         * Close all consumers for this session
         */
        for (ImaConsumer cons = consumerList.poll(); cons != null;  cons = consumerList.poll()) {
        	try {
                cons.closeInternal();
        	} catch (JMSException jex) {
        		if (firstex == null)
        			firstex = jex;
                if (connect.isTraceable(6)) {
                    connect.trace("Failure while closing consumer.");
                    connect.traceException(jex);
                }
        	}
        }
        
        /*
         * Close all producers for this session
         */
        Iterator<ImaProducer> itp = producerList.values().iterator();
        while (itp.hasNext()) {
            ImaProducer prod = itp.next();
            itp.remove();
            try {
                prod.closeInternal();
        	} catch (JMSException jex) {
        		if (firstex == null)
        			firstex = jex;
                if (connect.isTraceable(6)) {
                    connect.trace("Failure while closing producer.");
                    connect.traceException(jex);
                }
        	}
        }

        for (ImaProducer prod = noDestProducers.poll(); prod != null; prod = noDestProducers.poll()) {
        	try {
                prod.closeInternal();
        	} catch (JMSException jex) {
        		if (firstex == null)
        			firstex = jex;
                if (connect.isTraceable(6)) {
                    connect.trace("Failure while closing producer.");
                    connect.traceException(jex);
                }
        	}
        }

        /*              
         * Close the transaction
         */
        ImaAction act;
        /*
         * Close the session
         */
        act = new ImaSessionAction(ImaAction.closeSession, this);
        act.request();
        
		if (firstex != null) {
			if (connect.isTraceable(1)) {
				connect.trace("One or more errors occurred while closing consumers and producers.  Rethrowing the first of these exceptions.");
			}
			throw firstex;
		}
		
        if (connect.isTraceable(6)) {
            connect.trace("Session is closed: session=" + toString(ImaConstants.TS_All));
        }
    }
    void closeExternal() throws JMSException {
        if (isClosed.compareAndSet(false, true))
            closeInternal();
    }
    
    void markClosed() {
        if (isClosed.compareAndSet(false, true)) {
            ((ImaPropertiesImpl)this.props).putInternal("isClosed", true);
            if (timerTask != null) {
                timerTask.cancel();
            }
            stopInternal();

            /*
             * Close all consumers for this session
             */
            for (ImaConsumer cons = consumerList.poll(); cons != null;  cons = consumerList.poll()) {
                cons.markClosed();
                cons = null;
            }
            
            /*
             * Close all producers for this session
             */
            Iterator<ImaProducer> itp = producerList.values().iterator();
            while (itp.hasNext()) {
                ImaProducer prod = itp.next();
                itp.remove();
                prod.markClosed();
                prod = null;
            }
            
            if (connect.isTraceable(6)) {
                connect.trace("Session is marked closed: session=" + toString(ImaConstants.TS_All));
            }
        }
    }
    
    /*
     * Internal call to set stopped flag
     */
    void stopInternal() {
    	deliverLock.lock();
    	try {
            isStopped = true;
        } finally {
        	deliverLock.unlock();
        }
    }

    /*
     * Internal call to reset stopped flag
     */
    void startInternal() {
        isStopped = false;
    }

    
    /* 
     * Check if the session is closed
     */
    void checkClosed() throws JMSException {
        if (isClosed.get()) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0009", "A call to a Session object method failed because the session is closed.");
            connect.traceException(2, jex);
            throw jex;
        }
    }
    
    /*
     * Check the destination
     */
    public void checkDestination(Destination dest) throws JMSException {
        if (dest == null) {
        	ImaInvalidDestinationException jex = new ImaInvalidDestinationException("CWLNC0012", "An operation that requires a non-null destination failed because the destination object is null.");
            connect.traceException(2, jex);
            throw jex;
        }
        if (!(dest instanceof ImaDestination)) {
        	ImaInvalidDestinationException jex = new ImaInvalidDestinationException("CWLNC0013", "An operation that requires an IBM MessageSight destination failed because a non-MessageSight destination ({0}) was used.", dest);
            connect.traceException(2, jex);
            throw jex;
        }
        String destname = ((ImaDestination)dest).getName();
        if (destname == null) {
        	ImaInvalidDestinationException jex = new ImaInvalidDestinationException("CWLNC0007", "An operation that requires the destination name to be set failed because the destination name is not set.");
            connect.traceException(2, jex);
            throw jex;
        }
    }

    /*
     * Write a position for each of the non-message listener consumers
     */
    final boolean writeAckSqns(String caller, ImaSessionAction act, ImaMessageConsumer consumer) {
    	boolean result = false;
    	StringBuffer sb = null;
    	deliverLock.lock();
    	try{
    		if (((lastDeliveredMessage - lastAckedMessage) > 0) || (acksqn_count > 0)) {
    	    	sb = (connect.isTraceable(9)) ? new StringBuffer(256) : null;
        		if (sb != null) {
        			sb.append(caller).append(": session=").append(toString(ImaConstants.TS_Common)).
        			append(" lastDeliveredMessage=").append(lastDeliveredMessage).
        			append(" lastAckedMessage=").append(lastAckedMessage).
        			append(" ackSqns=");
        			if (consumer != null)
        				sb.append("consumer=").append(consumer.consumerid);
        		}
                lastAckedMessage = lastDeliveredMessage;
                act.outBB = ImaUtils.putIntValue(act.outBB,acksqn_count);
                act.outBB = ImaUtils.putLongValue(act.outBB,lastAckedMessage);
                if (consumer == null)
                    act.incHeaderCount(2);
                else {
                    act.outBB = ImaUtils.putIntValue(act.outBB,consumer.consumerid);
                    act.incHeaderCount(3);
                }
                if (acksqn_count > 0){
                    act.outBB = ImaUtils.putNullValue(act.outBB);          /* Properties */                
                    act.outBB = ImaUtils.putNullValue(act.outBB);          /* Body */                
                    for (int i=0; i<acksqn_count; i+=2) {
                    	act.outBB = ImaUtils.putIntValue(act.outBB,(int)acksqn[i]);
                    	act.outBB = ImaUtils.putLongValue(act.outBB,acksqn[i+1]);
                        if (sb != null)
                        	sb.append(" (").append(acksqn[i]).append(", ").
                        	append(acksqn[i+1]).append(") ");
                    }
                    acksqn_count = 0;
                }
                result = true;
    		} else {
                // act.setHeaderCount(0);
    		}
    	}finally {
    		deliverLock.unlock();
    	}
    	if (sb != null) 
    		connect.trace(sb.toString());
    	return result;
    }
    
    /*
     * Common implementation 
     */
    void acknowledgeInternalAsync() throws JMSException {
    	if (disableACK || isClosed.get())
    		return;
        if (asyncAckAction == null)
            asyncAckAction = new ImaAckAction(ImaAction.ack, this);
        else
            asyncAckAction.reset();
        if (writeAckSqns("acknowledgeInternalAsync", asyncAckAction, null))
        	asyncAckAction.request(false);
    }
    
    
    /*
     * Send a synchronous acknowledge.  
     * This is used for AUTO and CLIENT acks and during session close.
     */
    JMSException acknowledgeInternalSync(ImaMessageConsumer consumer) {
    	if (disableACK || isClosed.get())
            return null;
        if (syncAckAction == null)
            syncAckAction = new ImaAckAction(ImaAction.ackSync, this);
        else
            syncAckAction.reset();
        if (writeAckSqns("acknowledgeInternalSync", syncAckAction, consumer)) {
        	try {
                syncAckAction.request(true);
            } catch (JMSException e) {
                long sqn = (consumer == null) ? lastDeliveredMessage : consumer.lastDeliveredMessage;
                connect.trace(2, "Unable to send ack to server: session="
                        + toString(ImaConstants.TS_All) + " ack_sqn=" + sqn);
                connect.traceException(2, e);
                return e;
            }
        }
        return null;
    }
    
    
    /*
     * Implement the external acknowledge.
     * 
     * The JMS acknowledge is a method on message, but it is really a method
     * implemented against a session.  
     */
    void acknowledge(boolean async) throws JMSException{
    	checkClosed();
        if (async)
        	acknowledgeInternalAsync();
        else {
        	if (ackmode != CLIENT_ACKNOWLEDGE)
        		return;
            JMSException jex = acknowledgeInternalSync(null);
            if (jex != null)
                throw jex;
        }
    }
        
    /*
     * Update the delivery position (internal)
     */
    void updateDeliveredInt(ImaMessageConsumer consumer){
    	boolean done = false;
    	deliverLock.lock();
    	try{
        	if (acksqn == null)
        		acksqn = new long[10];
        	for (int i=0; i<acksqn_count; i+=2) {
        		if ((int)acksqn[i] == consumer.consumerid) {
        			done = true;
        			acksqn[i+1] = consumer.lastDeliveredMessage;
        			break;
        		}
        	}	
        	if (!done) {
        		if (acksqn_count == acksqn.length) {
        			long [] new_acksqn = new long[acksqn.length*4];
        			System.arraycopy(acksqn, 0, new_acksqn, 0, acksqn_count);
        			acksqn = new_acksqn;
        		}
        		acksqn[acksqn_count++] = consumer.consumerid;
        		acksqn[acksqn_count++] = consumer.lastDeliveredMessage;
        	}
    	} finally {
    		deliverLock.unlock();
    	}
    }
        
    /*
     * @see javax.jms.Session#commit()
     */
    public void commit() throws JMSException {
        checkClosed();
        connect.checkClosed();
        if (!transacted) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0001", "A to call commit() or rollback() on a Session object failed because the session was not configured as a transacted session.");
            connect.traceException(2, jex);
            throw jex;
        }
        if (commitAction == null)
            commitAction = new ImaSessionAction(ImaAction.commitSession, this, 256);
        else
            commitAction.reset(ImaAction.commitSession);
        writeAckSqns("commit", commitAction,null);
       	commitAction.request(true);
       	if (commitAction.rc != 0) {
       		ImaTransactionRolledBackException jex = new ImaTransactionRolledBackException(commitAction.rc);
       		connect.traceException(2, jex);
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
    public QueueBrowser createBrowser(Queue queue, String selector) throws JMSException {
        checkClosed();
        checkDestination(queue);
        if ("topic".equals(get("ObjectType"))) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0017", "A queue method ({0}) failed because it was called on a TopicSession object.", "createBrowser");
            connect.traceException(2, jex);
            throw jex;
        }

        return new ImaQueueBrowser(this, (ImaQueue)queue, selector);
    }

    
    /*
     * @see javax.jms.Session#createBytesMessage()
     */
    public BytesMessage createBytesMessage() throws JMSException {
        checkClosed();
        return (BytesMessage)new ImaBytesMessage(this);
    }

    
    /*
     * @see javax.jms.Session#createConsumer(javax.jms.Destination)
     */
    public MessageConsumer createConsumer(Destination dest) throws JMSException {
    	return createConsumer(dest, null, false, ImaJms.Common);
    }
    

    /*
     * @see javax.jms.Session#createConsumer(javax.jms.Destination, java.lang.String)
     */
    public MessageConsumer createConsumer(Destination dest, String selector) throws JMSException {
        return createConsumer(dest, selector, false, ImaJms.Common);
    }

    /*
     * @see javax.jms.Session#createConsumer(javax.jms.Destination, java.lang.String, boolean)
     */
    public MessageConsumer createConsumer(Destination dest, String selector, boolean nolocal) throws JMSException {
    	return createConsumer(dest, selector, nolocal, ImaJms.Common);
    }
    
    /*
     * @see javax.jms.Session#createConsumer(javax.jms.Destination, java.lang.String, boolean)
     */
     MessageConsumer createConsumer(Destination dest, String selector, boolean nolocal, int domain) throws JMSException {
        checkClosed();
        checkDestination(dest);
        
        if (selector != null && selector.length()== 0)
            selector = null;

        if (domain == 0)
            domain = (dest instanceof Topic) ? ImaJms.Topic : ImaJms.Queue;
        ImaMessageConsumer consumer = new ImaMessageConsumer(this, dest, selector, nolocal, false, null, domain);

        ImaAction act = new ImaCreateConsumerAction(ImaAction.createConsumer, this,consumer);
        act.outBB = ImaUtils.putByteValue(act.outBB, (byte)domain);  /* val0 = domain */
        act.outBB = ImaUtils.putBooleanValue(act.outBB, nolocal);     /* val1 = nolocal */
        act.outBB = ImaUtils.putStringValue(act.outBB, selector);     /* val2 = selector */
        act.setHeaderCount(3);
        act.outBB = ImaUtils.putMapValue(act.outBB,consumer);
        
        synchronized (this) {
            checkClosed();
            act.request();
            if (act.rc != 0) {
                switch (act.rc) {
           		case ImaReturnCode.IMARC_NotAuthorized:
           		case ImaReturnCode.IMARC_NotAuthenticated: 
                    if (connect.isTraceable(2)) {
                        connect.trace("Failed to create consumer due to authorization failure: consumer=" + 
                        		consumer.toString(ImaConstants.TS_All));
                    }
                    ImaJmsSecurityException jex = new ImaJmsSecurityException("CWLNC0207", "A client request to MessageSight failed due to an authorization failure.");
                    connect.traceException(2, jex);
                    throw jex;
                    
           		case ImaReturnCode.IMARC_DestNotValid: 
                    if (connect.isTraceable(2)) {
                        connect.trace("Failed to create consumer: reason=\"destination not valid\" consumer=" + 
                        		consumer.toString(ImaConstants.TS_All));
                    }
                    ImaInvalidDestinationException jex3 = new ImaInvalidDestinationException("CWLNC0217", "A request to create a producer or a consumer failed because the destination {0} is not valid.", 
                    		((ImaProperties)dest).get("Name").toString());
                    connect.traceException(2, jex3);
                    throw jex3;
                    
                case ImaReturnCode.IMARC_BadSysTopic: 
                    if (connect.isTraceable(2)) {
                        connect.trace("Failed to create consumer: reason=\"The topic is a system topic\" consumer=" + 
                                consumer.toString(ImaConstants.TS_All));
                    }
                    ImaJmsExceptionImpl jex4 = new ImaJmsExceptionImpl("CWLNC0225", "A request to create a message producer or a durable subscription, or a request to publish a message failed because the destination {0} is a system topic.", 
                            ((ImaProperties)dest).get("Name").toString());
                    connect.traceException(2, jex4);
                    throw jex4;
                    
                case ImaReturnCode.IMARC_TooManyProdCons: 
                    if (connect.isTraceable(2)) {
                        connect.trace("Failed to create consumer: reason=\"Too many consuumers\" consumer=" + 
                                consumer.toString(ImaConstants.TS_All));
                    }
                    ImaJmsExceptionImpl jex5 = new ImaJmsExceptionImpl("CWLNC0232", "A call by client {1} to create consumer {0} failed because there are too many consumers in this connection.", 
                            ((ImaProperties)dest).get("Name").toString(), connect.clientid);
                    connect.traceException(2, jex5);
                    throw jex5;

                default:
                    if (connect.isTraceable(2)) {
                        connect.trace("Failed to create consumer: rc=" + act.rc +
                        		" consumer="+consumer.toString(ImaConstants.TS_All));
                    }
                    ImaJmsExceptionImpl jex2 = new ImaJmsExceptionImpl("CWLNC0209", "Client {2} failed to create a consumer for topic or subscription {1} with MessageSight return code = {0}.", 
                            act.rc, ((ImaProperties)dest).get("Name").toString(), connect.clientid);
                    connect.traceException(2, jex2);
                    throw jex2;

                }
            }
            ImaAction saction = new ImaSessionAction(ImaAction.resumeSession, this);
            saction.request(false);
        }
        
        if (connect.isTraceable(6)) {
            connect.trace("Consumer is created: consumer=" + consumer.toString(ImaConstants.TS_All));
        }
        return (MessageConsumer) consumer;
    }

    
    /*
     * @see javax.jms.Session#createDurableSubscriber(javax.jms.Topic, java.lang.String)
     */
    public TopicSubscriber createDurableSubscriber(Topic topic, String name) throws JMSException {
    	return (TopicSubscriber)createSubscriptionAndConsumer(topic, name, null, false, ImaJms.Topic, "createDurableSubscriber");
    } 
   

    /*
     * @see javax.jms.Session#createDurableSubscriber(javax.jms.Topic, java.lang.String, java.lang.String, boolean)
     */
    public TopicSubscriber createDurableSubscriber(Topic topic, String name, String selector, boolean nolocal)
                    throws JMSException {
        return (TopicSubscriber)createSubscriptionAndConsumer(topic, name, selector, nolocal, ImaJms.Topic, "createDurableSubscriber");
    }
    
    /*
     * Additional JMS 2.0 durable subscription methods 
     */
    
    /*
     * @see javax.jms.Session#createDurableConsumer(javax.jms.Topic, java.lang.String)
     */
    public MessageConsumer createDurableConsumer(Topic topic, String name) throws JMSException {
        return createSubscriptionAndConsumer(topic, name, null, false, ImaJms.Topic_False, "createDurableConsumer");
    }


    /*
     * @see javax.jms.Session#createDurableConsumer(javax.jms.Topic, java.lang.String, java.lang.String, boolean)
     */
    public MessageConsumer createDurableConsumer(Topic topic, String name, String selector, boolean nolocal)
            throws JMSException {
        return createSubscriptionAndConsumer(topic, name, selector, nolocal, ImaJms.Topic_False, "createDurableConsumer");
    }


    /*
     * @see javax.jms.Session#createSharedConsumer(javax.jms.Topic, java.lang.String)
     */
    public MessageConsumer createSharedConsumer(Topic topic, String name) throws JMSException {
        return createSubscriptionAndConsumer(topic, name, null, false, ImaJms.Topic_SharedNonDurable, "createSharedConsumer");
    }


    /*
     * @see javax.jms.Session#createSharedConsumer(javax.jms.Topic, java.lang.String, java.lang.String)
     */
    public MessageConsumer createSharedConsumer(Topic topic, String name, String selector) throws JMSException {
        return createSubscriptionAndConsumer(topic, name, selector, false, ImaJms.Topic_SharedNonDurable, "createSharedConsumer");
    }


    /*
     * @see javax.jms.Session#createSharedDurableConsumer(javax.jms.Topic, java.lang.String)
     */
    public MessageConsumer createSharedDurableConsumer(Topic topic, String name) throws JMSException {
        return createSubscriptionAndConsumer(topic, name, null, false, ImaJms.Topic_Shared, "createSharedDurableConsumer");
    }


    /*
     * @see javax.jms.Session#createSharedDurableConsumer(javax.jms.Topic, java.lang.String, java.lang.String)
     */
    public MessageConsumer createSharedDurableConsumer(Topic topic, String name, String selector) throws JMSException {
        return createSubscriptionAndConsumer(topic, name, selector, false, ImaJms.Topic_Shared, "createSharedDurableConsumer");
    }
    
    /*
     * Common implementation for create durables
     */
    public MessageConsumer createSubscriptionAndConsumer(Topic topic, String name, String selector, boolean nolocal, int domain, String caller)
                        throws JMSException {
        checkClosed();
        checkDestination(topic);
        
        /* A durable subscription cannot be made on a temporary topic */
        if (topic instanceof ImaTemporaryTopic) {
        	ImaInvalidDestinationException jex = new ImaInvalidDestinationException("CWLNC0006", 
                    "A method called to create a durable or shared subscription failed because it specified a temporary topic.");
            connect.traceException(2, jex);
            throw jex;
        }
        
        if (!ImaUtils.isValidUTF16(name)) {
        	ImaInvalidDestinationException jex = new ImaInvalidDestinationException("CWLNC0002", "A method called to create a shared or durable consumer failed because the subscription name is not a valid UTF-16 string.");
            connect.traceException(2, jex);
            throw jex;
        }
        if ("queue".equals(get("ObjectType"))) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0018", "A topic method ({0}) failed because it was called on a QueueSession object.", caller);
            connect.traceException(2, jex);
            throw jex;
        }
        
        ImaMessageConsumer consumer = new ImaMessageConsumer(this, (Destination)topic, selector, nolocal, true, name, domain);
        if (selector != null && selector.length()== 0)
            selector = null;
        ((ImaPropertiesImpl)consumer.props).put("ObjectType", "topic");
        ImaAction act = new ImaCreateConsumerAction(ImaAction.createDurable, this,consumer);
        act.outBB = ImaUtils.putStringValue(act.outBB, name);      /* val0 = name     */ 
        act.outBB = ImaUtils.putStringValue(act.outBB, selector);  /* val1 = selector */
        act.setHeaderCount(2);
        act.outBB = ImaUtils.putMapValue(act.outBB, consumer); 

        synchronized (this) {
            checkClosed();
            act.request();
            
            ImaAction saction = new ImaSessionAction(ImaAction.resumeSession, this);
            saction.request(false);
            
            if (act.rc != 0) {
                String cid = connect.clientid;
                if (connect.generated_clientid || domain >= ImaJms.Topic_GlobalShared) {
                    cid = consumer.shared==2 ? "__SharedND" : "__Shared";
                }
                switch (act.rc) {
           		case ImaReturnCode.IMARC_NotAuthorized:
           		case ImaReturnCode.IMARC_NotAuthenticated: 
                    if (connect.isTraceable(2)) {
                    	connect.trace("Failed to create consumer: reason=Authorization failed. name=" + name + 
                    	        " session=" + toString(ImaConstants.TS_All));

                    }
                    ImaJmsSecurityException jex = new ImaJmsSecurityException("CWLNC0207", "A client request to MessageSight failed due to an authorization failure.");
                    connect.traceException(2, jex);
                    throw jex;
                    
                case ImaReturnCode.IMARC_BadSysTopic: 
                    if (connect.isTraceable(2)) {
                        connect.trace("Failed to create consumer: reason=The topic is a system topic. name=" + name + 
                                 " session=" + toString(ImaConstants.TS_All));
                    }
                    ImaJmsExceptionImpl jex3 = new ImaJmsExceptionImpl("CWLNC0225", "A request to create a message producer or a durable subscription, or a request to publish a message failed because the destination {0} is a system topic.",
                            topic.getTopicName());
                    connect.traceException(2, jex3);
                    throw jex3;
                    

                case ImaReturnCode.IMARC_ExistingSubscription: 
                    if (connect.isTraceable(2)) {
                        connect.trace("Failed to create consumer: reason=The subscription cannot be modified because the subscription is in use." +
                                " name=" + name + " session=" + toString(ImaConstants.TS_All));
                    }
                    ImaIllegalStateException jex4 = new ImaIllegalStateException("CWLNC0229", 
                            "A call by client {1} to create a consumer for existing subscription {0} failed because the request attempted to modify the subscription settings when there are active consumers for that subscription.",
                            name, cid);
                    connect.traceException(2, jex4);
                    throw jex4;
                    
                case ImaReturnCode.IMARC_DestinationInUse: 
                    if (connect.isTraceable(2)) {
                        connect.trace("Failed to create consumer: reason=An active non-shared durable consumer already exists." +
                                " name=" + name + " session=" + toString(ImaConstants.TS_All));
                    }
                    ImaIllegalStateException jex6 = new ImaIllegalStateException("CWLNC0021", 
                            "A request to create a non-shared durable message consumer failed because an active non-shared durable consumer already exists: Name={0} ClientID={1}.",
                            name, cid);
                    connect.traceException(2, jex6);
                    throw jex6;
                    
                case ImaReturnCode.IMARC_ShareMismatch: 
                    if (connect.isTraceable(2)) {
                        connect.trace("Failed to create consumer: reason=Share mistmach name=" + name + 
                                " session=" + toString(ImaConstants.TS_All));
                    }
                    ImaJmsExceptionImpl jex5 = new ImaJmsExceptionImpl("CWLNC0228", "A call by client {1} to create a consumer for existing subscription {0} failed because the request attempted to modify the settings of a shared subscription, or it attempted to modify a non-shared subscription to a shared subscription.",
                            name, cid);
                    connect.traceException(2, jex5);
                    throw jex5;

                default:
                    if (connect.isTraceable(2)) {
                        connect.trace("Failed to create consumer: rc=" + act.rc + 
                            " session=" + toString(ImaConstants.TS_All));
                    }
                    ImaJmsExceptionImpl jex2 = new ImaJmsExceptionImpl("CWLNC0209",
                            "Client {2} failed to create a consumer for topic or subscription {1} with MessageSight return code = {0}.", act.rc, name, cid);
                    connect.traceException(2, jex2);
                    throw jex2;

                }
            }
        }
        
        if (connect.isTraceable(6)) {
        	connect.trace("Consumer created: consumer=" + consumer.toString(ImaConstants.TS_All));
        }
        if (consumer.shared == ImaJms.Topic_GlobalNoConsumer)
            consumer = null;
        return (MessageConsumer) consumer;
    }
    

    /*
     * @see javax.jms.Session#createMapMessage()
     */
    public MapMessage createMapMessage() throws JMSException {
        checkClosed();
        return (MapMessage) new ImaMapMessage(this);
    }

    
    /*
     * @see javax.jms.Session#createMessage()
     */
    public Message createMessage() throws JMSException {
        checkClosed();
        return (Message) new ImaMessage(this);
    }

    
    /*
     * @see javax.jms.Session#createObjectMessage()
     */
    public ObjectMessage createObjectMessage() throws JMSException {
        checkClosed();
        return (ObjectMessage) new ImaObjectMessage(this);
    }

    
    /*
     * @see javax.jms.Session#createObjectMessage(java.io.Serializable)
     */
    public ObjectMessage createObjectMessage(Serializable obj) throws JMSException {
        checkClosed();
        ImaObjectMessage omsg = new ImaObjectMessage(this);
        omsg.setObject(obj);
        return omsg;
    }

    
    /*
     * @see javax.jms.Session#createProducer(javax.jms.Destination)
     */
    public MessageProducer createProducer(Destination dest) throws JMSException {
        int domain = ImaJms.Common;
        if (dest != null) {
            domain = (dest instanceof Topic) ? ImaJms.Topic : ImaJms.Queue;
        }
        return createProducer(dest, domain);
    }

    /*
     * Common function to create a producer from destination and domain
     */
    private MessageProducer createProducer(Destination dest, int domain) throws JMSException {
        checkClosed();
        if (dest != null) {
            return new ImaMessageProducer(this, dest, domain);
        }
        ImaMessageProducer prod = new ImaMessageProducer(this, null, domain);
        noDestProducers.add(prod);
        return prod;
    }

    
    /*
     * @see javax.jms.Session#createQueue(java.lang.String)
     */
    public Queue createQueue(String name) throws JMSException {
        checkClosed();
        if ("topic".equals(get("ObjectType"))) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0017", "A queue method ({0}) failed because it was called on a TopicSession object.", "createBrowser");
            connect.traceException(2, jex);
            throw jex;
        }
        ImaQueue queue = new ImaQueue(name);
        
        if (connect.isTraceable(7)) {
            connect.trace("Queue created: queue=" + queue.toString(ImaConstants.TS_All));
        }
        return (Queue)queue;
    }

    
    /*
     * @see javax.jms.Session#createStreamMessage()
     */
    public StreamMessage createStreamMessage() throws JMSException {
        checkClosed();
        return (StreamMessage) new ImaStreamMessage(this);
    }

    
    /*
     * Create a temporary queue.
     * @see javax.jms.Session#createTemporaryQueue()
     */
    public TemporaryQueue createTemporaryQueue() throws JMSException {
        checkClosed();
        if ("topic".equals(get("ObjectType"))) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0017", "A queue method ({0}) failed because it was called on a TopicSession object.", "createTemporaryQueue");
            connect.traceException(2, jex);
            throw jex;
        }
        /*
         * The first time we create a temporary destination on this connection, 
         * Find the property which defines the base of the configuration,
         * and create a unique identifier for this session.
         */
        if (tqueueName == null) {
            String tdest = props.getString("TemporaryQueue");
            if (tdest == null) 
                tdest = "_TQ/$CLIENTID/";
            int cid = tdest.indexOf("$CLIENTID");
            if (cid >= 0)
            	tdest = tdest.substring(0, cid) + connect.clientid + tdest.substring(cid+9);
            tqueueName = tdest + uniqueString(); 
        }    
        
        /*
         * Update the sequence number for this
         */
        String thiscount = ""+(tdestCount++);
        String thisdest = tqueueName + "00000".substring(thiscount.length())+thiscount;
        ImaTemporaryQueue queue;
        try {
            queue = new ImaTemporaryQueue(thisdest, connect);
        } catch (Exception ex) {
        	if (ex instanceof JMSException)
        		throw (JMSException)ex;
        	ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0003", ex, "A method called to create a temporary topic or temporary queue failed due to an exception.");
            connect.traceException(2, jex);
            throw jex;
        }
        if (connect.isTraceable(5)) {
            connect.trace("Create TemporaryQueue: " + thisdest);
        }
        return (TemporaryQueue)queue;
    }

    
    /*
     * Create a temporary topic.
     * @see javax.jms.Session#createTemporaryTopic()
     */
    public TemporaryTopic createTemporaryTopic() throws JMSException {
        checkClosed();
        if ("queue".equals(get("ObjectType"))) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0018", "A topic method ({0}) failed because it was called on a QueueSession object.", "createTemporaryTopic");
            connect.traceException(2, jex);
            throw jex;
        }
        /*
         * The first time we create a temporary destination on this connection, 
         * Find the property which defines the base of the configuration,
         * and create a unique identifier for this session.
         */
        if (ttopicName == null) {
            String tdest = props.getString("TemporaryTopic");
            if (tdest == null) 
                tdest = "_TT/$CLIENTID/";
            int cid = tdest.indexOf("$CLIENTID");
            if (cid >= 0)
            	tdest = tdest.substring(0, cid) + connect.clientid + tdest.substring(cid+9);
            ttopicName = tdest + uniqueString(); 
        }    
        
        /*
         * Update the sequence number for this
         */
        String thiscount = ""+(tdestCount++);
        String thisdest = ttopicName + "00000".substring(thiscount.length())+thiscount;
        ImaTemporaryTopic topic;
        try {
            topic = new ImaTemporaryTopic(thisdest, connect);
        } catch (Exception ex) {
        	if (ex instanceof JMSException)
        		throw (JMSException)ex;
        	ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0003", ex, "A method called to create a temporary topic or temporary queue failed due to an exception.");
            connect.traceException(2, jex);
            throw jex;
        }
        if (connect.isTraceable(5)) {
            connect.trace("Create TemporaryTopic: " + thisdest);
        }
        return (TemporaryTopic)topic;
    }

    
    /*
     * Make a unique string ID for this client instance
     */
    String uniqueString() {
        CRC32 crc = new CRC32();
        crc.update(connect.clientid.getBytes());
        long now = System.currentTimeMillis();
        crc.update((int)((now/77)&0xff));
        crc.update((int)((now/79731)&0xff));
        long crcval = crc.getValue();
        char [] uniqueCh = new char[5];
        for (int i=0; i<5; i++) {
            uniqueCh[i] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz".charAt((int)(crcval%62));
            crcval /= 62;
        }
        return new String(uniqueCh);
    }

    
    /*
     * Create a text message with no text.
     * @see javax.jms.Session#createTextMessage()
     */
    public TextMessage createTextMessage() throws JMSException {
        checkClosed();
        return (TextMessage) new ImaTextMessage(this, null);
    }

    
    /*
     * Create a text messge.
     * @see javax.jms.Session#createTextMessage(java.lang.String)
     */
    public TextMessage createTextMessage(String text) throws JMSException {
        checkClosed();
        return (TextMessage) new ImaTextMessage(this, text);
    }

    
    /*
     * Create a topic by name.
     * @see javax.jms.Session#createTopic(java.lang.String)
     */
    public Topic createTopic(String name) throws JMSException {
        checkClosed();
        if ("queue".equals(get("ObjectType"))) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0018", "A topic method ({0}) failed because it was called on a QueueSession object.", "createTopic");
            connect.traceException(2, jex);
            throw jex;
        }
        ImaTopic topic = new ImaTopic(name);
        if (connect.isTraceable(7)) {
            connect.trace("Topic created: topic=" + topic.toString(ImaConstants.TS_All));
        }
        return (Topic)topic;
    }
    
    
    /*
     * Get the acknowledge mode.
     * @see javax.jms.Session#getAcknowledgeMode()
     */
    public int getAcknowledgeMode() throws JMSException {
        checkClosed();
        return transacted ? SESSION_TRANSACTED : this.ackmode;
    }

    
    /*
     * Get the message listener.
     * @see javax.jms.Session#getMessageListener()
     */
    public MessageListener getMessageListener() throws JMSException {
        checkClosed();
        return listener;
    }

    
    /*
     * Get whether the session is transacted.
     * @see javax.jms.Session#getTransacted()
     */
    public boolean getTransacted() throws JMSException {
        checkClosed();
        return this.transacted;
    }


    final void setLastDeliveredSqn(long sqn){
        lastDeliveredMessage = sqn;
        lastAckedMessage = sqn;
        Iterator<ImaConsumer> it = consumerList.iterator();            
        while (it.hasNext()) {
            ImaConsumer consumer = it.next();
            if ((consumer instanceof ImaMessageConsumer) && !((ImaMessageConsumer)consumer).disableACK) {
                ImaMessageConsumer imc = (ImaMessageConsumer) consumer;
                imc.lastDeliveredMessage = sqn;
            }
        }        
    }
    /*
     * Recover the session
     * @see javax.jms.Session#recover()
     */
    public void recover() throws JMSException {
        checkClosed();
        if (transacted) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0019", "A call to recover() failed because the Session object is configured as a transacted session.");
            connect.traceException(2, jex);
            throw jex;
        }
        
    	/*
    	 * Note about disableACK.  When recover is called on a disableACK session, the request must still be
    	 * sent to the server because individual consumers in the session may be associated with destinations
    	 * where disableACK has been set to false.
    	 */
        
        if (ackmode == AUTO_ACKNOWLEDGE) {
        	/* Check if there is an unacknowledged message from the last receive call */
        	ImaAckAction act = null;
        	deliverLock.lock();
        	try{
	        	if (acksqn_count == 2) {/* There should not be more than 1 unacknowledged message */
	        		act = new ImaAckAction(ImaAction.ack, this);
	                act.outBB = ImaUtils.putIntValue(act.outBB,acksqn_count);
	                act.outBB = ImaUtils.putLongValue(act.outBB,0);
	                act.setHeaderCount(2);
	                act.outBB = ImaUtils.putNullValue(act.outBB);          /* Properties */                
	                act.outBB = ImaUtils.putNullValue(act.outBB);          /* Body */                
	            	act.outBB = ImaUtils.putIntValue(act.outBB,(int)acksqn[0]);
	            	act.outBB = ImaUtils.putLongValue(act.outBB,acksqn[1]);
	            	acksqn_count = 0;
                    if (connect.isTraceable(7)) {
                        connect.trace(7, "Recover session: session=" + toString(ImaConstants.TS_All) + "sendAck=("
                                + acksqn[0] + ", " + acksqn[1] + ')');
                    }
	        	}
        	}finally {
        		deliverLock.unlock();
        	}
        	if (act != null)
        		act.request(false);
        }
        ImaSessionAction act = new ImaSessionAction(ImaAction.recover, this);
        writeAckSqns("recover", act,null);
        act.request(true);
        /*
         * If recover succeeds, mark all messages received before the recover.
         * The last message ID to be removed is passed in recover.
         */
        if (act.rc == 0) {
            long sqn = ImaUtils.getLong(act.inBB);
            if (sqn > 0) 
                setLastDeliveredSqn(sqn);
            if (connect.isTraceable(7))
                connect.trace(7, "Recover session: session=" + toString(ImaConstants.TS_All) + "sqn=" + sqn);
            
            ImaSessionAction saction = new ImaSessionAction(ImaAction.resumeSession, this);
            saction.request(false);
        } else {
            connect.trace(4, "Recover session failed: session=" + toString(ImaConstants.TS_All) +
             		" rc=" + act.rc);
        }
    }


    /*
     * Rollback the transaction.
     * @see javax.jms.Session#rollback()
     */
    public void rollback() throws JMSException {
        checkClosed();
        if (!transacted) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0001", "A to call commit() or rollback() on a Session object failed because the session was not configured as a transacted session.");
            connect.traceException(2, jex);
            throw jex;
        }
        final ImaSessionAction act = new ImaSessionAction(ImaAction.rollbackSession, this);
        writeAckSqns("rollback", act,null);
        act.request(true);
        /*
         * If rollback succeeds, mark all messages received before the rollback.
         * The last message ID to be removed is passed in rollback response.
         */
        if (act.rc == 0) {
            long sqn = ImaUtils.getLong(act.inBB);
            if (sqn > 0) {
                lastDeliveredMessage = sqn;
                lastAckedMessage = sqn;
                Iterator<ImaConsumer> it = consumerList.iterator();
                while (it.hasNext()) {
                	ImaConsumer consumer = it.next();
                	if (consumer instanceof ImaMessageConsumer) {
    					ImaMessageConsumer imc = (ImaMessageConsumer) consumer;
    					imc.lastDeliveredMessage = sqn;
    				}
                }
            }
            if (connect.isTraceable(7))
                connect.trace(7, "Rollback transaction: session=" + toString(ImaConstants.TS_All) + "sqn=" + sqn);
            
            ImaSessionAction saction = new ImaSessionAction(ImaAction.resumeSession, this);
            saction.request(false);
        } else {
            connect.trace(4, "Rollback transaction failed: session=" + toString(ImaConstants.TS_All) + " rc=" + act.rc);
        }
        
    }

    
    /*
     * Session run. 
     * We do nothing on this.
     * @see javax.jms.Session#run()
     */
    public void run() {
    }

    
    /*
     * @see javax.jms.Session#setMessageListener(javax.jms.MessageListener)
     */
    public void setMessageListener(MessageListener listener) throws JMSException {
        checkClosed();
        throw ImaJms.notImplemented("Session.setMessageListener");
    }

    
    /*
     * @see javax.jms.Session#unsubscribe(java.lang.String)
     */
    public void unsubscribe(String name) throws JMSException {
        unsubscribeImpl(name, false);
    }
    
    /*
     * Implement unsubscribe
     */
    public void unsubscribeImpl(String name, boolean sharedNamespace) throws JMSException {
        checkClosed();
        if ("queue".equals(get("ObjectType"))) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0018", "A topic method ({0}) failed because it was called on a QueueSession object.", "unsubscribe");
            connect.traceException(2, jex);
            throw jex;
        }
        ImaAction act = new ImaSessionAction(ImaAction.unsubscribeDurable, this);
        act.outBB = ImaUtils.putStringValue(act.outBB, name);              /* val0 = name */
        act.outBB = ImaUtils.putBooleanValue(act.outBB, sharedNamespace);  /* val1 = sharedNamespace */
        act.setHeaderCount(2);
        act.request();
        if (act.rc != 0) {
            switch(act.rc) {
       		case ImaReturnCode.IMARC_NotAuthorized:
   		    case ImaReturnCode.IMARC_NotAuthenticated: 
                if (connect.isTraceable(2)) {
                	connect.trace("Failed to unsubscribe: reason=Authorization failed." +
                			" name=" + name + " session=" + toString(ImaConstants.TS_Common));
                }
                ImaJmsSecurityException jex = new ImaJmsSecurityException("CWLNC0207", "A client request to MessageSight failed due to an authorization failure.");
                connect.traceException(2, jex);
                throw jex;
                
   		    case ImaReturnCode.IMARC_DestinationInUse:
   		        int headerCount = act.inBB.get(ImaAction.HDR_COUNT_POS);
   		        int consumerCount = -1;
   		        if (headerCount >= 2) {
   		            consumerCount = ImaUtils.getInteger(act.inBB);
   		        }    
                if (connect.isTraceable(2)) {
                    connect.trace("The subscription has active consumers: rc="+ act.rc + 
                           " name=" + name + " count=" + consumerCount + 
                           " session=" + toString(ImaConstants.TS_Common));
                }
                ImaInvalidDestinationException jex2 = new ImaInvalidDestinationException("CWLNC0226", 
                        "A call to unsubscribe() from client {1} for subscription {0} failed because there are active consumers of the subscription, or a session is still using messages from the subscription.", name,
                        connect.generated_clientid ? "__Shared" : connect.clientid, consumerCount);
                connect.traceException(2, jex2);
                throw jex2;
                
            case ImaReturnCode.IMARC_NotFound:
                if (connect.isTraceable(2)) {
                    connect.trace("The subscription was not found: rc="+ act.rc + 
                           " name=" + name + " session=" + toString(ImaConstants.TS_Common));
                }
                ImaInvalidDestinationException jex3 = new ImaInvalidDestinationException("CWLNC0227", 
                        "A call to unsubscribe() from client {1} failed because the subscription {0} was not found.", name,
                        connect.generated_clientid ? "__Shared" : connect.clientid);
                connect.traceException(2, jex3);
                throw jex3;
                
            default:
                if (connect.isTraceable(2)) {
                	connect.trace("Failed to unsubscribe: rc="+ act.rc + 
                           " name=" + name + " session=" + toString(ImaConstants.TS_Common));
                }
                ImaJmsExceptionImpl jex4 = new ImaJmsExceptionImpl("CWLNC0206",
                        "Client {2} failed to unsubscribe from subscription {1} with MessageSight return code = {0}.", act.rc, name,
                        connect.generated_clientid ? "__Shared" : connect.clientid);
                connect.traceException(2, jex4);
                throw jex4;
            }
        }
        
        if (connect.isTraceable(5)) {
        	connect.trace("Unsubscribe successful: name=" + name +
        			" session=" + toString(ImaConstants.TS_Common));
        }
    }

    
    /*
     *
     * @see javax.jms.TopicSession#createPublisher(javax.jms.Topic)
     */
    public TopicPublisher createPublisher(Topic topic) throws JMSException {
        return (TopicPublisher) createProducer(topic, ImaJms.Topic);
    }

    
    /*
     *
     * @see javax.jms.TopicSession#createSubscriber(javax.jms.Topic)
     */
    public TopicSubscriber createSubscriber(Topic topic) throws JMSException {
    	return (TopicSubscriber)createConsumer(topic, null, false, ImaJms.Topic);
    }

    
    /*
     *
     * @see javax.jms.TopicSession#createSubscriber(javax.jms.Topic, java.lang.String, boolean)
     */
    public TopicSubscriber createSubscriber(Topic topic, String selector, boolean nolocal) throws JMSException {
    	return (TopicSubscriber)createConsumer(topic, selector, nolocal, ImaJms.Topic);
    }
    
    
    /*
     * Receive a notice that the producer is being closed.
     * Remove the producer from our producer list.
     */
    void closeProducer(ImaProducer producer) {
        producerList.remove(new Integer(producer.producerid));
    }

    
    /*
     * Receive a notice that the consumer is being closed.
     * Remove the consumer from our consumer list.
     */
    void closeConsumer(ImaMessageConsumer consumer) {
        consumerList.remove(consumer);
    }

    
    /*
     * Create a queue receiver.
     * @see javax.jms.QueueSession#createReceiver(javax.jms.Queue)
     */
    public QueueReceiver createReceiver(Queue queue) throws JMSException {
        return (QueueReceiver) createConsumer(queue, null, false, ImaJms.Queue);
    }


    /*
     * Create a queue receiver.
     * @see javax.jms.QueueSession#createReceiver(javax.jms.Queue, java.lang.String)
     */
    public QueueReceiver createReceiver(Queue queue, String selector) throws JMSException {
        return (QueueReceiver) createConsumer(queue, selector, false, ImaJms.Queue);
    }


    /*
     * Create a queue sender.
     * @see javax.jms.QueueSession#createSender(javax.jms.Queue)
     */
    public QueueSender createSender(Queue queue) throws JMSException {
        return (QueueSender) createProducer(queue, ImaJms.Queue);
    }

    
    /*
     * Get a consumer by ID
     */
    ImaMessageConsumer getConsumer(int id) {
        return (ImaMessageConsumer)connect.consumerMap.get(Integer.valueOf(id));
    }
    
    
    /*
     * Create a delivery thread and put it into the array 
     */
    synchronized ImaDeliveryThread getDeliveryThread() {
        if (deliveryThread != null)
            return deliveryThread;
        synchronized (ImaSession.class) {
            if (deliveryThreads[nextThread] == null) {
                deliveryThreads[nextThread] = new ImaDeliveryThread();
                final ImaDeliveryThread delthread = deliveryThreads[nextThread];
                AccessController.doPrivileged(new PrivilegedAction<Object>() {
                    public Object run() {
                        Thread th = new Thread(delthread, "ImaJmsDeliveryThread."+nextThread);
                        th.setDaemon(true);
                        th.start();
                        return null;
                    }
                }); 
            }
            int idx = nextThread++;
            if (nextThread == deliveryThreads.length)
                nextThread = 0;
            deliveryThread = deliveryThreads[idx];
        }
        return deliveryThread;
    }
    
    
    /*
     * 
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return "IMASession@"+Integer.toHexString(hashCode()); 
    }

    /*
     * Get class name as part of detailed toString
     */
    public String getClassName() {
    	return "ImaSession";
    }
    
    /*
     * Get info for detailed toString
     */
    public String getInfo() {
    	return sessid >= 0 ? ("id="+sessid) : null;
    }
}
