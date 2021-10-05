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
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.mq.MQException;
import com.ibm.mq.MQGetMessageOptions;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQTopic;
import com.ibm.mq.headers.MQDataException;

/**
 * This test sends a number of messages to a named IMA topic 
 * and retrieves them from an MQ topic of the same name
 */
public class ImaTopicMqttToMqTopicMqJavaTest extends ImaTopicToMqDestination {

	public ImaTopicMqttToMqTopicMqJavaTest() {
		super();
		this.testName = "ImaTopicToMqTopicTest";
		this.clientID = "ImaTopicToMqJTTest";
	}

	public static void main(String[] args) {
		boolean success = ImaTopicMqttToMqTopicMqJavaTest.run(args);
		if (success==false) {
			StaticUtilities.printFail();
		} else {
			StaticUtilities.printSuccess();
		}		
	}

	public static boolean run(String[] args) {

		boolean overallSuccess = true;	
		for (int qos=0; qos <=2; qos++) {			
			ImaTopicMqttToMqTopicMqJavaTest test = null;		
			try {
				test = new ImaTopicMqttToMqTopicMqJavaTest();
				test.qualityOfService = qos;
				test.setNumberOfMessages(5001);
				test.setNumberOfPublishers(1);
				test.setNumberOfSubscribers(1);
				test.topicString = "ISMTopic/To/MQTopic";
				test.waitTime = 10000;
				if (args.length > 0) {
					test.parseCommandLineArguments(args);
				}			
				test.execute();
				if (!test.checkMessagesOK()) {
					System.out.println("TEST FAILED - Message Mismatch");	
					test.setSuccess(false);
				}
			} catch (MQException e) {
				System.out.println("MQ exception - TEST FAILED");
				e.printStackTrace();
				test.setSuccess(false);
			} catch (MqttException e) {
				System.out.println("MQTT exception - TEST FAILED");
				e.printStackTrace();
				test.setSuccess(false);
			} catch (IOException e) {
				System.out.println("IO exception - TEST FAILED");
				e.printStackTrace();
				test.setSuccess(false);
			} catch (MQDataException e) {
				System.out.println("MQDataException - TEST FAILED");
				e.printStackTrace();
				test.setSuccess(false);
			} catch (Exception e) {
				System.out.println("Exception - TEST FAILED");
				e.printStackTrace();
				test.setSuccess(false);
			} finally {
				try {
					if (test.mqConn !=null) {
						test.mqConn.disconnect();
					}
				} catch (MQException e) {
					System.out.println("MQ exception - TEST FAILED");
					test.setSuccess(false);
				}
				overallSuccess = test.isSuccess() && overallSuccess;
			}
		}
		return overallSuccess;
	}

	private void execute() throws MQException, MqttException, IOException, MQDataException {
		createMQConnection();
		MQTopic mqTopic = mqConn.createSubscription(topicString, false);
		mqttcon = createMqttConnection();
		mqttcon.setPublisher();
		sendMessages();
		System.out.println("all messages sent with qos " + qualityOfService);
		// now disconnect - allows the server to be killed
		try {
			if (mqttcon!=null) {
				mqttcon.destroyClient();
			}
		} catch (MqttSecurityException mse) {
			mse.printStackTrace();
			System.out.println("MQTT Security exception - TEST FAILED");
			setSuccess(false);
			throw mse;
		} catch (MqttException me) {
			me.printStackTrace();
			System.out.println("MQTT exception - TEST FAILED");
			setSuccess(false);
			throw me;
		}
		// now check they arrived
		MQGetMessageOptions messageOptions = getMessageWaitOptions();
		MQMessage message;
		int messagesReceived = 0;
		try {
			while (messagesReceived <= getNumberOfMessages()) { // allows extra message to arrive
				message = new MQMessage();
				messagesReceived++;			
				mqTopic.get(message, messageOptions);
				updateReceivedMessages(returnMqMessagePayloadAsString(message));
				checkImaMqttToMQJavaQos(message.persistence);
			}
		} catch (MQException mqe ){
			if (mqe.getMessage().contains("2033")) {
				// expected
			}	else {
				throw mqe;
			}
		}
	}
}
