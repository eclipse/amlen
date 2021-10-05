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

package com.ibm.ima.resources.data;

import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.ApplianceResource;

public class MemoryResponse {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private int rc = ApplianceResource.APPLIANCE_REQUEST_FAILED;
    private int totalMemory;
    private int freeMemory;

    public MemoryResponse(int rc, int totalMemory, int freeMemory) {
        this.rc = rc;
        this.totalMemory = totalMemory;
        this.freeMemory = freeMemory;
    }

    @JsonProperty("RC")
    public int getRC() {
        return rc;
    }

    public void setRC(int rc) {
        this.rc = rc;   
    }
    
    public int getTotalMemory() {
        return totalMemory;
    }

    public void setTotalMemory(int totalMemory) {
        this.totalMemory = totalMemory;
    }
    
    public int getFreeMemory() {
        return freeMemory;
    }

    public void setFreeMemory(int freeMemory) {
        this.freeMemory = freeMemory;
    }
    
    public void setResponse(int rc, int totalMemory, int freeMemory) {
        this.rc = rc;
        this.totalMemory = totalMemory;
        this.freeMemory = freeMemory;
    }
}
