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

package com.ibm.ima.jms.impl;

import java.util.Locale;

import javax.jms.TransactionRolledBackException;

import com.ibm.ima.jms.ImaJmsException;

public class ImaTransactionRolledBackException extends TransactionRolledBackException implements ImaJmsException {
	private static final long serialVersionUID = -3822741636852055671L;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private static String msgfmt = "A client request to commit a transaction failed with return code = {0}.  The transaction was rolled back.";
    private static final String errorcode = "CWLNC0222";
    private final int returnCode;
    
    /*
     * Create an ImaTransactionRolledBackException with a error code.
     * @param rc  The error code returned by the server
     */
    public ImaTransactionRolledBackException(int rc ) {
        super(ImaJmsExceptionImpl.formatException(msgfmt, new Object [] {new Integer(rc)}, errorcode), errorcode);
        String defaultmsgfmt = msgfmt;
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errorcode, defaultmsgfmt);
        returnCode = rc;
    }
    
    /*
     * Return the error code.
     * @see com.ibm.ima.jms.ImaJmsException#getErrorType()
     */
    public int getErrorType() {
        return ImaJmsExceptionImpl.errorType(errorcode);
    }    
    

    /*
     * Return translated message.
     * @see com.ibm.ima.jms.ImaJmsException#getMessage(java.util.Locale)
     */
    public String getMessage(Locale locale) {
        return ImaJmsExceptionImpl.getMsg(this, locale);
    }
    
    /*
     * Return the translated message for the default locale.
     * @see com.ibm.ima.jms.ImaJmsException#getMessage(java.util.Locale)
     */
    public String getLocalizedMessage() {
        return getMessage(Locale.getDefault());
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
        return ImaJmsExceptionImpl.getMsgFmt(this, locale);
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
        return "TransactionRolledBackException: " + getErrorCode() + " " + getMessage();
    }
}

