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
import com.ibm.micro.client.mqttv3.MqttDeliveryToken;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttMessage;
import com.ibm.micro.client.mqttv3.MqttTopic;
import com.ibm.mq.MQDestination;
import com.ibm.mq.MQEnvironment;
import com.ibm.mq.MQException;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQQueueManager;
import com.ibm.mq.constants.CMQC;

public class QMFanOutTest extends MessagingTest {
	public QMFanOutTest(String clientID) {
		super(clientID);
	}

	static String topicName = "qmfanout";
	public static  MQQueueManager targetQueueManager1;
	public static  MQQueueManager targetQueueManager2;
	//	public static LoggingUtilities logger;
	//	private static int numberOfMessages =75000;
	static int qualityOfService=2;
	// public static String messageBody = "test";
	private static int sub1Mess =0;
	private static int sub2Mess =0;
	static QMFanOutTest test;
	//	public static String messageBody = MessageUtilities.duplicateMessage("1234567890-=[]'#,./\\!\"£$%^&*()_+{}:@~<>?|qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM`¬", 1);
	//	static MessageUtilities messageUtilities = new MessageUtilities();
	/**
	 * @param args
	 */
	public static void main(String[] args) throws IOException {
		test = new QMFanOutTest("qmfanout");
		numberOfMessages =1001;
		final boolean qm1 = true;
		final boolean qm2 = true;
		// subscribe to 2 QMs
		// publish messages to ISM
		// collect messages from both QMs
		// check all messages arrived, with no duplication
		MQDestination subscription1 = null;
		if (qm1) {
			MQEnvironment.hostname = "9.20.230.120";
			MQEnvironment.channel  = "SYSTEM.ISM.SVRCONN";
			MQEnvironment.port = Integer.parseInt("1414");
			targetQueueManager1 = ConnectionDetails.getMqQueueManager("lttest");
			try {
				subscription1 = targetQueueManager1.accessTopic(topicName,null,
						CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION, CMQC.MQSO_CREATE);
			} catch (MQException e) {
				e.printStackTrace();
			}
		}
		MQDestination subscription2 = null;
		if (qm2) {
			ConnectionDetails.setUpMqClientConnectionDetails();
			//		MQEnvironment.hostname = "9.20.230.234";
			//		MQEnvironment.channel  = "SYSTEM.ISM.SVRCONN";
			//		MQEnvironment.port = Integer.parseInt("1414");
			targetQueueManager2 = ConnectionDetails.getMqQueueManager();	
			try {
				subscription2 = targetQueueManager2.accessTopic(topicName,null,
						CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION, CMQC.MQSO_CREATE);
			} catch (MQException e) {
				e.printStackTrace();
			}
		}
		// check subscription(s) are open before publishing messages
		if (qm1) {
			while ((!subscription1.isOpen())) {
				try {
					Thread.sleep(1000);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		}
		if (qm2) {
			while ((!subscription2.isOpen())) {
				try {
					Thread.sleep(1000);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		}
		try {
			test.publishTTMessages(numberOfMessages);	
		} catch (Exception e) {
			e.printStackTrace();
		}
		int seconds = 0;
		if (qm1) {
			while (seconds < MessagingTest.waitInSeconds) {
				MQMessage message1 = new MQMessage();
				try {
					subscription1.get(message1);
					seconds = 0;
				} catch (MQException e) {
					if (e.getLocalizedMessage().contains("2033")) {
						try {
							Thread.sleep(1000);
						} catch (InterruptedException ie) {
							ie.printStackTrace();
						}
						seconds++;
						continue;
					}
					seconds++;
					e.printStackTrace();
				}
				System.out.println("message from sub 1: " + test.getPayload(message1));
				sub1Mess++;
			}
		}
		if (qm2) {
			seconds=0;
			while (seconds < MessagingTest.waitInSeconds) {
				MQMessage message2 = new MQMessage();	
				try {
					subscription2.get(message2);
					seconds = 0;
				} catch (MQException e) {
					if (e.getLocalizedMessage().contains("2033")) {
						try {
							Thread.sleep(1000);
						} catch (InterruptedException ie) {
							ie.printStackTrace();
						}
						seconds++;
						continue;
					}
					e.printStackTrace();
				}
				System.out.println("message from sub 2: " + test.getPayload(message2));
				sub2Mess++;
			}
		}
		if (qm1) {
			try {
				subscription1.close();
				targetQueueManager1.disconnect();
			} catch (MQException e) {
				e.printStackTrace();
			}	
		}
		if (qm2) {
			try {			
				subscription2.close();
				targetQueueManager2.disconnect();
			} catch (MQException e) {
				e.printStackTrace();
			}
		}		
		if (messageUtilities.getPayloads().size()==0) {
			test.logger.printString("Payloads empty");
		} else {
			test.logger.printString("Payloads size : " + messageUtilities.getPayloads().size());
		}
		if (messageUtilities.getExtraPayloads().size()==0) {
			test.logger.printString("Extra Payloads empty");
		} else {
			test.logger.printString("Extra Payloads size : " + messageUtilities.getExtraPayloads().size());

			for (String message: messageUtilities.getExtraPayloads()) {
				test.logger.printString("extra message " + message);
			}
		}
		test.logger.printString("sub1 messages " + sub1Mess);
		test.logger.printString("sub2 messages " + sub2Mess);

		if ((sub1Mess + sub2Mess == numberOfMessages) && (messageUtilities.getPayloads().size()==0) && (messageUtilities.getExtraPayloads().size()==0)) {
			test.logger.printString("all messages received, no duplicates");
		}
	}

	private String getPayload(MQMessage message) throws IOException {
		byte[] mqPayload = null;
		try {
			mqPayload = new byte[message.getDataLength()];
		} catch (IOException e1) {
			e1.printStackTrace();
			throw e1;
		}
		message.readFully(mqPayload);
		String mqPayloadString = null;
		mqPayloadString = new String (mqPayload);
		if (mqPayloadString!=null && mqPayloadString.length()!=0) {
			synchronized (messageUtilities.payloadsLock) {
				if (messageUtilities.getOriginalPayloads().contains(mqPayloadString)) {
					if (messageUtilities.getPayloads().remove(mqPayloadString)) {
						logger.printString("message removed: " + mqPayloadString);
					} else {
						logger.printString("message duplicated: " + mqPayloadString);
						messageUtilities.getExtraPayloads().add(mqPayloadString);
					}
				} else {
					logger.printString("corrupted message " + mqPayloadString);
					messageUtilities.getExtraPayloads().add(mqPayloadString);
				}
			}
		}
		return mqPayloadString;
	}

	private boolean publishTTMessages(int messageCount) {
		boolean success = true;
		logger.printString("starting publisher");
		MqttMessage message; 
		MqttTopic topic = null;
		MqttClient client = null;
		try {
			String host = "tcp://" + ConnectionDetails.ismServerHost + ":" + ConnectionDetails.ismServerPort;
			client = new MqttClient(host,"fanout",null);
			MqttConnectOptions mco = new MqttConnectOptions();
			boolean publisherConnected = false;
			while (!publisherConnected) {
				try {
					logger.printString("connecting to: " + host);
					client.connect(mco);
					publisherConnected= true;
				}
				catch (MqttException mqtte) {
					if (!mqtte.getLocalizedMessage().contains("2033")) {
						logger.printString(mqtte.getLocalizedMessage());
						logger.printString("MqttException occurred:");
						mqtte.printStackTrace();
						success = false;
					}
				}
			}
			topic = client.getTopic(topicName);
			Thread.sleep(1000);
			String publication = "";
			byte [] publicationBytes = publication.getBytes();
			message = new MqttMessage(publicationBytes);
			MqttDeliveryToken token;
			logger.printString("publishing to " + topicName);
			String identifier="";
			for (int count =0; count < messageCount ; count++) {
				identifier = "qmfanout"+count;
				publication =  MessagingTest.messageBody + "TOKEN" +identifier;

				publicationBytes = publication.getBytes();
				logger.printString("publicationBytes " + publicationBytes.length);				
				message = new MqttMessage(publicationBytes);
				message.setQos(qualityOfService);
				String messageToPrint="";
				if (message.toString().length() > 1000) {
					messageToPrint = "long message, ID " + identifier; 
				} else {messageToPrint = message.toString();}
				logger.printString("" +  ISMtoMQTest.timeNow()  +  " publication of mqtt\"" + messageToPrint
						+ "\" with QoS = " + message.getQos() + " to " + topic.getName());
				boolean published = false;
				token = topic.publish(message);
				while (!published) {
					try { 
						token.waitForCompletion(10000);  
						logger.printString("Delivery token \"" + token.hashCode()
								+ "\" has been received: " + token.isComplete());
						synchronized ( messageUtilities.payloadsLock) {
							messageUtilities.getOriginalPayloads().add(publication);
							messageUtilities.getPayloads().add(publication);
						}
						published = true;	  
					} 
					catch (MqttException mqtte) {
						mqtte.printStackTrace();
						//Thread.sleep(10000);
						throw mqtte;
					}
				}
			}							
			client.disconnect();
		} catch (Exception e) {
			logger.printString("*******************************************************************************************");
			e.printStackTrace();
			Class<? extends Exception> clazz = e.getClass();
			String className = clazz.toString();
			logger.printString("className is " + className);
			logger.printString("*******************************************************************************************");
			success=false;
		}		
		return success;
	}
}
