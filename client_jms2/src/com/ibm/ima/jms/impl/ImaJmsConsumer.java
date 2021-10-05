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

import java.nio.ByteBuffer;

import javax.jms.Destination;
import javax.jms.JMSConsumer;
import javax.jms.JMSException;
import javax.jms.JMSRuntimeException;
import javax.jms.Message;
import javax.jms.MessageListener;

import com.ibm.ima.jms.ImaProperties;

public class ImaJmsConsumer extends ImaReadonlyProperties implements JMSConsumer, ImaConsumer {
    
    ImaMessageConsumer  consumer;
    ImaJmsContext       context;
    Destination         dest;
    
    public ImaJmsConsumer(ImaJmsContext context, Destination dest, String name, boolean durable, boolean shared, String selector, boolean nolocal) {
        super(null);
        ImaSession session = context.session;
        this.dest = dest;
        try {
            int domain;
            if (dest instanceof ImaQueue) {
                domain = ImaJms.Queue;
            } else {
                if (shared) {
                    domain = durable ? ImaJms.Topic_Shared : ImaJms.Topic_SharedNonDurable;
                } else {
                    domain = ImaJms.Topic_False;
                }
            }

            consumer = new ImaMessageConsumer(session, dest, selector, nolocal, durable, name, domain);
            props = ((ImaDestination)dest).getCurrentProperties();
            putInternal("context", context);
            
            if (!durable && !shared) {
                createConsumer(session, dest, domain, selector, nolocal);
            } else {
                createSubscriptionAndConsumer(session, (ImaTopic)dest, name, shared, selector, nolocal);
            }  
            if (ImaTrace.isTraceable(6)) {
                ImaTrace.trace("JMSConsumer created: consumer=" + toString(ImaConstants.TS_All));
            }
            if (context.autostart)
                context.connect.start();
        } catch (Exception jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }
    
    
    /* 
     * Internal check if the session is closed.
     */
    void checkClosed() {
        if (consumer == null || consumer.isClosed) {
            ImaIllegalStateRuntimeException jex = new ImaIllegalStateRuntimeException("CWLNC0010", "The message consumer is closed");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }
    
    /*
     * @see javax.jms.JMSConsumer#close()
     */
    public void close() {
        putInternal("isClosed", true);
        if (consumer != null) {
            try {
                consumer.close();
                consumer = null;
            } catch (Exception jex) {
                throw ImaRuntimeException.mapException(jex);
            }
        }
        if (ImaTrace.isTraceable(6)) {
            ImaTrace.trace("JMSContext closed: context=" + toString(ImaConstants.TS_All));
        }
    }
    
    /*
     * Close the consumer on finalize of the application failed to close it.
     * @see java.lang.Object#finalize()
     */
    protected void finalize() {
        try {
            if (consumer != null && consumer.consumerid > 0)
                consumer.closeInternal();
            consumer = null;
        } catch (JMSException jex) {
        }
    }


    /**
     * 
     * @throws JMSException
     */
    public void closeInternal() throws JMSException {
        close();
    }


    /**
     * 
     */
    public void markClosed() {
        putInternal("isClosed", true);
    }


    /**
     * 
     * @return
     */
    public ImaDestination getDest() {
        checkClosed();
        return consumer.dest;
    }


    /**
     * 
     * @param bb
     * @throws JMSException
     */
    public void handleMessage(ByteBuffer bb) throws JMSException {
        /* Not used */
    }

    /*
     * Create a consumer
     *  
     * This duplicates code in ImaSession.createConsumer and if modified we may need to modify 
     * that code as well.
     */
    void createConsumer(ImaSession session, Destination dest, int domain, String selector, boolean nolocal) {
        if (selector != null && selector.length()== 0)
            selector = null;
        ImaAction act = new ImaCreateConsumerAction(ImaAction.createConsumer, session, consumer);
        synchronized (this) {
            try {
                domain = domain==ImaJms.Queue ? ImaJms.Queue : ImaJms.Topic;
                act.outBB = ImaUtils.putByteValue(act.outBB, (byte)domain);  /* val0 = domain */
                act.outBB = ImaUtils.putBooleanValue(act.outBB, nolocal);     /* val1 = nolocal */
                act.outBB = ImaUtils.putStringValue(act.outBB, selector);     /* val2 = selector */
                act.setHeaderCount(3);
                act.outBB = ImaUtils.putMapValue(act.outBB,consumer);
    
                checkClosed();
                act.request();
            } catch (JMSException jex) {
                throw ImaRuntimeException.mapException(jex);
            }
                
            if (act.rc != 0) {
                switch (act.rc) {
                case ImaReturnCode.IMARC_NotAuthorized:
                case ImaReturnCode.IMARC_NotAuthenticated: 
                    if (ImaTrace.isTraceable(2)) {
                        ImaTrace.trace("Failed to create consumer due to authorization failure: consumer=" + 
                                consumer.toString(ImaConstants.TS_All));
                    }
                    ImaJmsSecurityRuntimeException jex = new ImaJmsSecurityRuntimeException("CWLNC0207", "Authorization failed");
                    ImaTrace.traceException(2, jex);
                    throw jex;
                    
                case ImaReturnCode.IMARC_DestNotValid: 
                    if (ImaTrace.isTraceable(2)) {
                        ImaTrace.trace("Failed to create consumer: reason=\"destination not valid\" consumer=" + 
                                consumer.toString(ImaConstants.TS_All));
                    }
                    ImaInvalidDestinationRuntimeException jex3 = 
                        new ImaInvalidDestinationRuntimeException("CWLNC0217", "Destination \"{0}\" is not valid", 
                            ((ImaProperties)dest).get("Name").toString());
                    ImaTrace.traceException(2, jex3);
                    throw jex3;
                    
                case ImaReturnCode.IMARC_BadSysTopic: 
                    if (ImaTrace.isTraceable(2)) {
                        ImaTrace.trace("Failed to create consumer: reason=\"The topic is a system topic\" consumer=" + 
                                consumer.toString(ImaConstants.TS_All));
                    }
                    ImaJmsRuntimeException jex4 = new ImaJmsRuntimeException("CWLNC0225", "The topic is a system topic: {0}", 
                            ((ImaProperties)dest).get("Name").toString());
                    ImaTrace.traceException(2, jex4);
                    throw jex4;
    
                default:
                    if (ImaTrace.isTraceable(2)) {
                        ImaTrace.trace("Failed to create consumer: rc=" + act.rc +
                                " consumer="+consumer.toString(ImaConstants.TS_All));
                    }
                    ImaJmsRuntimeException jex2 = new ImaJmsRuntimeException("CWLNC0209", "Failed to create consumer  (rc = {0}).", act.rc);
                    ImaTrace.traceException(2, jex2);
                    throw jex2;
    
                }
            }
            try {
                ImaAction saction = new ImaSessionAction(ImaAction.resumeSession, session);
                saction.request(false);
            } catch (JMSException jex) {
                throw ImaRuntimeException.mapException(jex);
            }
        }
    } 
    
    /*
     * Create a subscription and consumer.
     * 
     * This duplicates code in ImaSession.createSubscriptionAndConsumer and if modified we may need to modify 
     * that code as well.
     */
    void createSubscriptionAndConsumer(ImaSession session, ImaTopic dest, String name, boolean shared, String selector, boolean nolocal) {
        if (selector != null && selector.length()== 0)
            selector = null;
        synchronized (this) {
            ImaAction act = new ImaCreateConsumerAction(ImaAction.createDurable, session, consumer);
            try {
                act.outBB = ImaUtils.putStringValue(act.outBB, name);      /* val0 = name     */ 
                act.outBB = ImaUtils.putStringValue(act.outBB, selector);  /* val1 = selector */
                act.setHeaderCount(2);
                act.outBB = ImaUtils.putMapValue(act.outBB, consumer); 
    
                checkClosed();
                act.request();
                
                ImaAction saction = new ImaSessionAction(ImaAction.resumeSession, session);
                saction.request(false);
            } catch (JMSException jex) {
                throw ImaRuntimeException.mapException(jex);
            }
            
            if (act.rc != 0) {
                String cid = session.connect.clientid;
                if (session.connect.generated_clientid) {
                    cid = consumer.shared==2 ? "__SharedND" : "__Shared";
                }
                switch (act.rc) {
                case ImaReturnCode.IMARC_NotAuthorized:
                case ImaReturnCode.IMARC_NotAuthenticated: 
                    if (ImaTrace.isTraceable(2)) {
                        ImaTrace.trace("Failed to create consumer: reason=Authorization failed. name=" + name + 
                                " session=" + session.toString(ImaConstants.TS_All));

                    }
                    ImaJmsSecurityRuntimeException jex = new ImaJmsSecurityRuntimeException("CWLNC0207", "Authorization failed");
                    ImaTrace.traceException(2, jex);
                    throw jex;
                    
                case ImaReturnCode.IMARC_BadSysTopic: 
                    if (ImaTrace.isTraceable(2)) {
                        ImaTrace.trace("Failed to create consumer: reason=The topic is a system topic. name=" + name + 
                                 " session=" + session.toString(ImaConstants.TS_All));
                    }
                    ImaJmsRuntimeException jex3 = new ImaJmsRuntimeException("CWLNC0225", "Failed to create consumer because the topic is a system topic: {0}",
                            dest.getName());
                    ImaTrace.traceException(2, jex3);
                    throw jex3;
                    

                case ImaReturnCode.IMARC_ExistingSubscription: 
                    if (ImaTrace.isTraceable(2)) {
                        ImaTrace.trace("Failed to create consumer: reason=The subscription cannot be modified because the subscription is in use." +
                                " name=" + name + " session=" + session.toString(ImaConstants.TS_All));
                    }
                    ImaIllegalStateRuntimeException jex4 = new ImaIllegalStateRuntimeException("CWLNC0229", 
                            "The subscription cannot be modified because the subscription is in use: Name={0} ClientID={1}.",
                            name, cid);
                    ImaTrace.traceException(2, jex4);
                    throw jex4;
                    
                case ImaReturnCode.IMARC_DestinationInUse: 
                    if (ImaTrace.isTraceable(2)) {
                        ImaTrace.trace("Failed to create consumer: reason=An active non-shared durable consumer already exists." +
                                " name=" + name + " session=" + session.toString(ImaConstants.TS_All));
                    }
                    ImaIllegalStateRuntimeException jex6 = new ImaIllegalStateRuntimeException("CWLNC0021", 
                            "An active non-shared durable consumer already exists: Name={0} ClientID={1}",
                            name, cid);
                    ImaTrace.traceException(2, jex6);
                    throw jex6;
                    
                case ImaReturnCode.IMARC_ShareMismatch: 
                    if (ImaTrace.isTraceable(2)) {
                        ImaTrace.trace("Failed to create consumer: reason=Share mistmach name=" + name + 
                                " session=" + session.toString(ImaConstants.TS_All));
                    }
                    ImaJmsRuntimeException jex5 = new ImaJmsRuntimeException("CWLNC0228", "The shared subscription cannot be modified: Name={0} ClientID={1}.",
                            name, cid);
                    ImaTrace.traceException(2, jex5);
                    throw jex5;

                default:
                    if (ImaTrace.isTraceable(2)) {
                        ImaTrace.trace("Failed to create consumer: rc=" + act.rc + 
                            " session=" + session.toString(ImaConstants.TS_All));
                    }
                    ImaJmsRuntimeException jex2 = new ImaJmsRuntimeException("CWLNC0209",
                            "Failed to create consumer: rc={0} Name={1} ClientID={2}", act.rc, name, cid);
                    ImaTrace.traceException(2, jex2);
                    throw jex2;

                }
            }
        }
    }
    
    /*
     * @see javax.jms.JMSConsumer#getMessageListener()
     */
    public MessageListener getMessageListener() throws JMSRuntimeException {
        checkClosed();
        return consumer.listener;
    }

    /*
     * @see javax.jms.JMSConsumer#getMessageSelector()
     */
    public String getMessageSelector() {
        checkClosed();
        return consumer.selector;
    }

    /*
     * @see javax.jms.JMSConsumer#receive()
     */
    public Message receive() {
        checkClosed();
        try {
            return consumer.receive();
        } catch (Exception jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /*
     * @see javax.jms.JMSConsumer#receive(long)
     */
    public Message receive(long timeout) {
        checkClosed();
        try {
            return consumer.receive(timeout);
        } catch (Exception jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /*
     * @see javax.jms.JMSConsumer#receiveBody(java.lang.Class)
     */
    public <T> T receiveBody(Class<T> cls) {
        checkClosed();
        try {
            Message msg = consumer.receive();
            if (msg == null)
                return null;
            return msg.getBody(cls);
        } catch (Exception jex) {
            jex.printStackTrace(System.out);
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /*
     * @see javax.jms.JMSConsumer#receiveBody(java.lang.Class, long)
     */
    public <T> T receiveBody(Class<T> cls, long timeout) {
        checkClosed();
        try {
            Message msg = consumer.receive(timeout);
            if (msg == null) {
                return null;
            }    
            return (T) msg.getBody(cls);
        } catch (Exception jex) {
            jex.printStackTrace(System.out);
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /*
     * @see javax.jms.JMSConsumer#receiveBodyNoWait(java.lang.Class)
     */
    public <T> T receiveBodyNoWait(Class<T> cls) {
        checkClosed();
        try {
            Message msg = consumer.receiveNoWait();
            if (msg == null)
                return null;
            return msg.getBody(cls);
        } catch (Exception jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /*
     * @see javax.jms.JMSConsumer#receiveNoWait()
     */
    public Message receiveNoWait() {
        checkClosed();
        try {
            return consumer.receiveNoWait();
        } catch (Exception jex) {
            throw ImaRuntimeException.mapException(jex);
        }
    }

    /*
     * @see javax.jms.JMSConsumer#setMessageListener(javax.jms.MessageListener)
     */
    public void setMessageListener(MessageListener listener) throws JMSRuntimeException {
        checkClosed();
        try {
            consumer.setMessageListener(listener);
        } catch (Exception jex) {
            throw ImaRuntimeException.mapException(jex);
        }
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
        return "ImaJMSConsumer";
    }
}
