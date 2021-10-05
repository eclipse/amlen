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

import javax.jms.CompletionListener;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageProducer;
import javax.jms.QueueSender;
import javax.jms.TopicPublisher;


/**
 * Implement the MessgeProducer interface for the IBM MessageSight JMS client.
 *
 * A message producer can either set the destination when it is created, or specify it on
 * each send.  
 * 
 * This method is extended in the JMS 2.0 implementation
 */
public class ImaMessageProducer extends ImaProducer implements MessageProducer, TopicPublisher, QueueSender {

    public ImaMessageProducer(ImaSession session, Destination dest, int domain) throws JMSException {
        super(session, dest, domain);
    }

    /*
     * @see javax.jms.MessageProducer#send(javax.jms.Message, javax.jms.CompletionListener)
     */
    public void send(Message message, CompletionListener listener) throws JMSException {
        send(message, this.deliverymode, this.priority, this.expire, listener);     
    }

    /*
     * @see javax.jms.MessageProducer#send(javax.jms.Message, int, int, long, javax.jms.CompletionListener)
     */
    public void send(Message message, int delmode, int priority, long ttl, CompletionListener listener) throws JMSException {
        checkClosed();
        checkNoDest();
        checkDeliveryMode(message, delmode);
        if (priority != Message.DEFAULT_PRIORITY)
            checkPriority(priority);
        sendAsync(this.dest, message, delmode, priority, expire, listener);            
    }

    /*
     * @see javax.jms.MessageProducer#send(javax.jms.Destination, javax.jms.Message, javax.jms.CompletionListener)
     */
    public void send(Destination dest, Message message, CompletionListener listener) throws JMSException {
        send(dest, message, this.deliverymode, this.priority, this.expire, listener);         
    }
    
    /*
     * @see javax.jms.MessageProducer#send(javax.jms.Destination, javax.jms.Message, int, int, long, javax.jms.CompletionListener)
     */
    public void send(Destination dest, Message message, int deliveryMode, int priority, long ttl, CompletionListener listener) throws JMSException {
        checkClosed();
        session.checkDestination(dest);
        checkHasDest();
        checkDeliveryMode(message, deliveryMode);
        if (priority != Message.DEFAULT_PRIORITY)
            checkPriority(priority);
        sendAsync(dest, message, deliveryMode, priority, ttl, listener);
    }
    
    
    /*
     * Send asynchronous
     */
    void sendAsync(Destination dest, Message message, int deliveryMode, int priority, long ttl, CompletionListener listener) throws JMSException {
        if (listener != null) {
            try {
                sendM(dest, message, deliveryMode, priority, ttl, disableTimestamp, disableMessageID, deliveryDelay);
                ImaConnection.currentConnection.set(session.connect);
                listener.onCompletion(message);
                ImaConnection.currentConnection.set(null);
            } catch (Exception ex) {
                listener.onException(message, ex);
                ImaConnection.currentConnection.set(null);
            }
        } else {
            throw new ImaIllegalArgumentException("CWLNC0089", "The completion listener cannot be null");
        } 
    }
}
