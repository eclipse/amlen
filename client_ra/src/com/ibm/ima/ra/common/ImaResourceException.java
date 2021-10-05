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

package com.ibm.ima.ra.common;

import java.util.Locale;

import javax.resource.ResourceException;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.impl.ImaJmsExceptionImpl;

public class ImaResourceException extends ResourceException implements ImaJmsException {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private static final long serialVersionUID = 6658908447850176663L;
    Object [] repl  = null;
    String msgfmt;

    /*
     * Create a ResourceException with an error code and message.
     * @param errorcode  The error code for the exception
     * @param message    The reason as a Java message format
     */
    public ImaResourceException(String errCode, String message) {
        this(errCode, null, message);
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errCode, message);
    }
    
    /*
     * Create a ResourceException with an error code, message, and replacement data.
     * @param errorcode  The error code for the exception
     * @param message    The reason as a Java message format
     * @param repl       An array of replacement data
     */
    public ImaResourceException(String errCode, String message, Object ...repl) {
        this(errCode, null, message, repl);
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errCode, message);
    }
    
    /*
     * Create a ResourceException with an error code, cause, and message but no replacement data.
     * @param errorcode  The error code for the exception
     * @param cause      The cause of the exception
     * @param message    The reason as a Java message format
     */
    public ImaResourceException(String errCode, Throwable cause, String message) {
        this(errCode, cause, message, (Object[])null);
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errCode, message);
    }
    
    /*
     * Create a ResourceException with an error code, cause, message, and replacement data.
     * @param errorcode  The error code for the exception
     * @param cause      The cause of the exception
     * @param message    The reason as a Java message format
     * @param repl       An array of replacement data
     */
    public ImaResourceException(String errCode, Throwable cause, String message, Object ... repl) {
        super(ImaJmsExceptionImpl.formatException(message, repl, errCode), errCode);
        if (cause != null)
            initCause(cause);
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errCode, message);
        this.repl = repl;
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
        return repl;
    }
    
    /*
     * Show the string form of the exception.
     * @see java.lang.Throwable#toString()
     */
    public String toString() {
        return "ResourceException: " + getErrorCode() + " " + getMessage();
    }
    
    public static ImaResourceException onMessageNotFound(Exception e) {
        if (e != null)
            return new ImaResourceException("CWLNC2144", "Endpoint activation failed because the onMessage method was not found in the message driven bean.");
        return new ImaResourceException("CWLNC2144", e, "Endpoint activation failed because the onMessage method was not found in the message driven bean.");
    }
    
    public static ImaResourceException managedConnCredentialMismatch(String credType, Object managedObj) {
        return new ImaResourceException("CWLNC2161","Failed to create a connection handle for a managed connection because the credentials provided are not correct. The {0} content does not match the expected content. Managed connection: {1}", credType, managedObj);
    }

}
