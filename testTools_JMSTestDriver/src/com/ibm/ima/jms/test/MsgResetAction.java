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
import javax.jms.StreamMessage;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class MsgResetAction extends JmsMessageAction {
    
    protected MsgResetAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
    }

    @Override
    protected boolean invokeMessageApi(Message msg) throws JMSException {
        if (msg instanceof BytesMessage) {
            BytesMessage bmsg = (BytesMessage) msg;
            bmsg.reset();
            return true;
        }
        if (msg instanceof StreamMessage) {
            StreamMessage smsg = (StreamMessage) msg;
            smsg.reset();
            return true;
        }
        _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "JMST4001", 
                "Action {0}: The reset API is not valid for message of type: ", _id, msg.getClass().getSimpleName());
        return false;
    }


}
