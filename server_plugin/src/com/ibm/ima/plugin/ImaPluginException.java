/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.plugin;

/**
 * The ImaPluginExecption is used to indicate an error when processing a synchronous request.
 * When processing asynchronous requests, the error will show up as a return code and message.
 */
public class ImaPluginException extends RuntimeException {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    private static final long serialVersionUID = 8017777389014841710L;
    private String errorCode; 
    
    /**
     * Create a plug-in runtime exception with a cause.
     * @param errcode  The error code
     * @param message  The human readable message
     * @param cause    The cause of the exception
     */
    public ImaPluginException(String errcode, String message, Throwable cause) {
        super(message, cause);
        errorCode = errcode;
    }
    
    
    /**
     * Create a plug-in runtime exception without a cause.
     * @param errcode  The error code
     * @param message  The human readable message
     */
    public ImaPluginException(String errcode, String message) {
        super(message);
        errorCode = errcode;
    }
    
    /**
     * Get the error code for the exception. 
     * The error code is a String uniquely identifying the exception message
     * and allows the message to be translated.
     * @return The error code
     */
    public String getErrorCode() {
        return errorCode;
    }
   
    /**
     * Get a string version of the exception including the error code.
     * @return The string representing the exception
     */
    public String toString() {
        if (errorCode != null) {
            return errorCode + ' ' + super.toString();
        } else {
            return super.toString();
        }    
    }
}
