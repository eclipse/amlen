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
/*-----------------------------------------------------------------
 * System Name : Internet Scale Messaging
 * File        : IsmTestException.java
 * Author      : 
 *-----------------------------------------------------------------
 */

package com.ibm.ism.ws.test;

public class IsmTestException extends Exception {
	private static final long serialVersionUID = -8621512746234898947L;
	private final String _errorCode;

	public IsmTestException(String errorCode, String message) {
		this(errorCode, message, null);
	}

	public IsmTestException(String errorCode, String message, Throwable cause) {
		super(message, cause);
		_errorCode = errorCode;
	}

	public String toString() {
		if (getCause() != null)
			return "ImsTestException: errorCode = " + _errorCode
					+ ". Description: " + getMessage() + ". Cause: "
					+ getCause();
		else
			return "ImsTestException: errorCode = " + _errorCode
					+ ". Description: " + getMessage();
	}

	String getErrorCode() {
		return _errorCode;
	}

}
