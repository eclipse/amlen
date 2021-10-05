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
package com.ibm.ima.mqcon.rules;

import com.ibm.ima.mqcon.utils.StaticUtilities;

public class RunAllTests {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		boolean result = true;
		String[] args2 = {"-numberOfMessages", "1", 
				"-waitTime", "10000"};
		
		args = args2;
		System.out.println("ImaQueueToMqTopicTest");
		result = ImaQueueToMqTopicTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("ImaQueueToMqQueueTest");
		result = ImaQueueToMqQueueTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("MqQueueToImaQueue");
		result = MqQueueToImaQueue.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("MqTopicSubtreeToImaQueueTest");
		result = MqTopicSubtreeToImaQueueTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("MqTopicToImaQueueTest");
		result = MqTopicToImaQueueTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("ImaQueueToMqTopicDurableTest");
		result =  ImaQueueToMqTopicDurableTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("ImaTopicMqttToMqTopicMqJavaTest");
		result = ImaTopicMqttToMqTopicMqJavaTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("ImaTopicSubtreeToMqQueueTest");
		result = ImaTopicSubtreeToMqQueueTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("ImaTopicSubtreeToMqTopicSubtreeTest");
		result = ImaTopicSubtreeToMqTopicSubtreeTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("ImaTopicSubtreeToMqTopicTest");
		result = ImaTopicSubtreeToMqTopicTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("ImaTopicToMqQueueTest");
		result = ImaTopicToMqQueueTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("ImaTopicToMultipleMqTopicsTest");
		result = ImaTopicToMultipleMqTopicsTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("MqQueueMqJavaToImaTopicMqttTest");
		result = MqQueueMqJavaToImaTopicMqtt.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}

		System.out.println("MqTopicMqJavaToImaTopicMqttTest");
		result = MqTopicMqJavaToImaTopicMqttTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}

		System.out.println("MqTopicSubtreeToImaSubtreeTest");
		result = MqTopicSubtreeToImaSubtreeTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("MqTopicSubtreeToImaTopicTest");
		result = MqTopicSubtreeToImaTopicTest.run(args) && result;
		if (!result) {
			StaticUtilities.printFail();
		}
		System.out.println("result: " + result);
	}

}
