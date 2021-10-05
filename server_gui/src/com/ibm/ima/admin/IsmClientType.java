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

package com.ibm.ima.admin;

/**
 * This defines ISM server client types
 */
public enum IsmClientType {
    ISM_ClientType_Configuration (9089, "admin.ism.ibm.com", "Eclipse Amlen"),
    ISM_ClientType_Monitoring (9089, "monitoring.ism.ibm.com", "Eclipse Amlen"),
    ISM_ClientType_MQConnectivityConfiguration (9086, "admin.ism.ibm.com", "MQ Connectivity"),
    ISM_ClientType_MQConnectivityMonitoring (9086, "monitoring.ism.ibm.com", "MQ Connectivity");

    private final int port;
    private final String protocol;
    private final String serverName;

    private IsmClientType (final int port, final String protocol, final String serverName) {
        this.port = port;
        this.protocol = protocol;
        this.serverName = serverName;
    }

    /**
     * @return the port
     */
    public int getPort() {
        return port;
    }

    /**
     * @return the protocol
     */
    public String getProtocol() {
        return protocol;
    }

    /**
     * @return the serverName
     */
    public String getServerName() {
        return serverName;
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


}
