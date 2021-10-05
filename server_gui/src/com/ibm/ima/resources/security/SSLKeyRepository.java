/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.resources.security;

import com.fasterxml.jackson.annotation.JsonAnySetter;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.util.IsmLogger;

public class SSLKeyRepository {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @JsonProperty("SslKeyFile")
    private String sslKeyFile;

    @JsonProperty("passwordFile")
    private String passwordFile;
    
    public SSLKeyRepository() {}
    
    /*
     * Handle any property we don't know about.
     */
    @JsonAnySetter
    public void handleUnknown(String key, Object value) {
        logger.trace(this.getClass().getCanonicalName(), "handleUnknown", "Unknown property " + key + " with value " + value + " received");
    }

    /**
     * @return the Password file
     */
    @JsonProperty("passwordFile")
    public String getPasswordFile() {
        return passwordFile;
    }

    /**
     * @param passwordFile the Password file to set
     */
    @JsonProperty("passwordFile")
    public void setPasswordFile(String passwordFile) {
        this.passwordFile = passwordFile;
    }

    /**
     * @return the SSL Key file
     */
    @JsonProperty("sslKeyFile")
    public String getSslKeyFile() {
        return sslKeyFile;
    }

    /**
     * @param sslKeyFile the SSL Key file to set
     */
    @JsonProperty("sslKeyFile")
    public void setSslKeyFile(String sslKeyFile) {
        this.sslKeyFile = sslKeyFile;
    }
}
