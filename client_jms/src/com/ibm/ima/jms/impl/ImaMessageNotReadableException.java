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

import javax.jms.MessageNotReadableException;

import com.ibm.ima.jms.ImaJmsException;

public class ImaMessageNotReadableException extends MessageNotReadableException implements ImaJmsException {
    private static final long serialVersionUID = 1925235115195650146L;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private Object [] repl  = null;
    //private Object [] repllist = null;
    private String msgfmt = null;
    
    /*
     * Create a MessageNotReadableException with a reason string.
     * @param errorcode  The error code for the exception
     * @param reasonmsg  The reason as string
     */
    public ImaMessageNotReadableException(String errorcode, String reason) {
        super(ImaJmsExceptionImpl.formatException(reason, null, errorcode), errorcode);
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errorcode, reason);
    }
    /*
     * Create a MessageNotReadableException with a reason message.
     * @param errorcode  The error code for the exception
     * @param reasonmsg  The reason as a Java message format
     * @param repl       An array of replacement data
     */
    public ImaMessageNotReadableException(String errorcode, String reasonmsg, Object ... repl ) {
        super(ImaJmsExceptionImpl.formatException(reasonmsg, repl, errorcode), errorcode);
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errorcode, reasonmsg);
        this.repl = repl;
    }
    
    /*
     * Create a MessageNotReadableException with a reason message.
     * @param errorcode  The error code for the exception
     * @param cause      An exception to link to this exception
     * @param reasonmsg  The reason as a Java message format
     * @param repl       An array of replacement data
     */
    public ImaMessageNotReadableException(String errorcode, Throwable cause, String reasonmsg, Object ... repl ) {
        super(ImaJmsExceptionImpl.formatException(reasonmsg, repl, errorcode), errorcode);
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errorcode, reasonmsg);
        this.repl = repl;
        if (cause != null) {
            if (cause instanceof Exception)
                setLinkedException((Exception)cause);
            initCause(cause);
        }
    }       

    /*
     * Return the error code.
     * @see com.ibm.ima.jms.ImaJmsException#getErrorType()
     */
    public int getErrorType() {
        return ImaJmsExceptionImpl.errorType(getErrorCode());
    }    
    

    /*
     * Return the translated message.
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
    	/*
        if (repllist == null && repl != null) {
            repllist = new Object [] {repl};
        }
        return repllist;
        */
    	return repl;
    }
    
    /*
     * Show the string form of the exception.
     * @see java.lang.Throwable#toString()
     */
    public String toString() {
        return "MessageNotReadableException: " + getErrorCode() + " " + getMessage();
    }
}
