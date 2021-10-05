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

import javax.jms.JMSException;

import com.ibm.ima.jms.ImaJmsException;

/*
 * Stub to return runtime exception when required
 */
public class ImaRuntimeException extends RuntimeException implements ImaJmsException {
    /**
	 * 
	 */
	private static final long serialVersionUID = 474824349730057979L;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

	
	private String errCode = null;
	private Object [] repl  = null;
	private String msgfmt = null;
	
    /*
     * Create a RuntimeException with a reason message.
     * @param errCode  The error code for the exception
     * @param reason  The reason as a Java message format
     * @param repl       An array of replacement data
     */
	public ImaRuntimeException(String errCode, String reason, Object ...repl) {
		super(ImaJmsExceptionImpl.formatException(reason, repl, errCode));
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errCode, reason);
    	this.errCode = errCode;
    	this.repl = repl;
	}
	
    /*
     * Create a RuntimeException with a reason message.
     * @param errCode  The error code for the exception
     * @param reason  The reason as a Java message format
     * @param repl       An array of replacement data
     */
	public ImaRuntimeException(String errCode, Throwable cause, String reason) {
		super(ImaJmsExceptionImpl.formatException(reason, null, errCode), cause);
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errCode, reason);
    	this.errCode = errCode;
	}
	
    /*
     * Create a RuntimeException with a reason message.
     * @param errCode  The error code for the exception
     * @param reason  The reason as a Java message format
     * @param repl       An array of replacement data
     */
	public ImaRuntimeException(String errCode, Throwable cause, String reason, Object ...repl) {
		super(ImaJmsExceptionImpl.formatException(reason, null, errCode), cause);
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errCode, reason);
    	this.errCode = errCode;
    	this.repl = repl;
	}

	public static Throwable mapException(Throwable t) {
        if (t instanceof RuntimeException)
            return t;

        if (t instanceof ImaJmsException) {
          return new ImaRuntimeException(((ImaJmsException)t).getErrorCode(), t, ((ImaJmsException)t).getMessageFormat(), ((ImaJmsException)t).getParameters());
        }
        else if (t instanceof JMSException) {
            return new ImaRuntimeException(((JMSException)t).getErrorCode(), t, t.getMessage());
        }
        
        return new ImaRuntimeException("CWLNC0999", t, "Failure caused by exception {0}: {1}", t.getClass().getName(), t.getMessage());
    }

	@Override
	public String getErrorCode() {
		return errCode;
	}

	@Override
	public String getMessageFormat() {
		return msgfmt;
	}

	@Override
	public String getMessageFormat(Locale locale) {
		return ImaJmsExceptionImpl.getMsgFmt(this, locale);
	}

	@Override
	public Object[] getParameters() {
		return repl;
	}

	@Override
	public int getErrorType() {
		return ImaJmsExceptionImpl.errorType(getErrorCode());
	}

	@Override
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
        return "RuntimeException: " + getErrorCode() + " " + getMessage();
    }
}
