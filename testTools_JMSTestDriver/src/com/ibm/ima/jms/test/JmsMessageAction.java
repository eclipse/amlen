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

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public abstract class JmsMessageAction extends ApiCallAction{

    private final String _msgID;

    protected JmsMessageAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _msgID = _actionParams.getProperty("message_id");
        if (_msgID == null) {
            throw new RuntimeException("message_id is not defined for "
                    + this.toString());
        }
    }
    
    protected boolean invokeApi() throws JMSException {
        final Message msg = (Message) _dataRepository.getVar(_msgID);
        if (msg == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                    "Action {0}: Failed to locate the Message object ({1}).",_id, _msgID);
            return false;
        }
        return invokeMessageApi(msg);
    }    
    
    protected abstract boolean invokeMessageApi(Message msg) throws JMSException;
}
