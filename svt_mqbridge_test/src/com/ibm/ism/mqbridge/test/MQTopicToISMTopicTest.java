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

import com.ibm.micro.client.mqttv3.MqttClient;
import com.ibm.micro.client.mqttv3.MqttConnectOptions;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttSecurityException;
import com.ibm.mq.MQException;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQTopic;
import com.ibm.mq.constants.CMQC;

public class MQTopicToISMTopicTest extends MessagingTest {

public MQTopicToISMTopicTest(String clientID) {
		super(clientID);
		// TODO Auto-generated constructor stub
	}

//	public static int NUMBEROFMESSAGES = 100;
	private static String testName = "MQTopicToISMTopicTest";
//	private static LoggingUtilities logger;
//	private static final String clientID = "NMTTNITT";
	private static String MQTopic = "MQTopic/To/ISMTopic";
//	private static String MQTopic = "MQ/1";
//	private static String ISMTopic = "MQ/1";
	private static String ISMTopic = "MQTopic/To/ISMTopic";
//	public static int [][]expectedQos = {{0,0,0},{0,1,2}};
	private static MQTopic mqDest;
	private static MqttClient client;
	static int persToTest[] =  {CMQC.MQPER_PERSISTENT , CMQC.MQPER_NOT_PERSISTENT }; //  
	static int currentPersistence;
//	static MessageUtilities messageUtilities;
//	private static int subscriptionQos =2;
	static int rc =0;
	TTCallBack callback;
	static MQTopicToISMTopicTest test;

	public static void main(String[] args) {
		test= new MQTopicToISMTopicTest("NMTTNITT");
		if (args.length > 0) {
		test.parseInput(args);	
		}
		test.logger.printString("test is " + testName);
		MessagingTest.numberOfSubscribers =1;
		MessagingTest.currentQualityOfService = MessagingTest.startingQualityOfService;
		
		while (MessagingTest.currentQualityOfService <= MessagingTest.finalQualityOfService) {
			for (int x: persToTest) {
				messageUtilities = new MessageUtilities(test.logger);
				currentPersistence = x;
				// create subscription
				test.createISMSubscription();
				// set up MQ client connection
				ConnectionDetails.setUpMqClientConnectionDetails();
				// publish messages
				test.publishMessages();					
				test.callback.setTimeLastCalled(System.currentTimeMillis());
				// wait while they all arrive
				// stop after last message or 10 seconds without a message - may not catch duplicates.
				while ((messageUtilities.getPayloads().size()>0 )|| test.callback.getLastCalled() > MessagingTest.waitInSeconds ) {
					try {
						test.logger.printString("sleeping 1 sec. Waiting for: " + messageUtilities.getPayloads().size() + " messages ");
						Thread.sleep(1000);
					} catch (InterruptedException e1) {
						test.logger.printString(e1.getMessage());
						e1.printStackTrace();
					}
				}
				// check messages
				test.checkMessages();
				// unsubscribe from ISM
				try {
					client.unsubscribe(ISMTopic);
					client.disconnect();
				} catch (MqttException e) {
					e.printStackTrace();
				}
			}
			MessagingTest.currentQualityOfService++;
		}
		// return fail!
		//return rc;		
	}

	private void createISMSubscription() {
		logger.printString("starting subscription");
		try {
			client = new MqttClient("tcp://" +
					ConnectionDetails.ismServerHost +
					":"  + ConnectionDetails.ismServerPort , test.clientID,null);
			test.callback = new TTCallBack(test.clientID, testName, logger);
			test.callback.setDrainingMessages(false);
			test.callback.setMessUtil(messageUtilities);
			test.callback.setExpectedQos(expectedQos[currentPersistence][MessagingTest.currentQualityOfService]);			
			client.setCallback(test.callback);
			MqttConnectOptions conOptions = new MqttConnectOptions();
			conOptions.setCleanSession(false);
			client.connect(conOptions);
		} 
		catch (MqttException e) {
			logger.printString("no connection");
			e.printStackTrace();
			rc =1;
		}		
		logger.printString("Subscribing to topic \"" + ISMTopic
				+ "\" for client instance \"" + client.getClientId()
				+ "\" using QoS " + MessagingTest.currentQualityOfService 
				+ ". Clean session is " + false);			
		try {
			client.subscribe(ISMTopic, MessagingTest.currentQualityOfService);
		} catch (MqttSecurityException e2) {
			e2.printStackTrace();
		} catch (MqttException e2) {
			e2.printStackTrace();
		}
	}

	private void publishMessages() {
		// connect to QM
		try {
			mqDest =
				ConnectionDetails.getMqQueueManager().accessTopic(MQTopic,null,
						CMQC.MQTOPIC_OPEN_AS_PUBLICATION, CMQC.MQOO_OUTPUT);
		} catch (MQException e) {
			e.printStackTrace();
		}
		// publish messages, each with individual number
		// add each message to array of payloads
		MQMessage message;
		for (int count =0; count < numberOfMessages; count++) {
			message = new MQMessage();
			String identifier = testName + count+"pub" + "persistence"+currentPersistence;
			synchronized (messageUtilities.payloadsLock) {
				messageUtilities.getPayloads().add(identifier);
				messageUtilities.getOriginalPayloads().add(identifier);
			}
			String messageString = messageUtilities.messageBody + "TOKEN" + identifier;
			byte[] byteArray = messageString.getBytes();
			try {
				message.write(byteArray);
			} catch (IOException e1) {
				e1.printStackTrace();
			}
			message.persistence = currentPersistence ;
			logger.printString("publication of " + identifier
					+ " to topic " + MQTopic + " with persistence " + message.persistence + " message length " + messageString.length());
			try {
				mqDest.put(message);
			} catch (MQException e) {
				e.printStackTrace();
			}			
		}	
		// disconnect from QM
		try {
			logger.printString("disconnecting from MQ");
			mqDest.close();
		} catch (MQException e) {
			e.printStackTrace();
		}
	}

//	private static void parseArgs(String[] args) {
//		String argsValues = "";
//		for (String value : args) {
//			argsValues = argsValues + value +" ";
//		}
//		logger.printString("args is " + argsValues);		
//		// iterate through arguments and assign parameters
//		try {
//			for (int x = 0; x< args.length;x++) {
//				if (args[x].startsWith("-")){
//					char parameter = args[x].charAt(1);
//					switch (parameter) {
//					case 'n':
//						x++;
//						numberOfMessages = Integer.valueOf(args[x]);
//						break;
//					case 'f' :
//						x++;
//						logger.setLogFile(args[x]);
//						logger.setLog(logger.getLogFile());
//						break;
//					case 'v' :
//						x++;
//						if (args[x].equalsIgnoreCase("true")) {
//							logger.setVerboseLogging(true);
//						} else if (args[x].equalsIgnoreCase("false")) {
//							logger.setVerboseLogging(false);
//						} else {
//							System.out.println("-v parameter takes only true and false as arguments");
//							System.exit(1);
//						}
//					case 'm' :
//						x++;
//						MQTopic = args[x];
//						break;
//					case 'i' :
//						x++;
//						ISMTopic = args[x];
//						break;						
//					}
//				} else {
//					System.out.println("invalid argument " + args);
//					System.exit(1);
//				}
//			}
//		} catch (Exception e) {
//			System.out.println("exception caught " + e.toString());
//			System.exit(1);
//		}
//	}
	
	public void checkMessages() {
		logger.printString(messageUtilities.checkMessagesInt(numberOfMessages));
	}
}
