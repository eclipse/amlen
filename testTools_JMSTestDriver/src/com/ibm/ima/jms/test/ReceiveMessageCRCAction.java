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

import java.util.zip.CRC32;
import java.util.zip.Checksum;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.BytesMessage;
import javax.jms.MessageConsumer;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class ReceiveMessageCRCAction extends ApiCallAction {
    private final String  _consumerID;
    private final String  _listenerID;
    private final long    _timeout;
    private final int     _numMessages;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public ReceiveMessageCRCAction(Element config, DataRepository dataRepository,
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
        _numMessages = Integer.parseInt(_actionParams.getProperty("num_messages", "0"));
    }

    protected boolean invokeApi() throws JMSException {
    	Message msg;
    	int msgCount = 0;

    	if (_consumerID != null) {
    	    final MessageConsumer consumer = (MessageConsumer) _dataRepository.getVar(_consumerID);
            if(consumer == null){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                        "Action {0}: Failed to locate the MessageConsumer object ({1}).", _id,_consumerID);
                return false;
            }
            
            while ((msg = consumer.receive(_timeout)) != null) {
                if (msg instanceof BytesMessage) {
                    BytesMessage bMsg = (BytesMessage) msg;
                    byte[] bytes = new byte[(int)bMsg.getBodyLength()-8];
                    long msgChecksum = bMsg.readLong();
                    bMsg.readBytes(bytes);
                    Checksum checksum = new CRC32();
                    checksum.update(bytes,0,bytes.length);
                    long calcChecksum = checksum.getValue();
                    if (msgChecksum == calcChecksum) {
                        msgCount++;
                    	_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "RMTD2081", 
                    			"Action {0}: Received message {1}, length = {2}", _id, msgCount, (int)bMsg.getBodyLength()-8);
                    } else if (msgChecksum != calcChecksum) {
                    	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2082", "Action {0}: Message CRC and calculated CRC do not match.", _id);
                        break;
                    }
                } else {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "JMST4002", 
                        "Action {0}: ReceiveMessageCRC does not support message type: {1}", _id, msg.getClass().getSimpleName());
                }
            }
        } else if (_listenerID != null) {
            final JmsMessageListener listener = (JmsMessageListener) _dataRepository.getVar(_listenerID);
            if(listener == null){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                        "Action {0}: Failed to locate the MessageListener object ({1}).", _id,_listenerID);
                return false;
            }

            while ((msg = listener.receive(_timeout)) != null) {
                if (msg instanceof BytesMessage) {
                    BytesMessage bMsg = (BytesMessage) msg;
                    byte[] bytes = new byte[(int)bMsg.getBodyLength()-8];
                    long msgChecksum = bMsg.readLong();
                    bMsg.readBytes(bytes);
                    Checksum checksum = new CRC32();
                    checksum.update(bytes,0,bytes.length);
                    long calcChecksum = checksum.getValue();
                    if (msgChecksum == calcChecksum) {
                        msgCount++;
                    	_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "RMTD2081", 
                    			"Action {0}: Received message {1}, length = {2}", _id, msgCount, (int)bMsg.getBodyLength()-8);
                    } else if (msgChecksum != calcChecksum) {
                    	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2082", "Action {0}: Message CRC and calculated CRC do not match.", _id);
                    	break;
                    }
                } else {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "JMST4002", 
                        "Action {0}: ReceiveMessageCRC does not support message type: {1}", _id, msg.getClass().getSimpleName());
                }
            }
        }
        if (_numMessages > 0) {
	        if (_numMessages < msgCount) {
	            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2078", "Action {0}: Receive more messages than expected. {1} : {2}", _id, _numMessages, msgCount);
	            return false;
	        } else if (_numMessages > msgCount) {
	            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2078", "Action {0}: Receive less messages than expected. {1} : {2}", _id, _numMessages, msgCount);
	            return false;
	        } else if (_numMessages == msgCount) {
	            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2080", "Action {0}: Received all {1} expected messages", _id, _numMessages);
	        }
        } else {
        	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2080", "Action {0}: Received {1} messages.", _id, msgCount);
        }
        
        return true;
    }

}
