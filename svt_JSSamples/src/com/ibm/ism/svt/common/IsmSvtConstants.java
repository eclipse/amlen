// Copyright (c) 2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
package com.ibm.ism.svt.common;

public interface IsmSvtConstants {

	// GUI delay between click and input ready
	public static final int STATIC_WAIT_TIME = 500; 	// milliseconds to wait entry fields
	public static final int SVT_DELAY = 0;				// milliseconds to wait buttons
	
	// Publisher Messages
	public static final String SVT_LOSTCONNECTION = "Publisher connection lost for client %1.";

	// Subscriber Messages
	public static final String SVT_CONNECTION = "Subscribed to topic: %1.";
	public static final String SVT_SUBSCRIPTION = "Message %1 received on topic '%2': %3";
	public static final String SVT_DISCONNECT = "Subscriber disconnected.";
	

}
