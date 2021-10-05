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
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.Topic;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;


public class CreateConsumerAction extends ApiCallAction {
    private final String _structureID;
    private final String _sessID;
    private final String _destID;
    private final String _durableName;
    private final String _selector;
    private final String _noLocal;
    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public CreateConsumerAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);    
        _structureID = _actionParams.getProperty("structure_id");
        if(_structureID == null){
            throw new RuntimeException("structure_id is not defined for " + this.toString());
        }
        _sessID = _actionParams.getProperty("session_id");
        if(_sessID == null){
            throw new RuntimeException("session_id is not defined for " + this.toString());
        }
        _destID = _actionParams.getProperty("dest_id");
        if(_destID == null){
            throw new RuntimeException("dest_id is not defined for " + this.toString());
        }
        if (_apiParameters.getProperty("durableName") != null) {
        	_durableName = _apiParameters.getProperty("durableName");
        } else {
        	_durableName = _actionParams.getProperty("durableName");
        }
        if (_apiParameters.getProperty("selector") != null) {
        	_selector = _apiParameters.getProperty("selector");
        } else {
        	_selector = _actionParams.getProperty("selector");
        }
        if (_apiParameters.getProperty("nolocal") != null) {
        	_noLocal = _apiParameters.getProperty("nolocal");
        } else {
        	_noLocal = _actionParams.getProperty("nolocal");
        }
    }

    protected boolean invokeApi() throws JMSException {
        final MessageConsumer consumer;
        final Session ses = (Session) _dataRepository.getVar(_sessID);
        final Destination dest = (Destination) _dataRepository.getVar(_destID);
        Boolean noLocal = false; 
        
        if(ses == null){
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                    "Action {0}: Failed to locate the Session object ({1}).", _id,_sessID);
            return false;
        }
        if(dest == null){
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                    "Action {0}: Failed to locate the Destination object ({1}).", _id,_destID);
            return false;
        }
        if(_noLocal != null){
            noLocal = Boolean.parseBoolean(_noLocal);
        }
        if(_durableName != null){
            if (!(dest instanceof Topic)) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD3041", 
                        "Action {0}: The Destination object ({1}) for durable subscriber is not a topic.", _id,_destID);
                return false;
            } else if (null == _selector && null == _noLocal) {
                Topic topic = (Topic) dest;
                consumer = ses.createDurableSubscriber(topic,_durableName);                
            } else {
                Topic topic = (Topic) dest;
                consumer = ses.createDurableSubscriber(topic,_durableName, "null".equals(_selector) ? null : _selector, noLocal);                
            }
        } else if (null == _selector && null == _noLocal) {
            consumer = ses.createConsumer(dest);
        } else {
            consumer = ses.createConsumer(dest, "null".equals(_selector) ? null : _selector, noLocal);
        }
        _dataRepository.storeVar(_structureID, consumer);
        return true;
    }

}
