/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.jms.test;

import java.util.Properties;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageListener;

import com.ibm.ima.jms.impl.ImaTextMessage;
import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class JmsMessageListener extends Callback implements MessageListener {
	LinkedBlockingQueue<Message> _receivedMessages = new LinkedBlockingQueue<Message>();
    private final String _listenerType;
    private final int    _stopAfterNum;
    private final int    _ackInterval;
    private final long   _delay;
    private int          _count;
    
    JmsMessageListener(String id, Properties config, TrWriter trWriter){
        super(id,config,trWriter);
        _listenerType = config.getProperty("listener_type", "default");
        _stopAfterNum = Integer.parseInt(config.getProperty("stopAfterNum", "100"));
        _ackInterval = Integer.parseInt(config.getProperty("ack_interval", "10"));
        _delay = Long.parseLong(config.getProperty("delay", "1000"));
        _count = 0;
    }
    
    public void onMessage(Message message) {
        String value = null;
        try {
            value = (message  instanceof ImaTextMessage) ? ((ImaTextMessage)message).getText() : message.toString();
        } catch (JMSException e) {
            value = message.toString();
        }
        _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "JMST9999", "JmsMessageListener.onMessage: {0} : Message is {1}", _id, value);
        _count++;
        if (_listenerType.equals("client_ack")) {
            if (_count % _ackInterval == 0) {
                try {
                    message.acknowledge();
                } catch (JMSException e) {
                    _trWriter.writeException(e);
                }
            }
        } else if (_listenerType.equals("delay")) {
        	synchronized (this) {
                if (_count == _stopAfterNum) {
                    try {
                        wait(_delay);
                    } catch (Exception e) {
                        _trWriter.writeException(e);
                    }
                }
        	}
        }
        
        callback();
        try {
        	_receivedMessages.put(message);
        } catch(InterruptedException iex) {
        	//Do nothing
        }
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD9999", "Action {0}: Exiting onMessage. count = {1}", _id, _count);
    }

    Message receive(long timeout){
        Message retmsg = null;
        try {
        	retmsg = _receivedMessages.poll(timeout, TimeUnit.MILLISECONDS);
        } catch(InterruptedException iex) {
        	//Do nothing
        }
        return retmsg;
    }
}
