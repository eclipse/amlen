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

import java.text.MessageFormat;
import java.util.Locale;

import javax.jms.JMSRuntimeException;

import com.ibm.ima.jms.ImaJmsException;

/**
 *
 */
public class ImaJmsRuntimeException extends JMSRuntimeException implements ImaJmsException{

    private static final long serialVersionUID = -3003473974517039239L;
    
    private Object [] repl  = null;
    //private Object [] repllist = null;
    private String msgfmt = null;

    /*
     * Create a JMSException with a reason string with no replacement data.
     * @param errorcode  The error code for the exception
     * @param reason     The reason as a Java message format
     */
    public ImaJmsRuntimeException(String errorcode, String reason ) {
        super(reason, errorcode);
        msgfmt = reason;
    }
    

    /*
     * Create a JMSException with a reason message.
     * @param errorcode  The error code for the exception
     * @param reasonmsg  The reason as a Java message format
     * @param repl       An array of replacement data
     */
    public ImaJmsRuntimeException(String errorcode, String reasonmsg, Object ... repl ) {
        super(formatException(reasonmsg, repl), errorcode);
        //this.repllist = repl;
        this.repl = repl;
        msgfmt = reasonmsg;
    }
    
    /*
     * Create a JMSException with a cause and reason message.
     * @param errorcode  The error code for the exception
     * @param cause      An exception to link to this exception
     * @param reasonmsg  The reason as a Java message format
     * @param repl       An array of replacement data
     */
    public ImaJmsRuntimeException(String errorcode, Throwable cause, String reasonmsg, Object ... repl) {
        super(formatException(reasonmsg, repl), errorcode);
        //this.repllist = repl;
        this.repl = repl;
        msgfmt = reasonmsg;
        if (cause != null) {
            initCause(cause);
        }    
    }
    
    /*
     * Format the message into a reason string.
     * @param reasonmsg  The reason as a Java message format
     * @param repl       A single replacement data
     */
    /*
    public static String formatException(String msg, Object repl) {
        try {
            return MessageFormat.format(msg, new Object [] {repl});
        } catch (Exception e) {
            return msg;
        }
    }
    */
    /*
     * Format the message into a reason string.
     * @param reasonmsg  The reason as a Java message format
     * @param repl       An array of replacement data
     */
    public static String formatException(String msg, Object [] repl) {
        try {
            return MessageFormat.format(msg, repl);
        } catch (Exception e) {
            return msg;
        }    
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
     * Common implementation method
     */
    public static int errorType(String ec) {
        int  code = 0;
        if (ec != null) {
            for (int pos=4; pos<ec.length(); pos++) {
                char ch = ec.charAt(pos);
                if (ch >= '0' && ch <= '9') 
                    code = code * 10 + (ch-'0');
                else
                    break;
            }
        }    
        return code; 
    }
    
    /*
     * Common implementation method
     */
    public static String getMsg(ImaJmsException lje, Locale locale) {
        return lje.getMessage();
    }
    
    /*
     * Common implementation method
     */
    public static String getMsgFmt(ImaJmsException lje, Locale locale) {
        return lje.getMessageFormat();
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
     * Show the string form of the exception.
     * @see java.lang.Throwable#toString()
     */
    public String toString() {
        return "JMSRuntimeException: " + getErrorCode() + " " + getMessage();
    }
}
