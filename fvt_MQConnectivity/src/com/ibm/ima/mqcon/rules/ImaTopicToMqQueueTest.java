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
import com.ibm.mq.MQQueue;
import com.ibm.mq.headers.MQDataException;

/**
 * This test sends a number of messages to a named IMA topic 
 * and retrieves them from a named MQ queue
 */
public class ImaTopicToMqQueueTest extends ImaTopicToMqDestination {

	public ImaTopicToMqQueueTest() {
		super();
		this.testName = "ImaTopicToMqQueueTest";
		this.clientID = "ImaTopicToMqQueueTest";
	}

	public static void main(String[] args) {
		boolean success = ImaTopicToMqQueueTest.run(args);
		if (success==false) {
			StaticUtilities.printFail();
		} else {
			StaticUtilities.printSuccess();
		}	
	}

	public static boolean run(String[] args) {
		ImaTopicToMqQueueTest test = null;	
		boolean runningSuccess = true;
		for (int x=0; x<=2; x++) {
			try {	
				test = new ImaTopicToMqQueueTest();
				test.qualityOfService = x;
				test.setNumberOfMessages(500);
				test.topicString = "ISMTopic/To/MQQueue";
				test.mqQueueName="ISMTopic.To.MQQueue";
				test.waitTime =20000;
				if (args.length > 0) {
					test.parseCommandLineArguments(args);
				}			
				test.setNumberOfPublishers(1);
				test.setNumberOfSubscribers(1);
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
			runningSuccess = runningSuccess && test.isSuccess(); 
		}
		return runningSuccess;
	}

	private void execute() throws MQException, MqttException, IOException, MQDataException {
		mqttcon = createMqttConnection();
		mqttcon.setPublisher();
		System.out.println("sending");
		sendMessages();
		System.out.println("all sent");
		// now check they arrived
		createMQConnection();
		MQQueue mqQueue = mqConn.getInputQueueConnection(mqQueueName);
		MQGetMessageOptions messageOptions = getMessageWaitOptions();
		MQMessage message;
		int messagesReceived = 0;
		try {
			while (messagesReceived <= getNumberOfMessages()) { // allows extra message to arrive
				message = new MQMessage();
				mqQueue.get(message, messageOptions);
				messagesReceived++;
				checkImaMqttToMQJavaQos(message.persistence);
				updateReceivedMessages(returnMqMessagePayloadAsString(message));
			}
		} catch (MQException mqe) {
			if (mqe.getMessage().contains("2033")) {
				// expected behaviour when there are no more messages
			} else {
				// all other MQ exceptions
				throw mqe;
			}
		}
	}
}
