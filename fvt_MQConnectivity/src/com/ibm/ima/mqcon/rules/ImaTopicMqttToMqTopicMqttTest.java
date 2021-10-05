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
import org.eclipse.paho.client.mqttv3.MqttSecurityException;
import com.ibm.ima.mqcon.utils.MqttConnection;
import com.ibm.ima.mqcon.utils.StaticUtilities;

public class ImaTopicMqttToMqTopicMqttTest extends ImaTopicToMqDestination {

	MqttConnection mqttconMq = null;
	private int mqMqttPort = 1883;
	public ImaTopicMqttToMqTopicMqttTest() {
		super();
		this.testName = "ImaTopicMqttToMqTopicMqttTest";
		this.clientID = "ITMqtt2MqTMqtt2";
	}

	public static void main(String[] args) {
		boolean success = ImaTopicMqttToMqTopicMqttTest.run(args);	
		if (success==false) {
			StaticUtilities.printFail();
		} else {
			StaticUtilities.printSuccess();
		}	
	}

	public static boolean run(String[] args) {
		ImaTopicMqttToMqTopicMqttTest test = null;
		boolean overallSuccess = true;
		for (int x=0; x<=2 ; x++) { // pub qos
			for (int y=0; y<=2 ; y++) { // sub qos
				try {
					test = new ImaTopicMqttToMqTopicMqttTest();
					test.setNumberOfMessages(1);
					test.setNumberOfPublishers(1);
					test.setNumberOfSubscribers(1);
					test.qualityOfService = x;
					if (x > y) {
						test.expectedQos = y;
					}
					else {
						test.expectedQos = x;
					}
					test.topicString = "ISMTopicMqtt/To/MQTopicMqtt";
					if (args.length > 0) {
						test.parseCommandLineArguments(args);
					}			
					test.execute(y);
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

	private void execute(int subQuality) throws MqttException, IOException {
		mqttconMq = new MqttConnection(mqHostName, mqMqttPort , clientID, cleanSession, subQuality);
		mqttcon = createMqttConnection();
		mqttcon.setPublisher();
		MqconMqttCallBack ttCallback = new MqconMqttCallBack(this);
		try {
			mqttconMq.createClient(ttCallback);
			mqttconMq.createSubscription(topicString);			
		} catch (MqttSecurityException mse) {
			mse.printStackTrace();
			System.out.println("FAILED");
			setSuccess(false);
		} catch (MqttException me) {
			me.printStackTrace();
			System.out.println("FAILED");
			setSuccess(false);
		}
		while (!mqttconMq.getClient().isConnected())
		{
			StaticUtilities.sleep(1000);
		}
		sendMessages();
		// now sleep while they arrive		
		StaticUtilities.sleep(waitTime);
		mqttconMq.unsubscribe(topicString);
	}
}
