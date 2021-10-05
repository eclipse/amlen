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
package com.ibm.ima.mqcon.enabledediting;

public class TestKickoff {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		EnabledEditConfig config = new EnabledEditConfig("9.20.123.102", 16230, "9.20.86.15", 1423,
				"IMAMQCONNECTIVITY1", "/IMATopic", "TOPIC1", "ENABLEDEDITING");
		config.setup();
		Retained_IMATopicToMQTopic r = new Retained_IMATopicToMQTopic(config);
		r.startTest();
		
/*		config.teardown();
		
		config.setup();*/
		Retained_MQTopicToIMATopic r2 = new Retained_MQTopicToIMATopic(config);
		r2.startTest();
		
		config.teardown();
		
		System.out.println("Test 1 passed - " + r.isSuccess());
		System.out.println("Test 2 passed - " + r2.isSuccess());
		
	}
}
