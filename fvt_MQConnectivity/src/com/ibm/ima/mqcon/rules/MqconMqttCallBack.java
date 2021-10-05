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

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import com.ibm.ima.mqcon.utils.Trace;

public class MqconMqttCallBack implements MqttCallback {
	private MqJavaConnectivityTest test;
	long timeArrived;

	public MqconMqttCallBack(MqJavaConnectivityTest test) {
		this.test = test;
	}

	public void connectionLost(Throwable throwable) {
		System.out.println("connectionLost " + throwable.getMessage());	
		throwable.printStackTrace();
	}

	public void deliveryComplete(IMqttDeliveryToken token) {		
		System.out.println("delivery complete called");
	}

	public void messageArrived(String topicName, MqttMessage message) throws Exception {

		String messagePayload = message.toString();
		int messageQos = message.getQos();		
		if (Trace.traceOn()) {

			System.out.println("topicName " + topicName);
			System.out.println("messagePayload = " + messagePayload);
			System.out.println("message qos = " + messageQos);
			System.out.println("test name = " + test.getTestName());
		}
		if (test.isDuplicate(messagePayload)) {
			//oops, we have a duplicate
			test.updateDuplicateMessages(messagePayload);			
		}	
		test.updateReceivedMessages(messagePayload);
		// check message arrived on correct topic
		if (test.getNumberOfSubscribers()> 1 && test.getNumberOfPublishers() > 1) {
			// the message will have been published to a topic ending in the publisher number	
			String [] topicLevels = topicName.split("/");
			String topicNumber = topicLevels[topicLevels.length-1];
			String checkName;
			if (test.getTestNameFinal()==null || test.getTestNameFinal()=="") {
				checkName = test.getTestName();
			} else {
				checkName = test.getTestNameFinal();
			}

			if (messagePayload.contains(checkName + topicNumber)) {
				// message on correct topic!
			} else {
				System.out.println("checkName = " + checkName+ topicNumber);
				System.out.println("messagePayload = " + messagePayload);
				test.setSuccess(false);
				System.out.println("test failed - message arrived, but on wrong topic");
			}
		}
		// check whether the message should be retained
		if (test.isRetainedMessages()) {
			if (!message.isRetained()) {
				// we got a message, but it was not a retained message
				System.out.println("test failed - normal message arrived, expected a retained message");
			}
		} else {
			if (message.isRetained()) {
				// we got a message, but it was a retained message - not expected
				System.out.println("test failed - retained message arrived, expected a normal message");
				// OK nothing to report
			} 
		}
		if (test.expectedQos != 3) { // if expectedQos = 3, then we are not checking qos in the test
			if (test.expectedQos != messageQos) {
				// oops a qos mismatch
				System.out.println("test failed - qos expected: " + test.expectedQos + " but was " + messageQos);
				test.setSuccess(false);
			}
		}
	}
}
