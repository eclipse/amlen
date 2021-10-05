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

package com.ibm.ima.jms.impl;

import java.util.Locale;

import javax.jms.TransactionRolledBackRuntimeException;

import com.ibm.ima.jms.ImaJmsException;

public class ImaTransactionRolledBackRuntimeException extends TransactionRolledBackRuntimeException implements ImaJmsException {
	private static final long serialVersionUID = -3822741636852055671L;
    private static final String msgfmt = "Transaction was rolled back. Reason: {0}.";
    private static final String errorcode = "CWLNC0222";
    private final int returnCode;
    
    /*
     * Create an ImaTransactionRolledBackException with a error code.
     * @param rc  The error code returned by the server
     */
    public ImaTransactionRolledBackRuntimeException(int rc ) {
        super(ImaJmsRuntimeException.formatException(msgfmt, new Object [] {new Integer(rc)}), errorcode);
        returnCode = rc;
    }
    
    /*
     * Return the error code.
     * @see com.ibm.ima.jms.ImaJmsException#getErrorType()
     */
    public int getErrorType() {
        return ImaJmsRuntimeException.errorType(errorcode);
    }    
    

    /*
     * Re
     * @see com.ibm.ima.jms.ImaJmsException#getMessage(java.util.Locale)
     */
    public String getMessage(Locale locale) {
        return ImaJmsRuntimeException.getMsg(this, locale);
    }
    

    /*
     * Return the untranslated message format.
     * @see com.ibm.ima.jms.ImaJmsException#getMessageFormat()
     */
    public String getMessageFormat() {
        return msgfmt;
    }

    /*
     * Return the message format for the specified locale.
     * @see com.ibm.ima.jms.ImaJmsException#getMessageFormat(java.util.Locale)
     */
    public String getMessageFormat(Locale locale) {
        return ImaJmsRuntimeException.getMsgFmt(this, locale);
    }
    
    /*
     * Get the replacement parameters.
     * @return An array of objects which are used as the replacement data for the exception.
     * This can be null if there is no replacement data. 
     */
    public Object [] getParameters() {
    	Integer []result = new Integer[1];
    	result[0] = new Integer(returnCode);
    	return result;
    }
    
    /*
     * Show the string form of the exception.
     * @see java.lang.Throwable#toString()
     */
    public String toString() {
        return "TransactionRolledBackRuntimeException: " + getErrorCode() + " " + getMessage();
    }
}

