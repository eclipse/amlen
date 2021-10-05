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
import javax.jms.Session;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class SessionRollbackAction extends ApiCallAction {
    private final String _sessionID;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public SessionRollbackAction(Element config,
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _sessionID = _actionParams.getProperty("session_id");
        if (_sessionID == null) {
            throw new RuntimeException("session_id is not defined for "
                    + this.toString());
        }
    }

    protected boolean invokeApi() throws JMSException {
        final Session ses = (Session) _dataRepository.getVar(_sessionID);

        if (ses == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                    "Action {0}: Failed to locate the Session object ({1}).",_id, _sessionID);
            return false;
        }
        ses.rollback();
        return true;
    }

}
