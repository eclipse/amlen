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
/**
 */
package com.ibm.ima.admin.request;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;

import com.ibm.ima.admin.IsmClientType;

/**
 *
 */
public class IsmConfigGetLogLevelRequest extends IsmConfigRequest {
    @JsonSerialize
    private String Item;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    public IsmConfigGetLogLevelRequest(String userId) {
        this(userId, "LogLevel");
    }

    public IsmConfigGetLogLevelRequest(String userId, String type) {
        this.Action = "GET";
        this.Component = "Server";
        if (type != null) {
            this.Item = type;
            if (type.equals("MQConnectivityLog")) {
                setClientType(IsmClientType.ISM_ClientType_MQConnectivityConfiguration);
                this.Component = "MQConnectivity";
            }
        }
        this.User = userId;
    }

    @Override
    public String toString() {
        return "IsmConfigGetLogLevelRequest [Item=" + Item + ",  Action=" + Action + ", Component="
                + Component + ", User=" + User + "]";
    }
}
