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
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.Session;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;


public class SetMessageListenerAction extends ApiCallAction {
    private final String _listenerID;
    private final String _sessID;
    private final String _consID;
    
    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public SetMessageListenerAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);    
        _listenerID = _actionParams.getProperty("listener_id");
        if(_listenerID == null){
            throw new RuntimeException("listener_id is not defined for " + this.toString());
        }
        _sessID = _actionParams.getProperty("session_id");
        _consID = _actionParams.getProperty("consumer_id");
        if((_consID == null) && (_sessID == null)){
            throw new RuntimeException("both consumer_id and session_id are not defined for " + this.toString()+". One or the other (but not both) must be defined.");
        }
        
        if((_consID != null) && (_sessID != null)){
            throw new RuntimeException("both consumer_id and session_id are defined for " + this.toString() +". Only one or the other can be used to set a message listener.");
        }
        
    }

    protected boolean invokeApi() throws JMSException {
        final MessageConsumer consumer;
        final MessageListener listener;
        if(_listenerID.compareToIgnoreCase("null") != 0){
            listener = (MessageListener) _dataRepository.getVar(_listenerID);
            if(listener == null){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                        "Action {0}: Failed to locate the MessageListener object ({1}).", _id,_listenerID);
                return false;
            }
        }else{
            listener = null;
        }
        if(_sessID != null){
            final Session ses = (Session) _dataRepository.getVar(_sessID);        
            if(ses == null){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                        "Action {0}: Failed to locate the Session object ({1}).", _id,_sessID);
                return false;
            }
            ses.setMessageListener(listener);
            return true;
        }
        consumer = (MessageConsumer) _dataRepository.getVar(_consID);
        if (null == consumer) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                    "Action {0}: Failed to locate the consumer object ({1}).", _id,_consID);
            return false;
        }
        consumer.setMessageListener(listener);
        return true;
    }

}
