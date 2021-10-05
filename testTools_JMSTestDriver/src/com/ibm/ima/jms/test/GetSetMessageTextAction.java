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

import java.io.IOException;
import java.io.UnsupportedEncodingException;

import javax.jms.BytesMessage;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.TextMessage;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class GetSetMessageTextAction extends GetSetAction {
    
    protected GetSetMessageTextAction(Element config,JmsActionType actionType, 
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, actionType, JmsValueType.String, dataRepository, trWriter);
    }
    
    @Override
    protected Object get(Message msg, JmsValueType type) throws JMSException {
        if (msg instanceof TextMessage) {
            TextMessage tMsg = (TextMessage) msg;
            return tMsg.getText();
        } else if (msg instanceof BytesMessage) {
            BytesMessage bMsg = (BytesMessage) msg;
            byte[] bytes = new byte[(int)bMsg.getBodyLength()];
            bMsg.readBytes(bytes);
            String string = null;
            try {
                string = new String(bytes, "UTF-8");
            } catch (UnsupportedEncodingException e) {    // Should not get here
                _trWriter.writeException(e);
                return false;
            }
            return string;//bMsg.readUTF();
        }
        _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "JMST4002", 
                "Action {0}: The getText API is not valid for message of type: {1}", _id, msg.getClass().getSimpleName());
        return null;
    }
    
    @Override
    protected boolean set(Message msg, JmsValueType type) throws JMSException {
        if (msg instanceof TextMessage) {
            TextMessage tMsg = (TextMessage) msg;
            String text;
            try {
                text = stringValue();
            } catch (IOException e) {
                _trWriter.writeException(e);
                return false;
            }
            tMsg.setText(text);
            return true;
        } else if (msg instanceof BytesMessage) {
            BytesMessage bMsg = (BytesMessage) msg;
            byte[] bytes = null;
            try {
                bytes = stringValue().getBytes("UTF-8");
            } catch (UnsupportedEncodingException e) {    // Should not get here
                _trWriter.writeException(e);
                return false;
            } catch (IOException e) {
                _trWriter.writeException(e);
                return false;
            }
            bMsg.writeBytes(bytes);
            return true;
        }
        _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "JMST4003", 
                "Action {0}: The getText API is not valid for message of type: ", _id, msg.getClass().getSimpleName());
        return false;
    }

}
