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

import com.ibm.ima.jms.ImaJmsException;

public class ImaUnsupportedOperationException extends
		UnsupportedOperationException implements ImaJmsException {
    String errCode = null;
    String msgfmt = null;
    Object[] repl = null;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

	/**
	 * 
	 */
	private static final long serialVersionUID = 8776919075682435275L;
	
	public ImaUnsupportedOperationException(String errCode, String message) {
		super(ImaJmsExceptionImpl.formatException(message, null, errCode));
		msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errCode, message);
		this.errCode = errCode;
	}
	
	public ImaUnsupportedOperationException(String errCode, String message, Object ... repl) {
		super(ImaJmsExceptionImpl.formatException(message, repl, errCode));
		msgfmt = ImaJmsExceptionImpl.getMessageFromBundle(Locale.ROOT, errCode, message);
		this.repl = repl;
		this.errCode = errCode;
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
        return "UnsupportedOperationException: " + getErrorCode() + " " + getMessage();
    }
}
