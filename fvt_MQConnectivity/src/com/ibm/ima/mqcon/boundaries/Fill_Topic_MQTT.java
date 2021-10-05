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
package com.ibm.ima.mqcon.boundaries;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import com.ibm.ima.mqcon.msgconversion.Mqtt_Mqtt;

public class Fill_Topic_MQTT extends Mqtt_Mqtt{
	static String clientIP = null;
	static String clientPort = null;
	static String topic = null;
	static int noMessages = 1;
	static String TCPAddress = null;
	
	/*
	 * Test to fill up a topic with messages, then publish a final message.
	 * This checks that the 'Max Messages' value of an Messaging Policy is working correctly.
	 */
	
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		/*
		 * Note - 'noMessages' value should equal the 'Max Messages' value set in your Messaging Policy.
		 * Example parameters - 9.20.123.102 16102 5000 /FillUpTopic
		 */
		if(args.length == 4){
			clientIP = args[0];
			clientPort = args[1];
			noMessages = Integer.parseInt(args[2]);				
			topic = args[3];
			TCPAddress = "tcp://" + clientIP + ":" + clientPort;
		} else {
			System.out.println("You must enter exactly 5 arguments. Client IP address, Client Port, number of messages and publish topic");
			System.exit(0);
		}
		
		String messageBody = "Message";
		MqttMessage message = new MqttMessage();
		message.setQos(2);
		
		try {
			//Connect a publisher and subscriber then subscribe to a topic.
			MqttClient subscriber = new MqttClient(TCPAddress, "FILL_DEST_SUB");
			MqttClient publisher = new MqttClient(TCPAddress, "FILL_DEST_PUB");
			System.out.println("Created subscriber and publisher.");
			subscriber.connect();
			publisher.connect();
			System.out.println("Connected subscriber and publisher.");
			subscriber.subscribe(topic);
			System.out.println("Subscribed to topic " + topic);
			
			//Publish QoS 2 messages, which should fill up topic as subscriber is not
			//receiving messages.
			System.out.println("Beginning publish of " + noMessages + " messages.");
			for(int i=1; i<=noMessages; i++){
				messageBody = "Message " + i;
				message.setPayload(messageBody.getBytes());
				//Attempt to publish each message
				try {
					publisher.publish(topic, message);
				} catch (Exception e) {
					System.out.println("Test Failed - Not enough messages could be published");
					System.exit(0);
				}
			}
			
			//Attempt to publish a final message, which *should* disconnect publisher.
			System.out.println("Attempting to publish one extra message");
			messageBody = "FinalMessage";
			message.setPayload(messageBody.getBytes());
			try {
				publisher.publish(topic, message);
				System.out.println("Test Failed - Message could be published");
				System.exit(0);
			} catch (Exception e) {
				System.out.println("Test Passed - Message could not be published - .\tError message = " + e.getMessage());
				System.exit(0);
			}
		} catch (MqttException e) {
			System.out.println("Test Failed - Test ended at an enexpected time");
		}
	}

	//@Override
	public void callback(MqttMessage msg) {
		// TODO Auto-generated method stub
	}
}
