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
 * This test sends a number of messages to named MQ topics (base name + /<publisher number>)
 * and retrieves them from IMA topics with the names
 */
public class MqTopicSubtreeToImaSubtreeTest extends MqTopicToImaDestination {

	public MqTopicSubtreeToImaSubtreeTest() {
		super();
		this.testName = "MqTopicSubtreeToImaTopicSubtreeTest";
		this.testNameFinal = "MqTopicSubtreeToImaTopicSubtreeTest";
		this.clientID = "MqTSToITSTest";
	}

	public static void main(String[] args) {
		boolean success = MqTopicSubtreeToImaSubtreeTest.run(args);
		if (success==false) {
			StaticUtilities.printFail();
		} else {
			StaticUtilities.printSuccess();
		}
	}

	public static boolean run(String[] args) {
		boolean runningSuccess = true;
		int [] mqPersistences = {CMQC.MQPER_NOT_PERSISTENT,CMQC.MQPER_PERSISTENT};
		for (int x : mqPersistences) {
			for (int qos=0; qos <=2; qos++) {
				MqTopicSubtreeToImaSubtreeTest test = null;
				try {
					test = new MqTopicSubtreeToImaSubtreeTest();
					test.setNumberOfMessages(1);
					test.topicString = "MQTopicSubtree/To/ISMTopicSubtree";
					test.setNumberOfPublishers(5);
					test.setNumberOfSubscribers(5);
					test.mqper = x;
					test.qualityOfService = qos;
					test.expectedQos = test.expectedQosArray[test.mqper][test.qualityOfService];
					if (args.length > 0) {
						test.parseCommandLineArguments(args);
					}			
					test.execute();
					if (test.messagesExpected) {
						if (!test.checkMessagesOK()) {
							System.out.println("Message Mismatch - TEST FAILED");	
							test.setSuccess(false);
						}
					} else {
						if (test.getReceivedMessages().isEmpty()) {
							System.out.println("No messages - correct");
						} else {
							System.out.println("Messages received = " + test.getReceivedMessages().size());
							test.setSuccess(false);
						}
					}
				} catch (MQException e) {
					System.out.println("MQ exception - TEST FAILED");
					test.setSuccess(false);
					e.printStackTrace();
				} catch (MqttException e) {
					System.out.println("MQTT exception - TEST FAILED");
					test.setSuccess(false);
					e.printStackTrace();
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
				}
				runningSuccess = runningSuccess && test.isSuccess();
			}
		}
		return runningSuccess;
	}

	private void execute() throws MQException, MqttException {
		createMQConnection();
		mqttcon = createMqttConnection();
		MqconMqttCallBack ttCallback = new MqconMqttCallBack(this);
		mqttcon.createClient(ttCallback);
		int [] qosArray = new int [numberOfSubscribers];
		String [] topicArray = new String[numberOfSubscribers];
		for (int x =0; x< numberOfSubscribers; x++) {
			qosArray[x] = qualityOfService;
			topicArray[x] = topicString + "/"+ x;
		}
		mqttcon.createMultipleSubscriptions(topicArray, qosArray);
		while (!mqttcon.getClient().isConnected())
		{
			StaticUtilities.sleep(1000);
		}
		String tn = testName;
		for (int x=0; x< getNumberOfPublishers() ; x++) {
			testName = tn + x;
			createTopic(topicString+"/"+ x);
			sendMessages();
		}
		testName = tn;
		// wait for a while - may need to increase time for more messages
		StaticUtilities.sleep(waitTime + (30 * numberOfSubscribers * numberOfMessages));	
		mqttcon.unsubscribeFromMultipleSubscriptions(topicArray);
	}
}

