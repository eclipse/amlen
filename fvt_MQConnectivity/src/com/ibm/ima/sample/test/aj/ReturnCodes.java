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
package com.ibm.ima.sample.test.aj;

public class ReturnCodes 
{
	public static int PASSED = 0;
	public static int DEFAULT_RC = 1;
		
	public static final int ERROR_NO_COMMANDLINE_ARGS= 101;
	public static final int ERROR_INVALID_NUMBER_OF_COMMANDLINE_ARGS= 102;
	public static final int ERROR_INVALID_COMMANDLINE_ARGS= 103;
	
	
	public static final int ERROR_CREATING_PRODUCER = 1011;
	public static final int ERROR_CREATING_CONSUMER = 1012;
	
	public static final int ERROR_SENDING_A_MESSAGE = 1004;
	public static final int ERROR_RECEIVING_A_MESSAGE = 1005;
	public static final int ERROR_NULL_MESSAGE_RECEIVED = 1006;
	public static final int ERROR_INVALID_MSG_RECEIVED = 1007;
	public static final int ERROR_EXTRA_MGS_RECEIVED = 1008;
	public static final int ERROR_WHILE_CHECKING_FOR_EXTRA_MSGS = 1009;
	
	public static final int ERROR_DESTROYING_PUBLISHER_FAILED = 1014;
	public static final int ERROR_DESTROYING_CONSUMER_FAILED = 1015;
	public static final int INTERRUPTED_SLEEP_FAILED = 1019;
	public static final int ERROR_NO_MSGS_FOR_10SECONDS_FAILED = 1121;
	public static final int ERROR_CONNECTION_LOST = 1211;
	
}
