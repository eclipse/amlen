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
package com.ibm.ima.admin.response;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

/**
 * Returned from an Ism Config/Monitoring Request in the case of an error.
 */
@JsonIgnoreProperties(ignoreUnknown = true)
public class IsmResponse {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    public String RC;
    public String ErrorString;

    public String getRC() {
        return RC;
    }
    public void setRC(String rC) {
        RC = rC;
    }
    public String getErrorString() {
        return ErrorString;
    }
    public void setErrorString(String errorString) {
        ErrorString = errorString;
    }

    @Override
    public String toString() {
        return "[RC=" + RC + "] " + ErrorString;
    }
}
