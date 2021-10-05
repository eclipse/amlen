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
import javax.jms.JMSException;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class CloseConnectionAction extends ApiCallAction {
    private final String _connID;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public CloseConnectionAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _connID = _actionParams.getProperty("conn_id");
        if (_connID == null) {
            throw new RuntimeException("conn_id is not defined for "
                    + this.toString());
        }
    }

    protected boolean invokeApi() throws JMSException {
        final Connection con = (Connection) _dataRepository.getVar(_connID);
        if (con == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                    "Action {0}: Failed to locate the Connection object ({1}).",_id, _connID);
            return false;
        }
        con.close();
        return true;
    }

}
