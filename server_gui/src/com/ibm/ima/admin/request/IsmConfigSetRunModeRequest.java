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

package com.ibm.ima.admin.request;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;

import com.ibm.ima.resources.data.RunMode;

/**
 * IsmConfigSetRunModeRequest may be used to request a runmode
 * change for the server. 
 *
 */
public class IsmConfigSetRunModeRequest extends IsmConfigRequest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @JsonSerialize
    private String Mode;
    
    /**
     * Create a new instance of IsmConfigSetRunModeRequest with a 
     * user id and desired mode
     * 
     * @param userId  The user id requesting the runmode change
     * @param mode  The desired mode
     */
    public IsmConfigSetRunModeRequest(String userId, RunMode mode) {
        this.Action = "setmode";
        this.Component = "Server";
        this.User = userId;
        this.Mode = mode.getMode();
        setUpdateAllowedInStandby(true);
    }

    @Override
    public String toString() {
        return "IsmConfigSetRunModeRequest [Mode=" + Mode  + "]";
    }

}
