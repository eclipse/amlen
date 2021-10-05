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

import java.io.Serializable;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.Session;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class CreateMessageAction extends ApiCallAction implements JmsMessageUtils {
    private final String         _structureID;
    private final String         _sessionID;
    private final JmsMessageType _msgType;
    private final String         _param; 

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public CreateMessageAction(Element config,
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
        _msgType = JmsMessageType.valueOf(_apiParameters.getProperty("msgType","SIMPLE"));
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
        switch (_msgType) {
        case TEXT:
            if(_param == null)
                msg = session.createTextMessage();
            else
                msg = session.createTextMessage(_param);
            break;
        case OBJECT:
            if(_param == null)
                msg = session.createObjectMessage();
            else{
                Serializable obj = (Serializable) _dataRepository.getVar(_param);
                if (obj == null) {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                            "Action {0}: Failed to locate the API parameter ({1}).",_id, _param);
                    return false;
                }
                msg = session.createObjectMessage(obj);
            }
            break;
        case MAP:
            msg = session.createMapMessage();
            break;
        case STREAM:
            msg = session.createStreamMessage();
            break;
        case BYTES:
            msg = session.createBytesMessage();
            break;
        default:
            msg = session.createMessage();
            break;
        }
        _dataRepository.storeVar(_structureID, msg);
        return true;
    }

}
