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
package com.ibm.ism.mqbridge.test;

import java.io.IOException;
import java.util.UUID;

import com.ibm.micro.client.mqttv3.MqttClient;
import com.ibm.micro.client.mqttv3.MqttConnectOptions;
import com.ibm.micro.client.mqttv3.MqttDeliveryToken;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttMessage;
import com.ibm.micro.client.mqttv3.MqttTopic;
import com.ibm.mq.MQDestination;
import com.ibm.mq.MQException;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQQueue;
import com.ibm.mq.MQTopic;
import com.ibm.mq.constants.CMQC;

public class BasicRequestReplyTest extends MessagingTest{

public BasicRequestReplyTest(String clientID) {
		super(clientID);
	}

	//	static LoggingUtilities logger;
//	static final String messageBody = MessageUtilities.duplicateMessage("1234567890-=[]'#,./\\!\"£$%^&*()_+{}:@~<>?|qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM`¬", 10);
//	public static  MQQueueManager targetQueueManager;
	private static  MQDestination subscriber;
	public static  String topicName = "ISM/Request/";
	public static  String replyTopicName = "ISM/Reply/";
	public  String queueName;
	public  PublisherThread publisherThread;
	public  MQQueueReaderThread mqQueueReaderThread;
	MQQueue reader;
	private String [] tokenisedPayloadString;
	String sendingClient;
	String receivedMessage;
//	private boolean mqClientConn = true;
	static MqttClient client;
	static int qos = 2;
	private static String clientID;
	static RRCallBack callback;

	public String[] getTokenisedPayloadString() {
		return tokenisedPayloadString;
	}

	public void setTokenisedPayloadString(String[] tokenisedPayloadString) {
		this.tokenisedPayloadString = tokenisedPayloadString;
	}

