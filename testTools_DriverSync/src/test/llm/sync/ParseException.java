/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package test.llm.sync;

/**
 * An exception class providing a means to pass in a string
 * that is accessible in catching.
 *
 */
public class ParseException extends Exception {
	
	private String				input				= null;
	
	private static final long	serialVersionUID	= 1L;
	
	public ParseException() {
		super();
	}
	
	/**
	 * 
	 * @param s Exception Message
	 * @param toParse A string with extra info about the cause
	 */
	public ParseException(String s, String toParse) {
		super(s);
		input = new String(toParse);
	}
	
	/**
	 * Retrieve the extra info
	 * @return string containing additional info
	 */
	public String getInput() {
		return input;
	}
}
