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

import java.io.IOException;
import java.util.ArrayList;

import javax.jms.JMSException;
import javax.jms.IllegalStateException;
import javax.jms.Message;
import javax.jms.MessageProducer;
import javax.jms.Destination;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;


public class SendMessageLoopAction extends ApiCallAction {
    private final String  _producerID;
    private final String  _msgList;
    private final String  _destID;
    private final String  _deliveryMode;
    private final String  _priority;
    private final String  _ttl;
    private final String  _messageIDS;
    private final boolean _checkIDS;
    private final int     _maxID;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public SendMessageLoopAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);    
        _msgList = _actionParams.getProperty("message_list");
        if(_msgList == null){
            throw new RuntimeException("message_id is not defined for " + this.toString());
        }
        _producerID = _actionParams.getProperty("producer_id");
        if(_producerID == null){
            throw new RuntimeException("producer_id is not defined for " + this.toString());
        }
        _destID = _actionParams.getProperty("dest_id");
        if (_apiParameters.getProperty("deliveryMode") != null) {
        	_deliveryMode = _apiParameters.getProperty("deliveryMode");
        } else {
        	_deliveryMode = _actionParams.getProperty("deliveryMode");
        }
        if (_apiParameters.getProperty("priority") != null) {
        	_priority = _apiParameters.getProperty("priority");
        } else {
        	_priority = _actionParams.getProperty("priority");
        }
        if (_apiParameters.getProperty("ttl") != null) {
        	_ttl = _apiParameters.getProperty("ttl");
        } else {
        	_ttl = _actionParams.getProperty("ttl");
        }
        _messageIDS = _actionParams.getProperty("messageIDS");
        _checkIDS = Boolean.parseBoolean(_actionParams.getProperty("checkIDS", "false"));
        _maxID = Integer.parseInt(_actionParams.getProperty("maxID", "0"));
        if(_maxID == 0 || _messageIDS == null) {
            throw new RuntimeException("maxID and messageIDS must both be defined for " + this.toString());
        }
    }

    protected boolean invokeApi() throws JMSException {
        final MessageProducer producer = (MessageProducer) _dataRepository.getVar(_producerID);
        Destination dest = null;
        int delMode = 0;
        int priority = 0;
        long ttl = 0;
        int incrID = 0;
        int msgID;
        int index = 0;
        int [] messageIDS = null;

        String [] messageList = getStringList(_msgList);
        Message [] msgs = new Message[messageList.length];
        incrID = messageList.length;

        for (int i=0; i<messageList.length; i++) {
            msgs[i] = (Message) _dataRepository.getVar(messageList[i]);
            if (msgs[i] == null) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", "Failed to locate the Message object ({0}).", messageList[i]);
                return false;
            }
        }

        if (_messageIDS != null) {
            messageIDS = (int []) _dataRepository.getVar(_messageIDS);
            if (messageIDS == null) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2077", "Action {0}: Message ID array ({1}) does not exist. Creating new array.", _id, _messageIDS);
                messageIDS = new int[_maxID];
                index = 0;
            } else {
                for (index=0; index<_maxID; index++) {
                    if (messageIDS[index] == 0)
                        break;
                }
            }
        }

        if(producer == null){
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", "Failed to locate the MessageProducer object ({0}).", _producerID);
            return false;
        }
        
        if (_destID != null) {
            dest = (Destination) _dataRepository.getVar(_destID);
        }
        
        if (_deliveryMode != null) {
            delMode = Integer.parseInt(_deliveryMode);
        }
        if (_priority != null) {
            priority = Integer.parseInt(_priority);
        }
        if (_ttl != null) {
            ttl = Long.parseLong(_ttl);
        }

        try {
            if (producer.getDestination() != null) {
                for (int i=index; i<_maxID; i++) {
                    msgID = msgs[i%incrID].getIntProperty("ID");
                    if (_deliveryMode != null || _priority != null || _ttl != null) {
                        producer.send(msgs[i%incrID], (_deliveryMode != null) ? delMode : producer.getDeliveryMode(),
                                (_priority != null) ? priority : producer.getPriority(),
                                (_ttl != null) ? ttl : producer.getTimeToLive());
                    }
                    else {
                        producer.send(msgs[i%incrID]);
                    }
                    messageIDS[i] = 1;
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2038", "Action {0}: Send Message completed for message {1}, ID={2}.", _id, messageList[i%incrID], msgID);
                    msgs[i%incrID].setIntProperty("ID", msgID+incrID);
                }
            } else {
                for (int i=0; i<_maxID; i++) {
                    msgID = msgs[i%incrID].getIntProperty("ID");
                    if (dest == null){
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", "Action {0}: Failed to locate the MessageProducer object({0}).", _id, _destID);
                        return false;
                    }
                    if (_deliveryMode != null || _priority != null || _ttl != null) {
                        producer.send(dest, msgs[i%incrID], (_deliveryMode != null) ? delMode : producer.getDeliveryMode(),
                           (_priority != null) ? priority : producer.getPriority(),
                           (_ttl != null) ? ttl : producer.getTimeToLive());
                    }
                    else {
                        producer.send(dest, msgs[i%incrID]);
                    }
                    messageIDS[i] = 1;
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2038", "Action {0}: Send Message completed for message {1}, ID={2}.", _id, messageList[i%incrID], msgID);
                    msgs[i%incrID].setIntProperty("ID", msgID+incrID);
                }
            }
        } catch (JMSException e) {
            _trWriter.writeException(e);
            if (!(e.getLinkedException() instanceof IOException) && !(e instanceof IllegalStateException)) {
            	throw e;
            }
        }

        if (_checkIDS == true) {
            for (int i=0; i<_maxID; i++) {
                if (messageIDS[i] == 0) {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2037", "Action {0}: Send Message failed at message ID={1}", _id, i+1);
                    break;
                }
            }
        }

        _dataRepository.storeVar(_messageIDS, messageIDS);

        return true;
    }

    static String [] getStringList(String s) {
        ArrayList<String> al = new ArrayList<String>();
        int pos = 0;
        int len = s.length();
        int start = -1;
        while (pos < len) {
            char ch = s.charAt(pos);
            if (ch==' ' || ch=='\t' || ch==';') {
                if (start >= 0)
                    al.add(s.substring(start, pos));
                start = -1;
            } else {
                if (start < 0)
                    start = pos;
            }
            pos++;
        }
        if (start >= 0)
            al.add(s.substring(start, pos));
        return al.toArray(new String[0]);
    }
}
