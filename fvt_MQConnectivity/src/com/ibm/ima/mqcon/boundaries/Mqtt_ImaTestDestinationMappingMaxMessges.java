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

import java.sql.Timestamp;
import java.util.UUID;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import com.ibm.ima.mqcon.msgconversion.Mqtt_Mqtt;
import com.ibm.ima.mqcon.msgconversion.ReturnCodes;
import com.ibm.ima.mqcon.utils.MqttConnection;
import com.ibm.ima.mqcon.utils.Trace;

/**
 *
 * Test publishes MQTT messages to an IMA topic to test the Maximum Messages value
 * of a Destination Mapping Rule.
 *
 * It checks that the client is disconnected at the appropriate time.
 *
 */
public class Mqtt_ImaTestDestinationMappingMaxMessges extends Mqtt_Mqtt {

	private String msg1Body = null;
	int currentMsgNo = 0;

	public static void main(String[] args) {

		new Trace().traceAutomation();
		Mqtt_ImaTestDestinationMappingMaxMessges testClass = new Mqtt_ImaTestDestinationMappingMaxMessges();
		testClass.setNumberOfPublishers(1);
		testClass.setTestName("Mqtt_ImaToIma");

		// Get the hostname and port passed in as a parameter
		try {
			if (args.length == 0) {
				if (Trace.traceOn()) {
					Trace.trace("\nTest aborted, test requires command line arguments!! (IMA IP address, IMA port, IMA topic, timeout and number of messages)");
				}
				RC = ReturnCodes.INCORRECT_PARAMS;
				System.exit(RC);
			} else if (args.length == 5) {
				imahostname = args[0];
				imaport = new Integer((String) args[1]);
				imaDestinationName = args[2];
				timeout = new Integer((String) args[3]);
				testClass.setNumberOfMessages(Integer.parseInt((String) args[4]) + 1);
				if (Trace.traceOn()) {
					Trace.trace("The arguments passed into test class are: "
							+ imahostname + ", " + imaport + ", "
							+ imaDestinationName + ", " + timeout + ", "
							+ (testClass.getNumberOfMessages() - 1));
				}
			} else {
				if (Trace.traceOn()) {
					Trace.trace("\nInvalid Commandline arguments! System Exit.");
					Trace.trace("The arguments needed are: imahostname, imaport, imaDestinationName, timeout and number of messages");
				}
				RC = ReturnCodes.INCORRECT_PARAMS;
				System.exit(RC);
			}

			for (int i = 0; i < testClass.getNumberOfMessages(); i++){
				testClass.sendMQTTMessages(testClass.getNumberOfMessages());
			}
			

		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void sendMQTTMessages(int numberOfCycles) {
		try {
			
			String time = new Timestamp(System.currentTimeMillis()).toString();
			if (Trace.traceOn()) {
				Trace.trace(time + "\tCreating the message producer");
			}
			
			String clientID = UUID.randomUUID().toString().substring(0, 10).replace('-', '_');
			mqttProducerConnection = new MqttConnection(imahostname, imaport, clientID, true, 2);
			msg1Body = "";

			MqttMessage msg1 = new MqttMessage(msg1Body.getBytes());
			msg1.setQos(2);
			mqttProducerConnection.setPublisher();
			MqttClient producer = mqttProducerConnection.getClient();
			MqttTopic topic = producer.getTopic(imaDestinationName);

			for (int i = 0; i < numberOfCycles; i++) {
				currentMsgNo++;
				msg1Body = Integer.toString(currentMsgNo);
				msg1.setPayload(msg1Body.getBytes());

				if (Trace.traceOn()) {
					if (currentMsgNo % 100 == 0){
						time = new Timestamp(System.currentTimeMillis()).toString();
						Trace.trace(time + "\tSending message: " + msg1Body + " with QOS=" + msg1.getQos());
					}
				}

				try{
					MqttDeliveryToken token = topic.publish(msg1);
					token.waitForCompletion(5000);
				} catch (MqttException mqtte){
					time = new Timestamp(System.currentTimeMillis()).toString();
					if (currentMsgNo == numberOfCycles) {
						Trace.trace(time + "\tTest passed - Connection lost at the expected time");
						RC = ReturnCodes.PASSED;
						System.out.println("Test.*Success!");
						closeConnections();
						System.exit(RC);
					} else if (currentMsgNo > numberOfCycles) {
						Trace.trace(time + "\tTest Failed - Too many messages could be sent");
						RC = ReturnCodes.MESSAGE_RECEIVED_INCORRECTLY;
						System.out.println("Test.*Failed!");
						closeConnections();
						System.exit(RC);
					} else if (currentMsgNo < numberOfCycles) {
						Trace.trace(time + "\tTest Failed - Not enough messages could be sent");
						RC = ReturnCodes.MESSAGE_RECEIVED_INCORRECTLY;
						System.out.println("Test.*Failed!");
						closeConnections();
						System.exit(RC);
					} else {
						Trace.trace(time + "\tTest Failed - Unknown problem");
						RC = ReturnCodes.MQTT_EXCEPTION;
						System.out.println("Test.*Failed!");
						closeConnections();
						System.exit(RC);
					}
				}
			}
		}
		catch (MqttException mqtte) {
			String time = new Timestamp(System.currentTimeMillis()).toString();
			Trace.trace(time + "\tAn exception occurred whilst attempting the test. Exception="+ mqtte.getMessage());
			mqtte.printStackTrace();
			RC = ReturnCodes.MQTT_EXCEPTION;
			System.out.println("Test.*Fail!");
			closeConnections();
			System.exit(RC);
		}

		if (Trace.traceOn()) {
			String time = new Timestamp(System.currentTimeMillis()).toString();
			Trace.trace(time + "\tTest failed - Client did not automatically lose connection for some reason. Check values.");
		}
			RC = ReturnCodes.INCOMPLETE_TEST;
			System.out.println("Test.*Fail!");
			closeConnections();
			System.exit(RC);
	}

	@Override
	public void callback(MqttMessage msg) {
		// TODO Auto-generated method stub
		//NOT USED - NOT RECEIVING MESSAGES
	}
}
