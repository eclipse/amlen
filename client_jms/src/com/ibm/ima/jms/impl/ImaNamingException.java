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

import javax.naming.NamingException;

import com.ibm.ima.jms.ImaJmsException;

public class ImaNamingException extends NamingException implements
        ImaJmsException {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


	private String errCode = null;
	private String msgfmt = null;
	private Object[] repl;
    
    /**
	 * 
	 */
	private static final long serialVersionUID = -6313179159380074301L;

    /*
     * Create a NamingException with an error code and explanation.
     * @param errCode      The error code for the exception
     * @param explanation  The reason as a Java message format
     */
    public ImaNamingException(String errCode, String explanation) {
        super(ImaJmsExceptionImpl.formatException(explanation, null, errCode));
        this.errCode = errCode;
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errCode, explanation);
    }
    
    /*
     * Create a NamingException with an error code, explanation, and replacement data.
     * @param errCode      The error code for the exception
     * @param explanation  The reason as a Java message format
     * @param repl         An array of replacement data
     */
    public ImaNamingException(String errCode, String explanation, Object ...repl) {
        super(ImaJmsExceptionImpl.formatException(explanation, repl, errCode));
        this.errCode = errCode;
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errCode, explanation);
        this.repl = repl;
    }
    
    /*
     * Return the error code.
     * @see com.ibm.ima.jms.ImaJmsException#getErrorCode()
     */
    public String getErrorCode() {
        return errCode;
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
    public Object[] getParameters() {
        return repl;
    }

    /*
     * Return the error code.
     * @see com.ibm.ima.jms.ImaJmsException#getErrorType()
     */
    public int getErrorType() {
        return ImaJmsExceptionImpl.errorType(getErrorCode());
    }

    /*
     * Return the message for a specified locale
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
     * Show the string form of the exception.
     * @see java.lang.Throwable#toString()
     */
    public String toString() {
        return "NamingException: " + getErrorCode() + " " + getMessage();
    }
}
