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

/**
 * This test sends a number of messages to a named MQ topic with 32 levels and retrieves them from
 * an ISM topic of the same name
 */

public class MqToImaTooManyLevelsTest extends MqTopicToImaDestination {

	int extraLevels =30;
	public MqToImaTooManyLevelsTest() {
		super();
		this.testName = "MqToImaTooManyLevelsTest";
		this.clientID = "MqToImaTMLTest";
	}

	public static void main(String[] args) {
		boolean runningSuccess = true;
		for (int xl=28; xl<=28; xl++) {		
			MqToImaTooManyLevelsTest test = null;
			try {
				test = new MqToImaTooManyLevelsTest();
				test.extraLevels= xl;
				test.setNumberOfMessages(1);
				test.setNumberOfPublishers(1);
				test.setNumberOfSubscribers(1);
				test.topicString = "too/many/levels/levels";
				test.waitTime = 15000;
				if (args.length > 0) {
					test.parseCommandLineArguments(args);
				}			
				test.execute();
				if (xl==28) { // messages should arrive
					if (!test.checkMessagesOK()) {
						System.out.println("TEST FAILED - Message Mismatch");	
						test.setSuccess(false);
					}
				} else { // messages should not arrive
					if (test.checkMessagesOK()) {
						System.out.println("TEST FAILED - Message Mismatch");	
						test.setSuccess(false);
					}
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
				runningSuccess = runningSuccess && test.isSuccess();
			}
		}
		if (runningSuccess==false) {
			System.exit(1);
		} else {
			StaticUtilities.printSuccess();
		}
	}

	private void execute() throws MQException {
		createMQConnection();
		String wildCardOriginalTopic = topicString+"/#";
		// extend the topic string
		for (int x = 0; x< extraLevels; x++) {
			topicString = topicString + "/extralevel";
		}
		createTopic();
		mqttcon = createMqttConnection();
		MqconMqttCallBack ttCallback = new MqconMqttCallBack(this);
		try {
			mqttcon.createClient(ttCallback);
			mqttcon.createSubscription(wildCardOriginalTopic);
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
		sendMessages();
		// wait for a while
		StaticUtilities.sleep(waitTime);
	}
}

