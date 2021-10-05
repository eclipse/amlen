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
package com.ibm.ima.jms.test.jca;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.ObjectMessage;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;
import com.ibm.ima.jms.test.*;

public class GetTestPropsMsgAction extends JmsMessageAction implements JmsMessageUtils{
    private final int        _verifyValue; 
    
    public GetTestPropsMsgAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _verifyValue = Integer.parseInt(_actionParams.getProperty("verifyValue"));
    }

    @Override
    public boolean invokeMessageApi(Message msg) throws JMSException {
        
        if (!(msg instanceof ObjectMessage)) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", 
                    "Action {0}: Message is not of type ObjectMessage.", _id);
            return false;
        }
        ObjectMessage om = (ObjectMessage) msg;
        Object o = om.getObject();
        if (!(o instanceof TestProps)) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", 
                    "Action {0}: Object from message is not of type TestProps.", _id);
            return false;
        }
        
        TestProps tp = (TestProps) o;
        
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "Action {0}: TestProps Contents: {1}", _id, tp.toString());
        
        if (tp.testNumber != _verifyValue) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", 
                    "Action {0}: TestProps.testNumber did not match the expected value. Expected: {1}, Received: {2}",
                    _id, _verifyValue, tp.testNumber);
            return false;
        }
        
        return true;
    }
}
