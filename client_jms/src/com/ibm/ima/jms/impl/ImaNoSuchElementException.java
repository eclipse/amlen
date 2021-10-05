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
import java.util.NoSuchElementException;

import com.ibm.ima.jms.ImaJmsException;

public class ImaNoSuchElementException extends NoSuchElementException implements
		ImaJmsException {
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

	/**
	 * 
	 */
	private static final long serialVersionUID = 8638771955179156395L;
	
    /*
     * Create a NullPointerException with a reason string.
     * @param errCode  The error code for the exception
     * @param reason The reason as string
     */
	public ImaNoSuchElementException(String errCode, String reason) {
		super(ImaJmsExceptionImpl.formatException(reason, null, errCode));
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errCode, reason);
    	this.errCode = errCode;
	}
	
    /*
     * Create a NullPointerException with a reason message.
     * @param errCode  The error code for the exception
     * @param reason  The reason as a Java message format
     * @param repl       An array of replacement data
     */
	public ImaNoSuchElementException(String errCode, String reason, Object ...repl) {
		super(ImaJmsExceptionImpl.formatException(reason, null, errCode));
    	msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errCode, reason);
    	this.errCode = errCode;
    	this.repl = repl;
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
        return "NoSuchElementException: " + getErrorCode() + " " + getMessage();
    }
}
