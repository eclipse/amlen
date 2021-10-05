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

public class ApplianceResponse{

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private int rc = ApplianceResource.APPLIANCE_REQUEST_FAILED;
    private String response = null;
    private String error = null;

    /**
     * @param rc
     * @param response
     */
    public ApplianceResponse(int rc, String response) {
        this.rc = rc;
        this.response = response;
    }


    /**
     * @param rc
     * @param response
     */
    public void setApplianceResponse(int rc, String response) {
        this.rc = rc;
        this.response = response;
    }


    /**
     * @return the rc
     */
    @JsonProperty("RC")
    public int getRc() {
        return rc;
    }

    /**
     * @param rc the rc to set
     */
    public void setRc(int rc) {
        this.rc = rc;
    }

    /**
     * @return the response
     */
    @JsonProperty("Response")
    public String getResponse() {
        return response;
    }

    /**
     * @param response the response to set
     */
    public void setResponse(String response) {
        this.response = response;
    }

    public String getError() {
        return error;
    }

    public void setError(String error) {
        this.error = error;
    }

    @Override
    public String toString() {
        return "ApplianceResponse [rc=" + rc + ", response=" + response + ", error=" + error + "]";
    }


}
