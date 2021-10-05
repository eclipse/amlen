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

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageProducer;
import javax.jms.Destination;
import javax.jms.TextMessage;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;


public class SendMessageAction extends ApiCallAction {
    private final String _producerID;
    private final String _msgID;
    private final String _destID;
    private final String _deliveryMode;
    private final String _priority;
    private final String _ttl;
    private final String _incrID;
    private final boolean _serialNumberInPayload;
    private int serial;
    private String origTextPayload;
    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public SendMessageAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);    
        serial = 0;
        //_structureID = _actionParams.getProperty("structure_id");
        //if(_structureID == null){
        //    throw new RuntimeException("structure_id is not defined for " + this.toString());
        //}
        _msgID = _actionParams.getProperty("message_id");
        if(_msgID == null){
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
        _serialNumberInPayload = Boolean.parseBoolean(_actionParams.getProperty("serialNumberInPayload", "false"));
        _incrID = _actionParams.getProperty("incrID");
    }

    protected boolean invokeApi() throws JMSException {
        final Message msg = (Message) _dataRepository.getVar(_msgID);
        final MessageProducer producer = (MessageProducer) _dataRepository.getVar(_producerID);
        int delMode = 0;
        int priority = 0;
        long ttl = 0;
        Destination dest = null;
        int msgID;

        if(msg == null){
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", "Failed to locate the Message object ({0}).", _msgID);
            return false;
        }
        if(producer == null){
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", "Failed to locate the MessageProducer object ({0}).", _producerID);
            return false;
        }
        
       // _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD9999", "serial? {0} instanceofTextMessage? {1}", _serialNumberInPayload, msg instanceof TextMessage);
        if (_serialNumberInPayload && msg instanceof TextMessage) {        	
        	TextMessage tm = (TextMessage) msg;
        	if (serial == 0)
        		origTextPayload = tm.getText();
        	tm.setText(origTextPayload + " " + serial++);
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

        if (producer.getDestination() != null) {
            if (_deliveryMode != null || _priority != null || _ttl != null) {
                producer.send(msg, (_deliveryMode != null) ? delMode : producer.getDeliveryMode(),
                        (_priority != null) ? priority : producer.getPriority(),
                        (_ttl != null) ? ttl : producer.getTimeToLive());
            }
            else {
                producer.send(msg);
            }
        } else {
            if (dest == null){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", "Failed to locate the MessageProducer object({0}).", _destID);
                return false;
            }
            if (_deliveryMode != null || _priority != null || _ttl != null) {
                producer.send(dest, msg, (_deliveryMode != null) ? delMode : producer.getDeliveryMode(),
                        (_priority != null) ? priority : producer.getPriority(),
                        (_ttl != null) ? ttl : producer.getTimeToLive());
            }
            else {
                producer.send(dest, msg);
            }
        }
        
        if (_incrID != null) {
            int incrID = Integer.parseInt(_incrID);
            msgID = msg.getIntProperty("ID");
            msg.setIntProperty("ID", msgID+incrID);
        }

        return true;
    }

}
