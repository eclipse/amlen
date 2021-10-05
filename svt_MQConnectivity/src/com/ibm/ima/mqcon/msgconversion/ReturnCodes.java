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
package com.ibm.ima.mqcon.msgconversion;

public interface ReturnCodes {
	
	public static int PASSED = 0;
	public static int INCORRECT_PARAMS = 1;
	public static int JMS_EXCEPTION = 2;
	public static int INCORRECT_MESSAGE = 3;
	public static int MESSAGE_NOT_ARRIVED = 4;
	public static int UNABLE_TO_CREATE_MESSAGE = 5;
	public static int UNABLE_TO_READ_MESSAGE = 6;
	public static int MESSAGE_NOT_ARRIVED_FOR_DURABLE = 7;
	public static int MESSAGE_RECEIVED_INCORRECTLY = 8;
	public static int CONNECTION_LOST = 9;
	public static int MQTT_EXCEPTION = 11;
	public static int IOE_EXCEPTION = 12;
	public static int INCORRECT_QOS = 13;
	public static int INCOMPLETE_TEST = 14;
	public static int INCORRECT_RETAINED_PROP = 15;
	public static int GENERAL_EXCEPTION = 16;
	public static int CONSUMER_DIDNT_DISCONNECT_DESPITE_REQUEST = 17;



}
