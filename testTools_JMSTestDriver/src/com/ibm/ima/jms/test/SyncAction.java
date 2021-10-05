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
/**
 * 
 */
package com.ibm.ima.jms.test;

import org.w3c.dom.Element;

import test.llm.sync.Request;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

/**
 * 
 */
class SyncAction extends Action {
    final Request requestType;
    final String  condition;
    final int     value;
    final int     timeout;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public SyncAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        try {
            requestType = new Request(_actionParams.getProperty("request"));
            condition = _actionParams.getProperty("condition");
            value = Integer.parseInt(_actionParams.getProperty("value", "-1"));
            timeout = Integer.parseInt(_actionParams.getProperty("timeout",
                    "120000"));
        } catch (Throwable t) {
            trWriter.writeException(t);
            throw new RuntimeException(t);
        }

    }

    /*
     * (non-Javadoc)
     * 
     * @see com.ibm.mds.rm.test.Action#run()
     */
    @Override
    protected final boolean run() {
        try {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "RMTS0001",
                    "Action {0}: Entrance to Sync Client Call", _id);
            String response = JmsSyncClient.invokeSyncRequest(requestType,
                    condition, value, timeout, _trWriter);
            _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "RMTS0001",
                    "Action {0}: Return from Sync Client Call. response={1}",
                    _id, response);
            return true;
        } catch (Throwable e) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2100",
                    "Action {0} failed. The reason is: {1}", _id,
                    e.getMessage());
            _trWriter.writeException(e);
            return false;
        }
    }

}
