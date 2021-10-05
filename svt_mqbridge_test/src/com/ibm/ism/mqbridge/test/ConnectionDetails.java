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
package com.ibm.ism.mqbridge.test;

import com.ibm.mq.MQEnvironment;
import com.ibm.mq.MQQueueManager;

public class ConnectionDetails {
	public static String ismServerPort = "16102";
	public static String ismServerQPort = "16104";
	public static String ismServerHost = "9.20.123.100";
	//public static String queueManager = "MQC_RULES";
	  public static String queueManager = "dttest3";
	//public static final String queueManager = "lttest";
	//public static final String mqHostAddress = "127.0.0.1"; // "9.20.230.78";
//	public static String mqHostAddress = "9.3.177.100";
	public static String mqHostAddress = "9.20.230.234";
	private static MQQueueManager mqQueueManager;
	public static String mqChannel = "JAVA.CHANNEL";
//	public static String mqChannel = "SYSTEM.ISM.SVRCONN";	
//	public static int mqPort = 1434;
	public static int mqPort = 1414;
	public static int mqTTPort = 1883;
	public static String ismServerQueuePort = "16104";

	public static void setUpMqClientConnectionDetails() {
		MQEnvironment.hostname = ConnectionDetails.mqHostAddress;
		MQEnvironment.channel  = ConnectionDetails.mqChannel;
		MQEnvironment.port = ConnectionDetails.mqPort;
	//	MQEnvironment.userID = "ismuser";
	}
	
	public static MQQueueManager getMqQueueManager() {		
			try {
				mqQueueManager = new MQQueueManager(queueManager);
			} catch (Exception e) {
				e.printStackTrace();
				// TODO add test failed
			}
		return mqQueueManager;
	}

	public static void setMqQueueManager(MQQueueManager mqQueueManager) {
		ConnectionDetails.mqQueueManager = mqQueueManager;
	}
	
	public static MQQueueManager getMqQueueManager(String qm) {
	//	if (mqQueueManager == null) {
			try {
				mqQueueManager = new MQQueueManager(qm);
			} catch (Exception e) {
				e.printStackTrace();
				// TODO add test failed
				System.exit(1);
			}
	//	}
		return mqQueueManager;
	}
	
}
