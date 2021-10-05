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
import java.util.List;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttSecurityException;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.mq.MQException;
import com.ibm.mq.MQGetMessageOptions;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQTopic;
import com.ibm.mq.headers.MQDataException;

/**
 * This test sends 1 message to a named ISM topic 
 * and retrieves copies from MQ topics of the same base name plus a number(starting from 0)
 */
public class ImaTopicToMultipleMqTopicsTest extends ImaTopicToMqDestination {

	public ImaTopicToMultipleMqTopicsTest() {
		super();
		this.testName = "ImaTopicToMultipleMqTopicsTest";
		this.clientID ="ImaTopic2xMqTopics";
	}

	public static void main(String[] args) {
		boolean success = ImaTopicToMultipleMqTopicsTest.run(args);
		if (success==false) {
			StaticUtilities.printFail();
		} else {
			StaticUtilities.printSuccess();
		}
	}

	public static boolean run(String[] args) {
		ImaTopicToMultipleMqTopicsTest test = null;		
		try {
			test = new ImaTopicToMultipleMqTopicsTest();
			test.setNumberOfMessages(1);
			test.setNumberOfPublishers(1);
			test.setNumberOfSubscribers(3);
			test.topicString = "ISMTopic/To/MultipleMQTopics";
			test.qualityOfService = 2;
			if (args.length > 0) {
				test.parseCommandLineArguments(args);
			}			

			test.execute();

			// all messages are in duplicate messages.
			List<String> list = test.getDuplicateMessages();
			System.out.println("received " + list.size());
			if (list.size() != (test.numberOfSubscribers * test.numberOfMessages)) {
				System.out.println("wrong number of messages");
				test.setSuccess(false);
			} else {
				// we got the right number of messages
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
				if (test.mqttcon!=null) {
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
		return test.isSuccess();
	}

	private void execute() throws MQException, MqttException, IOException, MQDataException {
		//		messagesReceived = new String[numberOfSubscribers];
		createMQConnection();
		MQTopic [] mqTopics = new MQTopic[numberOfSubscribers];
		for (int x=0; x< numberOfSubscribers; x++) {
			mqTopics[x] = mqConn.createSubscription(topicString+x, false);
		}
		mqttcon = createMqttConnection();
		mqttcon.setPublisher();
		sendMessages();
		MQGetMessageOptions messageOptions = getMessageWaitOptions();
		// now check they arrived	
		MQMessage message;
		for (int x=0; x< numberOfSubscribers; x++) {			
			try {
				while (true) { // keep going round until we time out
					message = new MQMessage();
					mqTopics[x].get(message, messageOptions);
					checkImaMqttToMQJavaQos(message.persistence);
					String payload = returnMqMessagePayloadAsString(message);
					// check payload is correct				
					updateDuplicateMessages(payload);
					if (!sentMessages.contains(payload)) {
						// message is incorrect
						setSuccess(false);
						System.out.println("got incorrect message " + payload + " from " + mqTopics[x].getName());
					}	
					checkImaMqttToMQJavaQos(message.persistence);
					messageOptions.waitInterval =2000;
				}
			} catch (MQException mqe ){
				if (mqe.getMessage().contains("Reason '2033'")) {
					// expected
				} else {
					throw mqe;
				}
			}			
		}
	}
}

