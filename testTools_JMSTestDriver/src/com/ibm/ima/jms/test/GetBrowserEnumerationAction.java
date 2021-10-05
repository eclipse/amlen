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

import java.util.Enumeration;

import javax.jms.JMSException;
import javax.jms.QueueBrowser;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;


public class GetBrowserEnumerationAction extends ApiCallAction {
    private final String _structureID;
    private final String _browserID;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public GetBrowserEnumerationAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);    
        _structureID = _actionParams.getProperty("structure_id");
        if(_structureID == null){
            throw new RuntimeException("structure_id is not defined for " + this.toString());
        }
        _browserID = _actionParams.getProperty("browser_id");
        if(_browserID == null){
            throw new RuntimeException("session_id is not defined for " + this.toString());
        }
    }

    protected boolean invokeApi() throws JMSException {
        final QueueBrowser qBrowser = (QueueBrowser) _dataRepository.getVar(_browserID);
        
        if(qBrowser == null){
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                "Action {0}: Failed to locate the QueueBrowser object ({1}).", _id,_browserID);
            return false;
        }
        
        Enumeration<?> messages = qBrowser.getEnumeration();
        
        _dataRepository.storeVar(_structureID, messages);
        return true;
    }
}
