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

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.Map;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.Session;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;
import com.ibm.ima.jms.test.jca.TestProps;

public class CreateTestPropsMessageAction extends ApiCallAction implements JmsMessageUtils {
    private final String         _structureID;
    private final String         _sessionID;
    private final String         _param; 

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public CreateTestPropsMessageAction(Element config,
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _structureID = _actionParams.getProperty("structure_id");
        if (_structureID == null) {
            throw new RuntimeException("structure_id is not defined for "
                    + this.toString());
        }
        _sessionID = _actionParams.getProperty("session_id");
        if (_sessionID == null) {
            throw new RuntimeException("session_id is not defined for "
                    + this.toString());
        }
        _param = _apiParameters.getProperty("msgParam");
    }

    protected boolean invokeApi() throws JMSException {
        final Session session = (Session) _dataRepository.getVar(_sessionID);
        final Message msg; 
        if (session == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                    "Action {0}: Failed to locate the Connection object ({1}).", _id,_sessionID);
            return false;
        }
        FileInputStream f = null;
        try {
            f = new FileInputStream("tests.properties");
        } catch (FileNotFoundException e) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,"RMTD0000","couldn't find file");
            _trWriter.writeException(e);
        }
        
        Map<Integer, TestProps> testCases = TestProps.readCases(f);
        
        if(_param == null) {
            TestProps test = testCases.get(1);
            msg = session.createObjectMessage(test);
        }else{
            TestProps test = testCases.get(Integer.parseInt(_param));
            if (test == null) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                        "Action {0}: Failed to locate the API parameter ({1}).",_id, _param);
                return false;
            }
            msg = session.createObjectMessage(test);
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO,"RMTD0000",test.toString());
        }

        _dataRepository.storeVar(_structureID, msg);
        return true;
    }

}
