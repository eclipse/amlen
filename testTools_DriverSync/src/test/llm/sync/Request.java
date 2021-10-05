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
/**
 * 
 */
package test.llm.sync;

/**
 * Roughly an ENUM; for compat with java1.4 compliant driver code.
 * 
 */
public class Request {
	public static final int	INIT	= 1;
	public static final int	SET		= 2;
	public static final int	WAIT	= 3;
	public static final int	RESET	= 4;
	public static final int	DELETE	= 5;
	public static final int GET     = 6;
	
	private int				type;
	
	/**
	 * Creates a request with corresponding type
	 * 
	 * @param val
	 *            Integer specifying request type
	 * @throws ParseException
	 *             invalid value chosen
	 */
	public Request(int val) throws ParseException {
		setType(val);
	}
	
	/**
	 * Creates a request with corresponding type
	 * 
	 * @param val
	 *            String specifying request type
	 * @throws ParseException
	 *             invalid value chosen
	 */
	public Request(String val) throws ParseException {
		int ival = -1;
		if (val.equals("init")){
			ival = Request.INIT;
		}
		if (val.equals("set")){
			ival= Request.SET;
		}
		if (val.equals("wait")){
			ival= Request.WAIT;
		}
		if (val.equals("delete")){
			ival = Request.DELETE;
		}
		if (val.equals("reset")){
			ival = Request.RESET;
		}		
		if (val.equals("get")){
			ival = Request.GET;
		}		
		setType(ival);
	}
	
	/**
	 * Return the request type
	 * 
	 * @return Integer representation of Request
	 */
	public int getType() {
		return type;
	}
	
	/**
	 * Set the type of the request to given value
	 * 
	 * @param val
	 *            Type to be set
	 * @throws ParseException
	 *             on invalid value
	 */
	private void setType(int val) throws ParseException {
		if (val == INIT || val == SET || val == WAIT || val == DELETE || val == RESET || val == GET) {
			type = val;
		} else {
			throw new ParseException("Invalid Value for Request: ", Integer.toString(val));
		}
	}
	
	/**
	 * Value comparison
	 * 
	 * @param t
	 *            value to compare request's type to
	 * @return True if type == t
	 */
	public boolean eq(int t) {
		return type == t;
	}
}
