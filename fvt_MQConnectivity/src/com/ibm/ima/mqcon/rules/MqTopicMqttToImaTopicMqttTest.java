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

import java.io.IOException;

import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttPersistenceException;
import org.eclipse.paho.client.mqttv3.MqttSecurityException;

import com.ibm.ima.mqcon.utils.MqttConnection;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.ima.mqcon.utils.Trace;

public class MqTopicMqttToImaTopicMqttTest extends MqTopicToImaDestination {

	MqttConnection mqttconMq = null;
	private int mqMqttPort = 1883;
	public MqTopicMqttToImaTopicMqttTest() {
		super();
		this.testName = "MqTopicMqttToImaTopicMqttTest";
		this.clientID = "MqTMqtt2ImaTMqtt";
	}

	public static void main(String[] args) {
		boolean success = MqTopicMqttToImaTopicMqttTest.run(args);
		if (success==false) {
			StaticUtilities.printFail();
		} else {
			StaticUtilities.printSuccess();
		}
	}

	public static boolean run(String[] args) {
		MqTopicMqttToImaTopicMqttTest test = null;
		boolean overallSuccess = true;
		for (int x=0; x<=2 ; x++) { // pub qos
			for (int y=0; y<=2 ; y++) { // sub qos
				try {
					test = new MqTopicMqttToImaTopicMqttTest();
					test.setNumberOfMessages(1);
					test.setNumberOfPublishers(1);
					test.setNumberOfSubscribers(1);
					test.qualityOfService = y;
					if (y > x) {
						test.expectedQos = x;
					}
					else {
						test.expectedQos = y;
					}
					test.topicString = "MQTopicMqtt/To/ISMTopicMqtt";
					if (args.length > 0) {
						test.parseCommandLineArguments(args);
					}			
					test.execute(x);
					if (!test.checkMessagesOK()) {
						System.out.println("TEST FAILED - Message Mismatch");	
						test.setSuccess(false);
					}
				} catch (MqttException e) {
					System.out.println("MQTT exception - TEST FAILED");
					e.printStackTrace();
					test.setSuccess(false);
				} catch (IOException e) {
					System.out.println("IO exception - TEST FAILED");
					e.printStackTrace();
					test.setSuccess(false);
				} catch (Exception e) {
					System.out.println("Exception - TEST FAILED");
					e.printStackTrace();
					test.setSuccess(false);
				} finally {
					try {
						if (test.mqttcon!=null) {
							test.mqttcon.destroyClient();
						}
					} catch (MqttSecurityException mse) {
						mse.printStackTrace();
						System.out.println("IMA MQTT Security exception - TEST FAILED");
						test.setSuccess(false);
					} catch (MqttException me) {
						me.printStackTrace();
						System.out.println("IMA MQTT exception - TEST FAILED");
						test.setSuccess(false);
					}
					try {
						if (test.mqttconMq !=null) {
							test.mqttconMq.destroyClient();
						}
					} catch (MqttSecurityException mse) {
						mse.printStackTrace();
						System.out.println("MQ MQTT Security exception - TEST FAILED");
						test.setSuccess(false);
					} catch (MqttException me) {
						me.printStackTrace();
						System.out.println("MQ MQTT exception - TEST FAILED");
						test.setSuccess(false);
					}
					overallSuccess = overallSuccess && test.isSuccess();
				}
			}
		}
		return overallSuccess;		
	}

	private void execute(int pubQuality) throws MqttException, IOException {
		mqttconMq = new MqttConnection(mqHostName, mqMqttPort , clientID, cleanSession, pubQuality);
		mqttcon = createMqttConnection();
		mqttconMq.setPublisher();
		MqconMqttCallBack ttCallback = new MqconMqttCallBack(this);
		try {
			mqttcon.createClient(ttCallback);
			mqttcon.createSubscription(topicString);
		} catch (MqttSecurityException mse) {
			mse.printStackTrace();
			System.out.println("FAILED");
			setSuccess(false);
		} catch (MqttException me) {
			me.printStackTrace();
			System.out.println("FAILED");
			setSuccess(false);
		}
		while (!mqttcon.getClient().isConnected())
		{
			StaticUtilities.sleep(1000);
		}
		sendMessages(topicString, testName, pubQuality);
		// now sleep while they arrive		
		StaticUtilities.sleep(waitTime);
	}

	protected void sendMessages(String topicName, String testString, int pubQuality) throws MqttPersistenceException, MqttException {	
		if (Trace.traceOn()){	
			System.out.println("Sending " + testString + " " + + getNumberOfMessages() + " times: " );
		}
		for (int count = 1; count <= getNumberOfMessages(); count++) {
			String publication = testString + "messNo" +count;
			MqttMessage mqttm = new MqttMessage(publication.getBytes());
			mqttm.setQos(pubQuality);
			mqttconMq.sendMessage(topicName, mqttm);
			addMessage(publication);
			if (Trace.traceOn()){
				System.out.println("message sent: " + publication + " qos " + mqttm.getQos());
			}
		}
		if (Trace.traceOn()){	
			System.out.println("all messages sent");
		}	
	}

}