	public static void main(String[] args) {		
		/*
		 * send message to ISM 
		 * collect message from MQ 
		 * put new message onto MQ 
		 * collect from ISM
		 */

		BasicRequestReplyTest basicRequestReplyTest = new BasicRequestReplyTest("BasicRequestReplyTest");
		basicRequestReplyTest.logger = new LoggingUtilities();
		basicRequestReplyTest.logger.printString("Starting BasicRequestReplyTest");
		basicRequestReplyTest.basicMQSetup();
		basicRequestReplyTest.setUpMqttClient();
		int options = CMQC.MQSO_CREATE | CMQC.MQSO_WILDCARD_TOPIC;// CMQC.MQSO_WILDCARD_CHAR; // CMQC.MQSO_WILDCARD_TOPIC |
		int openAs = CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION;		
		try {
			subscriber =
				basicRequestReplyTest.targetQueueManager.accessTopic(topicName+
						"#"
						//clientID
						,null,
						openAs, options);
		} catch (MQException e1) {
			e1.printStackTrace();
		}		
		while (!subscriber.isOpen()) {
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
		try {
			basicRequestReplyTest.logger.printString("MQ subscriber " + subscriber.getName());
		} catch (MQException e4) {
			// TODO Auto-generated catch block
			e4.printStackTrace();
		}				
		basicRequestReplyTest.publishMessage();
		MQMessage message = new MQMessage();
		int noOfFails=0;
		boolean noMessageReceived = true;
		while (noMessageReceived) {
			try {
				subscriber.get(message);
				noMessageReceived =false;
			} catch (MQException e1) {
				if (e1.completionCode==2 && e1.reasonCode == 2033 && noOfFails < 30) { 
					// expected
					noOfFails++;
					basicRequestReplyTest.logger.printString("noOfFails = " + noOfFails);
					try {
						Thread.sleep(500);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
				} else {
					e1.printStackTrace();
					basicRequestReplyTest.logger.printString("terminal error - quitting test");
					System.exit(1);
				}
			}
		}
		// now check that no other messages have arrived
		MQMessage extraMessage = new MQMessage();
		try {
			subscriber.get(extraMessage);
			basicRequestReplyTest.logger.printString("extra message arrived");
			basicRequestReplyTest.logger.printString("quitting test");
			System.exit(1);
		} catch (MQException e3) {
			if (e3.completionCode==2 && e3.reasonCode ==2033) {
				basicRequestReplyTest.logger.printString("no extra messages arrived - good");
			}
		}		
		basicRequestReplyTest.logger.printString(message.toString());
		byte[] mqPayload = null;
		try {
			mqPayload = new byte[message.getDataLength()];
		} catch (IOException e1) {
			 
			e1.printStackTrace();
		}
		try {
			message.readFully(mqPayload);
		} catch (IOException e1) {
			 
			e1.printStackTrace();
		}
		String mqPayloadString = new String (mqPayload);
		basicRequestReplyTest.logger.printString(mqPayloadString);
		basicRequestReplyTest.splitMessage(mqPayloadString);			
		basicRequestReplyTest.logger.printString("sending client is " + basicRequestReplyTest.sendingClient);
		try {		
			subscriber.close();			
		} catch (MQException e) {
			 
			e.printStackTrace();
		}
		// open connection to MQ topic for ISM/Reply/sendingClient
		try {
			basicRequestReplyTest.logger.printString("qm is " + basicRequestReplyTest.targetQueueManager.getName());
		} catch (MQException e2) {
			 
			e2.printStackTrace();
		}
		MQMessage replyMessage = new MQMessage();
		String targetName = "ISM/Reply/" + basicRequestReplyTest.sendingClient;
		MQTopic publisher = null;
			try {
				publisher =
					basicRequestReplyTest.targetQueueManager.accessTopic(targetName,null,
							CMQC.MQTOPIC_OPEN_AS_PUBLICATION, CMQC.MQOO_OUTPUT);
			} catch (MQException e1) {
				 
				e1.printStackTrace();
			}			
			String messageString = basicRequestReplyTest.receivedMessage + "REPLY" + basicRequestReplyTest.sendingClient;
			byte[] byteArray  = messageString.getBytes();
			try {
				replyMessage.write(byteArray);
			} catch (IOException e2) {
				 
				e2.printStackTrace();
			}
			//	if (!isTopic) {
			// message.persistence = CMQC.MQPER_PERSISTENT;
			basicRequestReplyTest.logger.printString("incoming MQ persistence is " + message.persistence);
			replyMessage.persistence = message.persistence;			
			try {
				publisher.put(replyMessage);
			} catch (MQException e1) {
				 
				e1.printStackTrace();
			}		
		try {
			basicRequestReplyTest.targetQueueManager.disconnect();
		} catch (MQException e) {
			 
			e.printStackTrace();
		}			
		// now find the message in ISM
		while (callback.getMessagesProcessed()==0) {
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
		try {
			client.unsubscribe(topicName);
			client.disconnect();
		} catch (Exception e) {
			e.printStackTrace();
			System.exit(1);
		}		
	}
	
	private void setUpMqttClient() {
		clientID = String.format("%-23.23s", 
				System.getProperty("user.name") + "_" + 
				(UUID.randomUUID().toString())).trim().replace('-', '_');
		logger.printString("starting ISM subscriber");
		callback = new RRCallBack("instance", "request-reply", logger);
		String ismHost = "tcp://" + ConnectionDetails.ismServerHost + ":" + ConnectionDetails.ismServerPort;
		try {
			client = new MqttClient(ismHost,clientID,null);
		} catch (MqttException e) {
			 
			e.printStackTrace();
		}
		try {
			client.setCallback(callback);
		MqttConnectOptions conOptions = new MqttConnectOptions();
		conOptions.setCleanSession(false);
		client.connect(conOptions);
	//	int subscriptionQos = MQtoISMTest.messagePersistence == 1 ? 2 : 0;
		logger.printString("Subscribing to topic \"" + replyTopicName + clientID
				+ "\" for client instance \"" + client.getClientId()
				     + "\" using QoS " + qos
				+ ". Clean session is " + false);			
		client.subscribe(replyTopicName+clientID, qos);		
		} catch (MqttException e4) {
			e4.printStackTrace();
		}		
	}
	
	private void publishMessage() {
		String topicString = "ISM/Request/" + clientID;
		logger.printString("publishing message with message number " + 1);
		try {
			logger.printString("threadName is " + clientID);
			MqttTopic topic;
			MqttMessage message;  
			topic = client.getTopic(topicString);
			Thread.sleep(1000);
			String publication = "";
			byte [] publicationBytes;
			MqttDeliveryToken token;
			logger.printString("publishing to " + topic.getName());
			publication = messageBody + "TOKEN" +clientID;
			publicationBytes = publication.getBytes();
			message = new MqttMessage(publicationBytes);
			message.setQos(qos);
			logger.printString("" + ISMtoMQTest.timeNow()  +  " publication of \"" + publication
					+ "\" with QoS = " + message.getQos() + " to " + topic.getName());

			logger.printString(" on topic \"" + topic.getName()
					+ "\" for client instance: \"" + client.getClientId()
					+ "\" on address " + client.getServerURI() + "\"");
			//topic.publish(message);
			logger.printString("message length is " + publication.length());
			boolean published = false;
			token = topic.publish(message);
			while (!published) {
				try { 
					token.waitForCompletion(sleepTimeout);  
					if (logger.isVerboseLogging()) {
						logger.printString("Delivery token \"" + token.hashCode()
								+ "\" has been received: " + token.isComplete());
					}
					published = true;	  
				} 
				catch (MqttException mqtte) {
					mqtte.printStackTrace();
					Thread.sleep(sleepTimeout);
				}
			}								
		} catch (Exception e) {
			logger.printString("*******************************************************************************************");
			e.printStackTrace();
			Class<? extends Exception> clazz = e.getClass();
			String className = clazz.toString();
			logger.printString("className is " + className);
			logger.printString("*******************************************************************************************");
			System.exit(1);
		}		

	}

	private void splitMessage (String stringToSplit) {
		tokenisedPayloadString = stringToSplit.split("TOKEN");
		if (!(tokenisedPayloadString.length ==2)) {
			logger.printString("tokenisedPayloadString has length " + tokenisedPayloadString.length);

			logger.printString("tokenisedPayloadString does not have 2 elements\n");
		} else { 
			if  (tokenisedPayloadString[0].equalsIgnoreCase(messageBody)) {
				logger.printString("tokenisedPayloadString[0] matches message body\n");
			}
		}		
		sendingClient = tokenisedPayloadString[1];
		receivedMessage = tokenisedPayloadString[0];
	}	
}
