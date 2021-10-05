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

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.Queue;
import javax.jms.QueueReceiver;
import javax.jms.Topic;
import javax.jms.TopicSubscriber;

import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaIllegalStateException;
import com.ibm.ima.jms.impl.ImaMessageConsumer;
import com.ibm.ima.jms.impl.ImaTrace;

final class ImaMessageConsumerProxy extends ImaProxy implements MessageConsumer, QueueReceiver, TopicSubscriber {
    /** the Destination to receive messages from */
    private final Destination        destination;
    /** the Consumer wrapped by this class */
    private final ImaMessageConsumer consumer;

    ImaMessageConsumerProxy(ImaSessionProxy sess, ImaMessageConsumer cons, Destination dest) {
        super(sess);
        destination = dest;
        consumer = cons;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.TopicSubscriber#getNoLocal()
     */
    public boolean getNoLocal() throws JMSException {
        return consumer.getNoLocal();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.TopicSubscriber#getTopic()
     */
    public Topic getTopic() throws JMSException {
        return (Topic) destination;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.QueueReceiver#getQueue()
     */
    public Queue getQueue() throws JMSException {
        return (Queue) destination;
    }

    protected void closeImpl(boolean connClosed) throws JMSException {
        if (!connClosed)
            consumer.close();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.MessageConsumer#getMessageListener()
     */
    public MessageListener getMessageListener() throws JMSException {
        ImaIllegalStateException ex = ImaIllegalStateException.apiNotAllowedForConsumerProxy("getMessageListener");
        traceException(1, ex);
        throw ex;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.MessageConsumer#setMessageListener(javax.jms.MessageListener)
     */
    public void setMessageListener(MessageListener arg0) throws JMSException {
        ImaIllegalStateException ex = ImaIllegalStateException.apiNotAllowedForConsumerProxy("setMessageListener");
        traceException(1, ex);
        throw ex;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.MessageConsumer#getMessageSelector()
     */
    public String getMessageSelector() throws JMSException {
        return consumer.getMessageSelector();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.MessageConsumer#receive()
     */
    public Message receive() throws JMSException {
        return consumer.receive();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.MessageConsumer#receive(long)
     */
    @Override
    public Message receive(long timeout) throws JMSException {
        return consumer.receive(timeout);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.MessageConsumer#receiveNoWait()
     */
    public Message receiveNoWait() throws JMSException {
        return consumer.receiveNoWait();
    }
    
    private void traceException(int level, Throwable ex) {
        ImaConnection conn = null;
        if (consumer != null)
            conn = (ImaConnection)consumer.get("Connection");
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
