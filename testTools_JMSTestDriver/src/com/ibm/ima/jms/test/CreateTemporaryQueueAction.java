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
import javax.jms.Queue;
import javax.jms.JMSException;
import javax.jms.Session;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class CreateTemporaryQueueAction extends ApiCallAction{
    private final String    _structureID;
    private final String    _sessID;
    private final String    _verify;
    private final TrWriter  _trWriter;

    public CreateTemporaryQueueAction(Element config, DataRepository dataRepository,
            TrWriter trWriter){
        super(config, dataRepository, trWriter);
        _trWriter = trWriter;
        _structureID = _actionParams.getProperty("structure_id");
        if(_structureID == null){
            throw new RuntimeException("structure_id is not defined for " + this.toString());
        }
        _sessID = _actionParams.getProperty("session_id");
        if(_sessID == null){
            throw new RuntimeException("session_id is not defined for " + this.toString());
        }
        _verify = _actionParams.getProperty("verifyQueue");
    }

    protected boolean invokeApi() throws JMSException {
        final Session session = (Session) _dataRepository.getVar(_sessID);
        final Destination destination;
        
        if (session == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                    "Action {0}: Failed to locate the connection object ({1}).", _id,_sessID);
            return false;
        }
        
        _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "RMTD9224", 
                "Action {0} calling ImaSession.createTemporaryQueue().", this.toString());
        destination = session.createTemporaryQueue();
        _dataRepository.storeVar(_structureID, destination);
        
        if (_verify != null) {
            if (!((Queue)destination).getQueueName().startsWith(_verify)) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD9225",
                        "Action {0}: Temporary Queue verification failed: Actual Queue = {1} : Expected Queue = {2}", this.toString(), ((Queue)destination).getQueueName(), _verify);
                return false;
            }
        }

        return true;
    }
}

