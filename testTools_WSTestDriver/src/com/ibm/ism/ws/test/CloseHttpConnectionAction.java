/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ism.ws.test;

import org.w3c.dom.Element;

import com.ibm.ism.ws.IsmHTTPConnection;
import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class CloseHttpConnectionAction extends ApiCallAction {
    private final String connectID;
    
    public CloseHttpConnectionAction(Element config, DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        connectID = _actionParams.getProperty("connection_id");
        if (connectID == null) {
            throw new RuntimeException("connection_id is not defined for " + this);
        }
    }
    
    protected boolean invokeApi() throws IsmTestException {
        CreateHttpConnectionAction conact = (CreateHttpConnectionAction)_dataRepository.getVar(connectID);
        if (conact == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST3612",
                    "Action {0}: Failed to locate the Connection object ({1}).",_id, connectID);
            return false;
        }
        IsmHTTPConnection connect = conact.getConnect();
        if (connect == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST3613",
                    "Action {0}: The connection does not exist ({1}).",_id, connectID);
            return false;    
        }
        if (connect.isConnected()) {
            try {
                connect.disconnect();
            } catch (Exception e) {
            }
        }    
        return true;
    }

}
