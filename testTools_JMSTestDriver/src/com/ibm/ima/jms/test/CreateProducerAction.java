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

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.Session;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class CreateProducerAction extends ApiCallAction {
    private final String _structureID;
    private final String _sessID;
    private final String _destID;
    private final String _deliveryMode;
    private final String _priority;
    private final String _ttl;
    private final String _disableMsgTimestamp;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public CreateProducerAction(Element config,
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _structureID = _actionParams.getProperty("structure_id");
        if (_structureID == null) {
            throw new RuntimeException("structure_id is not defined for "
                    + this.toString());
        }
        _sessID = _actionParams.getProperty("session_id");
        if (_sessID == null) {
            throw new RuntimeException("session_id is not defined for "
                    + this.toString());
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
        _disableMsgTimestamp = _actionParams.getProperty("disableMsgTimestamp");
    }

    protected boolean invokeApi() throws JMSException {
        final MessageProducer producer;
        final Session ses = (Session) _dataRepository.getVar(_sessID);
        Destination dest = null;
        int delMode = 0;
        int priority = 0;
        long ttl = 0;
        boolean disMsgTimestamp = true;

        if(ses == null){
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                    "Action {0}: Failed to locate the Session object ({1}).", _id,_sessID);
            return false;
        }
        if(_destID != null){
            dest = (Destination) _dataRepository.getVar(_destID);
            if (dest == null){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                        "Action {0}: Failed to locate the Destination object ({1}).", _id,_destID);
                return false;
            }
        }

        producer = ses.createProducer(dest);
        
        if(_deliveryMode != null){
            delMode = Integer.parseInt(_deliveryMode);
            producer.setDeliveryMode(delMode);
        }
        if(_priority != null){
            priority = Integer.parseInt(_priority);
            producer.setPriority(priority);
        }
        if(_ttl != null){
            ttl = Long.parseLong(_ttl);
            producer.setTimeToLive(ttl);
        }
        if(_disableMsgTimestamp != null){
            disMsgTimestamp = Boolean.parseBoolean(_disableMsgTimestamp);
            producer.setDisableMessageTimestamp(disMsgTimestamp);
        }
        
        _dataRepository.storeVar(_structureID, producer);
        return true;
    }

}
