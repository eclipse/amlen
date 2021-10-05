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

import javax.jms.BytesMessage;
import javax.jms.JMSException;
import javax.jms.Message;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class GetMessageBodyLengthAction extends GetSetAction {
 
    protected GetMessageBodyLengthAction(Element config,JmsActionType actionType, 
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, actionType,dataRepository, trWriter);
    }
 
    @Override
    protected Object get(Message msg, JmsValueType type) throws JMSException {
        final BytesMessage bMsg;
        if (msg instanceof BytesMessage) {
            bMsg = (BytesMessage) msg;
            return Long.valueOf(bMsg.getBodyLength());
        }else{
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "JMST4001", 
                    "Action {0}: The getBodyLength API is not valid for message of type: ", _id, msg.getClass().getSimpleName());
            return null;
        }
    }
  
    @Override
    protected boolean set(Message msg, JmsValueType type) throws JMSException {
        throw new RuntimeException("Invalid action");
    }

}
