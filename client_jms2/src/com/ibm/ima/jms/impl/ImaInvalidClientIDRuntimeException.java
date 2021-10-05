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

import javax.jms.InvalidClientIDRuntimeException;

import com.ibm.ima.jms.ImaJmsException;

public class ImaInvalidClientIDRuntimeException extends InvalidClientIDRuntimeException implements ImaJmsException {
    private static final long serialVersionUID = 7942808003733211852L;
    private Object[] repl  = null;
    private String msgfmt = null;
    
    /*
     * Create an InvalidClientIDException with a reason message.
     * @param errorcode  The error code for the exception
     * @param reasonmsg  The reason as a Java message format
     * @param repl       An array of replacement data
     */
    public ImaInvalidClientIDRuntimeException(String errorcode, String reasonmsg, Object ... repl ) {
        super(ImaJmsRuntimeException.formatException(reasonmsg, repl), errorcode);
        this.repl = repl;
        msgfmt = reasonmsg;
    }
    
    /*
     * Create a InvalidClientIDException with a reason message.
     * @param errorcode  The error code for the exception
     * @param cause      An exception to link to this exception
     * @param reasonmsg  The reason as a Java message format
     * @param repl       An array of replacement data
     */
    public ImaInvalidClientIDRuntimeException(String errorcode, Throwable cause, String reasonmsg, Object ... repl ) {
        super(ImaJmsRuntimeException.formatException(reasonmsg, repl), errorcode);
        this.repl = repl;
        msgfmt = reasonmsg;
        if (cause != null) {
            initCause(cause);
        }
    }       

    /*
     * Return the error code.
     * @see com.ibm.ima.jms.ImaJmsException#getErrorType()
     */
    public int getErrorType() {
        return ImaJmsRuntimeException.errorType(getErrorCode());
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
    	/*
        if (repl == null)
            return null;
        return new Object [] {repl};
        */
    	return repl;
    }
    
    /*
     * Show the string form of the exception.
     * @see java.lang.Throwable#toString()
     */
    public String toString() {
        return "InvalidClientIDRuntimeException: " + getErrorCode() + " " + getMessage();
    }
}
