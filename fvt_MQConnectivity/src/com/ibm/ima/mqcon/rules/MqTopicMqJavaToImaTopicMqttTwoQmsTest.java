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
import com.ibm.ima.mqcon.utils.MQConnection;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.ima.mqcon.utils.Trace;
import com.ibm.mq.MQException;
import com.ibm.mq.constants.CMQC;

public class MqTopicMqJavaToImaTopicMqttTwoQmsTest extends MqTopicToImaDestination {

	public MqTopicMqJavaToImaTopicMqttTwoQmsTest() {
		super();
		this.testName = "MqTopicMqJavaToImaTopicMqttTest";
		this.clientID = "MqTpcMJ2ImaTpcMqttTest";
	}

	public static void main(String[] args) {
		boolean runningSuccess = true;
		int [] mqPersistences = {CMQC.MQPER_NOT_PERSISTENT,CMQC.MQPER_PERSISTENT};
		for (int x : mqPersistences) {
			for (int qos=0; qos <=2; qos++) {
				MqTopicMqJavaToImaTopicMqttTwoQmsTest test = null;
				try {
					test = new MqTopicMqJavaToImaTopicMqttTwoQmsTest();
					test.setNumberOfMessages(1);
					test.setNumberOfPublishers(2);
					test.setNumberOfSubscribers(1);
					test.topicString = "MQTopic/To/ISMTopic/FanIn2QM2QMC";
					test.setSecondmqChannel("IMA.SC");
					test.setSecondmqHostName("9.20.230.213");
					test.setSecondQueueManagerName("q1");
					test.setSecondmqPort(1414);
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
		if (runningSuccess) {
			StaticUtilities.printSuccess();
		} else {
			StaticUtilities.printFail();
		}
	}

	private void execute() throws MQException {
		createMQConnection();
		createSecondMQConnection();
		createTopic();
		secondMqTopic = secondMqConn.accessTopic(topicString);
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
		while (!mqttcon.getClient().isConnected())
		{
			StaticUtilities.sleep(1000);
		}
		sendMessages(); // sends to first connection
		sendMessagesToSecondMqConnection();
		// wait for a while
		StaticUtilities.sleep(waitTime);
	}


	private void sendMessagesToSecondMqConnection() {
		try {
			if (Trace.traceOn()){	
				System.out.println("Sending " + testName + " " + + getNumberOfMessages() + " times: " );
			}
			for (int count = 1; count <= getNumberOfMessages(); count++) {
				String publication = testName + "Con2messNo" +count;
				secondMqConn.sendMessage(secondMqTopic,publication , mqper);
				addMessage(publication);
			}
			if (Trace.traceOn()){	
				System.out.println("all messages sent");
			}
		} catch (MQException e) {
			System.out.println("FAILED");
			e.printStackTrace();
		}	
	}

	protected void createSecondMQConnection() throws MQException {
		this.secondMqConn = new MQConnection(secondQueueManagerName, secondmqHostName, secondmqPort, secondmqChannel);
	}
}
