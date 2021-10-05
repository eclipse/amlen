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

import javax.jms.Connection;
import javax.jms.TopicConnection;
import javax.jms.QueueConnection;
import javax.jms.JMSException;
import javax.jms.Session;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class CreateSessionAction extends ApiCallAction {
    private final String  _structureID;
    private final String  _connID;
    private final Boolean _transacted;
    private final int     _ackMode;
    private final String  _type;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public CreateSessionAction(Element config,
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _structureID = _actionParams.getProperty("structure_id");
        if (_structureID == null) {
            throw new RuntimeException("structure_id is not defined for "
                    + this.toString());
        }
        _connID = _actionParams.getProperty("conn_id");
        if (_connID == null) {
            throw new RuntimeException("conn_id is not defined for "
                    + this.toString());
        }
        if (_apiParameters.getProperty("transacted") != null) {
        	_transacted = (Integer.parseInt(_apiParameters.getProperty("transacted", "0")) != 0);
        } else {
        	_transacted = (Integer.parseInt(_actionParams.getProperty("transacted", "0")) != 0);
        }
        if (_apiParameters.getProperty("ack_mode") != null) {
        	_ackMode = Integer.parseInt(_apiParameters.getProperty("ack_mode","1"));
        } else {
        	_ackMode = Integer.parseInt(_actionParams.getProperty("ack_mode","1"));
        }
        
        _type = _actionParams.getProperty("type", "default");
    }

    protected boolean invokeApi() throws JMSException {
        final Session ses;
        if (_type.equals("default")) {
            final Connection con = (Connection) _dataRepository.getVar(_connID);
            if (con == null) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                        "Action {0}: Failed to locate the Connection object ({1}).",_id, _connID);
                return false;
            }
            // TODO: Need to define API params for createSession args
            if (_ackMode == 3) {
                ses = con.createSession(_transacted, Session.DUPS_OK_ACKNOWLEDGE);
            } else if (_ackMode == 2) {
                ses = con.createSession(_transacted, Session.CLIENT_ACKNOWLEDGE);
            } else {
                ses = con.createSession(_transacted, Session.AUTO_ACKNOWLEDGE);
            }
            _dataRepository.storeVar(_structureID, ses);
        } else if (_type.equals("queue")) {
            final QueueConnection con = (QueueConnection) _dataRepository.getVar(_connID);
            if (con == null) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                        "Action {0}: Failed to locate the Connection object ({1}).",_id, _connID);
                return false;
            }
            if (_ackMode == 3) {
                ses = con.createQueueSession(_transacted, Session.DUPS_OK_ACKNOWLEDGE);
            } else if (_ackMode == 2) {
                ses = con.createQueueSession(_transacted, Session.CLIENT_ACKNOWLEDGE);
            } else {
                ses = con.createQueueSession(_transacted, Session.AUTO_ACKNOWLEDGE);
            }
            _dataRepository.storeVar(_structureID, ses);
        } else if (_type.equals("topic")) {
            final TopicConnection con = (TopicConnection) _dataRepository.getVar(_connID);
            if (con == null) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                        "Action {0}: Failed to locate the Connection object ({1}).",_id, _connID);
                return false;
            }
            if (_ackMode == 3) {
                ses = con.createTopicSession(_transacted, Session.DUPS_OK_ACKNOWLEDGE);
            } else if (_ackMode == 2) {
                ses = con.createTopicSession(_transacted, Session.CLIENT_ACKNOWLEDGE);
            } else {
                ses = con.createTopicSession(_transacted, Session.AUTO_ACKNOWLEDGE);
            }
            _dataRepository.storeVar(_structureID, ses);
        } else {
            throw new RuntimeException("Type "+_type+" is not valid. Valid types are default, topic and queue.");
        }
        return true;
    }

}
