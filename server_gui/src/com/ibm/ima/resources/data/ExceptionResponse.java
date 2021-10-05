/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.resources.data;

import java.util.Locale;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize.Inclusion;

import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.SecurityContext;
import com.ibm.ima.util.IsmLogger;

public class ExceptionResponse {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

	
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
	
	@JsonSerialize(include = Inclusion.NON_NULL)
	public String code;
	@JsonSerialize(include = Inclusion.NON_NULL)
	public String message;
	@JsonSerialize(include = Inclusion.NON_NULL)
	public String cause;
	
	public ExceptionResponse(final String code, final String message) {
		this.code = code;
		this.message = message;
		this.cause = null;
	}
	
	public ExceptionResponse(Throwable exception, Locale locale) {
		
		// if locale null see if we can look it up
		if (locale == null) {
	        SecurityContext context = SecurityContext.getContext();
	        if (context != null) {
	        	locale = SecurityContext.getContext().getLocale();
	        } else {
	        	// fall back to English if SecurityContext instance is null
				locale = Locale.ENGLISH;
	        }
		}
		
		if (exception instanceof IsmRuntimeException) {
			IsmRuntimeException ire = (IsmRuntimeException) exception;
			code = ire.getMessageID();
			if (ire.isNativeMessage()) {
				message = ire.getMessage();
			} else {
				message = logger.getFormattedMessage(ire.getMessageID(), ire.getParams(), locale, false);
			}
			if (ire.getCause() != null) {
				cause = ire.getCause().getLocalizedMessage();
			}
		} else {
			message = exception.getMessage();
			if (exception.getCause() != null) {
				cause = exception.getCause().getLocalizedMessage();
			}
		}
		
	}
	
}
