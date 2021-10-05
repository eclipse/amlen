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

import javax.transaction.xa.XAException;

import com.ibm.ima.jms.ImaJmsException;

public class ImaXAException extends XAException implements ImaJmsException {
    String errCode = null;
    String msgfmt = null;
    Object[] repl = null;
    
    /**
     * 
     */
    private static final long serialVersionUID = -4469198526818537292L;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /*
     * Create an XAException with an error code, XA error code, and reason.
     * @param imaErrcode  The error code for the exception
     * @param xaErrCode   The XA error code for the exception
     * @param reason      The reason as a Java message format
     */
    public ImaXAException(String imaErrCode, int xaErrCode, String reason) {
    	super(ImaJmsExceptionImpl.formatException(reason, null, imaErrCode));
    	this.errorCode = xaErrCode;
        this.errCode = imaErrCode;
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, imaErrCode, reason);
    }
    
    /*
     * Create an XAException with an error code, XA error code, reason, and replacement data.
     * @param imaErrcode  The error code for the exception
     * @param xaErrCode   The XA error code for the exception
     * @param reason      The reason as a Java message format
     * @param repl        An array of replacement data
     */
    public ImaXAException(String imaErrCode, int xaErrCode, String reason, Object ...repl) {
        super(ImaJmsExceptionImpl.formatException(reason, repl, imaErrCode));
        this.errorCode = xaErrCode;
        this.errCode = imaErrCode;
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, imaErrCode, reason);
        this.repl = repl;
    }
    
    /*
     * Create an XAException with an error code, XA error code, cause, reason, and replacement data.
     * @param imaErrcode  The error code for the exception
     * @param xaErrCode   The XA error code for the exception
     * @param ex          The cause of the exception
     * @param reason      The reason as a Java message format
     * @param repl        An array of replacement data
     */
    public ImaXAException(String imaErrCode, int xaErrCode, Throwable ex, String reason, Object ...repl) {
        super(ImaJmsExceptionImpl.formatException(reason, repl, imaErrCode));
        this.errorCode = xaErrCode;
        this.errCode = imaErrCode;
        if (ex != null)
            initCause(ex);
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, imaErrCode, reason);
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
    
    public static ImaXAException notAllowed(String method) {
        ImaXAException xe = new ImaXAException("CWLNC2401", XAException.XAER_PROTO, "A call to method {0} failed because it cannot be called on an XAResource for recovery. This might indicate a problem with the application server.", method);
        return xe;
    }
    
    /*
     * Show the string form of the exception.
     * @see java.lang.Throwable#toString()
     */
    public String toString() {
        String msg = super.getMessage();
        if (msg == null) {
            msg = ImaJmsExceptionImpl.formatException(msgfmt, repl, getErrorCode());
        }
        return "XAException: " + getErrorCode() + " " + msg;
    }

}
