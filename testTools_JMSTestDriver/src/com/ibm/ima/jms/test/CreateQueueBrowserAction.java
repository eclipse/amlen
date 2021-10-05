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
import javax.jms.Session;
import javax.jms.Queue;
import javax.jms.QueueBrowser;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;


public class CreateQueueBrowserAction extends ApiCallAction {
    private final String _structureID;
    private final String _sessID;
    private final String _queueID;
    private final String _selector;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public CreateQueueBrowserAction(Element config, DataRepository dataRepository,
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
        _queueID = _actionParams.getProperty("queue_id");
        if(_queueID == null){
            throw new RuntimeException("dest_id is not defined for " + this.toString());
        }
        if (_apiParameters.getProperty("selector") != null) {
        	_selector = _apiParameters.getProperty("selector");
        } else {
        	_selector = _actionParams.getProperty("selector");
        }
    }

    protected boolean invokeApi() throws JMSException {
        final QueueBrowser qBrowser;
        final Queue queue = (Queue) _dataRepository.getVar(_queueID);
        final Session ses = (Session) _dataRepository.getVar(_sessID); 
        
        if(ses == null){
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                "Action {0}: Failed to locate the Session object ({1}).", _id,_sessID);
            return false;
        }
        if(queue == null){
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                "Action {0}: Failed to locate the Queue object ({1}).", _id,_queueID);
            return false;
        }
        
        if (null == _selector) {
            qBrowser = ses.createBrowser(queue);
        } else {
            qBrowser = ses.createBrowser(queue, _selector);
        }

        _dataRepository.storeVar(_structureID, qBrowser);
        return true;
    }
}
