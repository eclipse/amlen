/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.admin.request;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;

import com.ibm.ima.admin.IsmClientType;
import com.ibm.ima.resources.data.LogLevel;
import com.ibm.ima.resources.data.LogLevel.ValidLogLevel;

public class IsmConfigSetLogLevelRequest extends IsmConfigRequest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @JsonSerialize
    private String Type;
    @JsonSerialize
    private String Item;

    @JsonSerialize
    private String Value = ValidLogLevel.getDefaultLogLevel();

    public IsmConfigSetLogLevelRequest(String userId) {
        this(userId, null);
    }

    /**
     * @param LogLevel
     */
    public IsmConfigSetLogLevelRequest(String userId, LogLevel logLevel) {
        this.Action = "SET";
        this.Component = "Server";
        this.User = userId;
        this.Type = "Enum";
        this.Item = "LogLevel";

        if (logLevel != null) {
            this.Value = logLevel.getLogLevel();
            if (logLevel.getLogType() != null) {
                this.Item = logLevel.getLogType();
                if (logLevel.isMQConnectivityLogType()) {
                    setClientType(IsmClientType.ISM_ClientType_MQConnectivityConfiguration);
                    this.Component = "MQConnectivity";
                }
            }
        }
    }

    @Override
    public String toString() {
        return "IsmConfigSetLogLevelRequest [Type=" + Type + ", Item=" + Item
                + ", Value=" + Value + "]";
    }

}
