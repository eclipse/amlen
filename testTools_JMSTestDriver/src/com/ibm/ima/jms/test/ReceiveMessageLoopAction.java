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

import java.io.UnsupportedEncodingException;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.BytesMessage;
import javax.jms.TextMessage;
import javax.jms.MessageConsumer;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class ReceiveMessageLoopAction extends ApiCallAction {
    private final String  _consumerID;
    private final String  _listenerID;
    private final long    _timeout;
    private final int     _maxID;
    private final String  _messageIDS;
    private final boolean _checkIDS;
    private final boolean _transacted;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public ReceiveMessageLoopAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _consumerID = _actionParams.getProperty("consumer_id");
        _listenerID = _actionParams.getProperty("listener_id");
        if((_listenerID == null) && (_consumerID == null) ){
            throw new RuntimeException("Both consumer_id and listener_id are not defined for " + this.toString());
        }
        if((_listenerID != null) && (_consumerID != null) ){
            throw new RuntimeException("Both consumer_id and listener_id are defined for " + this.toString());
        }
        _timeout = Long.parseLong(_apiParameters.getProperty("timeout", "0"));
        _maxID = Integer.parseInt(_actionParams.getProperty("maxID", "0"));
        _checkIDS = Boolean.parseBoolean(_actionParams.getProperty("checkIDS", "false"));
        _messageIDS = _actionParams.getProperty("messageIDS");
        if (_apiParameters.getProperty("transacted") != null) {
        	_transacted = Boolean.parseBoolean(_apiParameters.getProperty("transacted", "false"));
        } else {
        	_transacted = Boolean.parseBoolean(_actionParams.getProperty("transacted", "false"));
        }
        
    }

    protected boolean invokeApi() throws JMSException {
        Message msg;
        int messageCount = 0;
        int messageID;
        int [] messageIDS = null;
        if (_messageIDS != null && _maxID > 0) {
            messageIDS = (int []) _dataRepository.getVar(_messageIDS);
            if (messageIDS == null) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2077", "Action {0}: Message ID array ({1}) does not exist. Creating new array.", _id, _messageIDS);
                messageIDS = new int[_maxID];
            }
        }

        try {
            if(_consumerID != null){
                final MessageConsumer consumer = (MessageConsumer) _dataRepository.getVar(_consumerID);            
                if(consumer == null){
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                            "Action {0}: Failed to locate the MessageConsumer object ({1}).",_id, _consumerID);
                    return false;
                }
    
                while ((msg = consumer.receive(_timeout)) != null) {
                    messageCount++;
                    if (_maxID > 0) {
                        messageID = msg.getIntProperty("ID");
                        if (messageIDS[messageID-1] > 0) {
                            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2074", "Action {0}: Received  duplicate message, ID={1}, Redelivered={2}", _id, messageID, msg.getJMSRedelivered());
                            messageIDS[messageID-1]++;
                        } else {
                            messageIDS[messageID-1] = messageID;
                            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2042", "Action {0}: Received message, JMSid={1}, ID={2}, messageIDSs[{3}]={4}, Redelivered={5}", _id, msg.getJMSMessageID(), messageID, messageID-1, messageIDS[messageID-1], msg.getJMSRedelivered());
                        }
                        if (_maxID == messageID) {
                            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2075", "Action {0}: Received last expected message, ID={1}", _id, messageID);
                        }
                    }
                    if (msg instanceof TextMessage) {
                        TextMessage tMsg = (TextMessage) msg;
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2039", "Action {0}: Text Message ({1}) content: {2}", _id, messageCount, tMsg.getText());
                    } else if (msg instanceof BytesMessage) {
                        BytesMessage bMsg = (BytesMessage) msg;
                        byte[] bytes = new byte[(int)bMsg.getBodyLength()];
                        bMsg.readBytes(bytes);
                        String string = null;
                        try {
                            string = new String(bytes, "UTF-8");
                        } catch (UnsupportedEncodingException e) {    // Should not get here
                            _trWriter.writeException(e);
                            return false;
                        }
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2039", "Action {0}: Bytes Message ({1}) content: {2}", _id, messageCount, string);
                    } else {
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "JMST4002", 
                            "Action {0}: The getText API is not valid for message {"+messageCount+"} of type: {1}", _id, msg.getClass().getSimpleName());
                    }
                }
                _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2040", "Action {0}: {1} messages were received.", _id, messageCount);
            } else if (_listenerID != null) {
                final JmsMessageListener listener = (JmsMessageListener) _dataRepository.getVar(_listenerID);
                if(listener == null){
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                            "Action {0}: Failed to locate the MessageListener object ({1}).", _id,_listenerID);
                    return false;
                }
    
                while ((msg = listener.receive(_timeout)) != null) {
                    messageCount++;
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2042", "Action {0}: Received message, JMSid={1}", _id, msg.getJMSMessageID());
                    if (_maxID > 0) {
                        messageID = msg.getIntProperty("ID");
                        if (messageIDS[messageID-1] > 0) {
                            _trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "RMTD2074", "Action {0}: Received  duplicate message, ID={1}", _id, messageID);
                        } else {
                            messageIDS[messageID-1] = messageID;
                        }
                        if (_maxID == messageID) {
                            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2075", "Action {0}: Received last expected message, ID={1}", _id, messageID);
                        }
                    }
                    if (msg instanceof TextMessage) {
                        TextMessage tMsg = (TextMessage) msg;
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2039", "Action {0}: Text Message ({1}) content: {2}", _id, messageCount, tMsg.getText());
                    } else if (msg instanceof BytesMessage) {
                        BytesMessage bMsg = (BytesMessage) msg;
                        byte[] bytes = new byte[(int)bMsg.getBodyLength()];
                        bMsg.readBytes(bytes);
                        String string = null;
                        try {
                            string = new String(bytes, "UTF-8");
                        } catch (UnsupportedEncodingException e) {    // Should not get here
                            _trWriter.writeException(e);
                            return false;
                        }
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2039", "Action {0}: Bytes Message ({1}) content: {2}", _id, messageCount, string);
                    } else {
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "JMST4002", 
                            "Action {0}: The getText API is not valid for message {"+messageCount+"} of type: {1}", _id, msg.getClass().getSimpleName());
                    }
                }
                _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2040", "Action {0}: {1} messages were received.", _id, messageCount);
            }
        } catch (Exception e) {
            _trWriter.writeException(e);
        }

        if (_checkIDS == true && _maxID > 0) {
            int fail = 0;
            for (int i=0; i<_maxID; i++) {
                if (messageIDS[i] == 0) {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2076", "Action {0}: Message ID={1} was not received.", _id, i+1);
                    fail = 1;
                }
            }
            if (fail == 0 && _transacted) {
                int j = 0;
                while (messageIDS[j] == 2)
                    j++;
                for (int i=j; i<_maxID; i++) {
                    if (messageIDS[i] == 2) {
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", "Action {0}: Message {1} through message {2} were not redelivered.", _id, j, i-1);
                        fail = 1;
                    }
                }
            }
            if (fail == 1) {
                return false;
            }
        }

        _dataRepository.storeVar(_messageIDS, messageIDS);
        return true;
    }

}
