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

import java.io.UnsupportedEncodingException;
import java.util.Enumeration;

import javax.jms.BytesMessage;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.TextMessage;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;


public class PollBrowserAction extends ApiCallAction {
    private final String  _enumID;
    private final int     _maxID;
    private final boolean _checkIDS;
    private final String  _messageIDS;
    private final String  _existingIDS;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public PollBrowserAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);    
        _enumID = _actionParams.getProperty("enum_id");
        if (_enumID == null){
            throw new RuntimeException("enum_id is not defined for " + this.toString());
        }
        _maxID = Integer.parseInt(_actionParams.getProperty("maxID", "0"));
        _checkIDS = Boolean.parseBoolean(_actionParams.getProperty("checkIDS", "false"));
        _messageIDS = _actionParams.getProperty("messageIDS");
        _existingIDS = _actionParams.getProperty("existing_msgIDS", "false");
    }

    protected boolean invokeApi() throws JMSException {
        final Enumeration<?> qBrowserEnum = (Enumeration<?>) _dataRepository.getVar(_enumID);
        int messageID;
        int [] messageIDS = null;
        if (_maxID > 0) {
                messageIDS = new int[_maxID];    
        }

        if (qBrowserEnum == null){
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041", 
                    "Action {0}: Failed to locate the Enumerator object ({1}).", _id,_enumID);
            return false;
        }
        
        while (qBrowserEnum.hasMoreElements()) {
            Message msg = (Message) qBrowserEnum.nextElement();
            if (_maxID > 0) {
                messageID = msg.getIntProperty("ID");
                if (messageID > _maxID) {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2078", "Action {0}: Browsed message with ID {1} higher than max ID expected {2}", _id, messageID, _maxID);
                }
                if (messageIDS[messageID-1] > 0) {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2074", "Action {0}: Browsing duplicate message, ID={1}, Redelivered={2}", _id, messageID, msg.getJMSRedelivered());
                } else {
                    messageIDS[messageID-1] = messageID;
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2042", "Action {0}: Browsing message, JMSid={1}, ID={2}, messageIDSs[{3}]={4}, Redelivered={5}", _id, msg.getJMSMessageID(), messageID, messageID-1, messageIDS[messageID-1], msg.getJMSRedelivered());
                }
                if (_maxID == messageID) {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2075", "Action {0}: Browsing last expected message, ID={1}", _id, messageID);
                }
            }
            if (msg instanceof TextMessage) {
                TextMessage tMsg = (TextMessage) msg;
                _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2039", "Action {0}: Text Message content: {1}", _id, tMsg.getText());
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
                _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2039", "Action {0}: Bytes Message content: {1}", _id, string);
            } else {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "JMST4002", 
                    "Action {0}: The getText API is not valid for message of type: {1}", _id, msg.getClass().getSimpleName());
            }
        }

        if (_checkIDS == true && _maxID > 0) {
            int fail = 0;
            int index = 0;
            if (_existingIDS.equals("true")) {
                int [] oldMessages = (int []) _dataRepository.getVar(_messageIDS);
                index = oldMessages.length;
            }
            for (int i=index; i<_maxID; i++) {
                if (messageIDS[i] == 0) {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2076", "Action {0}: Message ID={1} was not found.", _id, i+1);
                    fail = 1;
                }
            }
            if (fail == 1) {
                throw new JMSException("Failed to find 1 or more messages","JMSTDBrowseMissing");
            }
        }

        _dataRepository.storeVar(_messageIDS, messageIDS);
        return true;
    }
}
