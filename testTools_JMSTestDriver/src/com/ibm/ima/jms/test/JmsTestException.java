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
package com.ibm.ima.jms.test;

public class JmsTestException extends Exception {
    private static final long serialVersionUID = -8621512746234898947L;
    private final String      _errorCode;

    public JmsTestException(String errorCode, String message) {
        this(errorCode, message, null);
    }

    public JmsTestException(String errorCode, String message, Throwable cause) {
        super(message, cause);
        _errorCode = errorCode;
    }

    public String toString() {
        if (getCause() != null)
            return "JmsTestException: errorCode = " + _errorCode
                    + ". Description: " + getMessage() + ". Cause: "
                    + getCause();
        else
            return "JmsTestException: errorCode = " + _errorCode
                    + ". Description: " + getMessage();
    }

    String getErrorCode() {
        return _errorCode;
    }

}
