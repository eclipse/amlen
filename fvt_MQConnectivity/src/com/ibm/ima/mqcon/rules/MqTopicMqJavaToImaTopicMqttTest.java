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

import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttSecurityException;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.mq.MQException;
import com.ibm.mq.constants.CMQC;

/**
 * This test sends a number of messages to a named MQ topic and retrieves them from
 * an IMA topic of the same name
 */

public class MqTopicMqJavaToImaTopicMqttTest extends MqTopicToImaDestination {

	public MqTopicMqJavaToImaTopicMqttTest() {
		super();
		this.testName = "MqTopicMqJavaToImaTopicMqttTest";
		this.clientID = "MqTpcMJ2ImaTpcMqttTest";
	}

	public static void main(String[] args) {
		boolean passed = MqTopicMqJavaToImaTopicMqttTest.run(args);		
		if (passed) {
			StaticUtilities.printSuccess();
		} else {
			StaticUtilities.printFail();
		}
	}

	public static boolean run(String[] args) {
		boolean runningSuccess = true;
		int [] mqPersistences = {CMQC.MQPER_NOT_PERSISTENT,CMQC.MQPER_PERSISTENT};
		for (int x : mqPersistences) {
			for (int qos=0; qos <=2; qos++) {
				MqTopicMqJavaToImaTopicMqttTest test = null;
				try {
					test = new MqTopicMqJavaToImaTopicMqttTest();
					test.setNumberOfMessages(1);
					test.setNumberOfPublishers(1);
					test.setNumberOfSubscribers(1);
					test.topicString = "MQTopic/To/ISMTopic";
					if (args.length > 0) {
						test.parseCommandLineArguments(args);
					}	
					test.mqper = x;
					test.qualityOfService = qos;
					test.expectedQos = test.expectedQosArray[test.mqper][test.qualityOfService];
					test.execute();
					if (!test.checkMessagesOK()) {
						System.out.println("TEST FAILED - Message Mismatch");	
						test.setSuccess(false);
					}
				} catch (MQException e) {
					System.out.println("MQ exception - TEST FAILED");
					e.printStackTrace();
					test.setSuccess(false);
				} catch (Exception e) {
					System.out.println("Exception - TEST FAILED");
					e.printStackTrace();
					test.setSuccess(false);
				} finally {
					try {
						if (test.mqttcon!=null) {
							test.mqttcon.unsubscribe(test.topicString);
							test.mqttcon.destroyClient();
						}
					} catch (MqttSecurityException mse) {
						mse.printStackTrace();
						System.out.println("MQTT Security exception - TEST FAILED");
						test.setSuccess(false);
					} catch (MqttException me) {
						me.printStackTrace();
						System.out.println("MQTT exception - TEST FAILED");
						test.setSuccess(false);
					}
					try {
						if (test.mqConn !=null) {
							test.mqConn.disconnect();
						}
					} catch (MQException e) {
						System.out.println("MQ exception - TEST FAILED");
						test.setSuccess(false);
					}	
					runningSuccess = test.isSuccess() && runningSuccess;
				}
			}
		}		
		return runningSuccess;
	}

	private void execute() throws MQException {
		createMQConnection();
		createTopic();
		mqttcon = createMqttConnection();
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
		StaticUtilities.sleep(1000);
		while (!mqttcon.getClient().isConnected())
		{
			StaticUtilities.sleep(1000);
		}
		sendMessages();
		// wait for a while
		StaticUtilities.sleep(waitTime);
	}
}
