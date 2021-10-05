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
import com.ibm.mq.MQDestination;
import com.ibm.mq.MQException;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQTopic;

public class MQSubscriberThread implements Runnable {
	String topicString ="";
	MQTopic subscriber;
	int numberOfMessages;
	int numberOfPublishers;
	MessagingTest test;
//	private int publisherNumber = -1;
	private String testName;
//	private PrintStream log = System.out;
	LoggingUtilities logger;
	private int threadNumber;

	public MQSubscriberThread(MQTopic subscriber) {
		this.subscriber = subscriber;
	}

	public MQSubscriberThread(MQDestination subscriber2,
			MessagingTest test) {
		this.subscriber = (MQTopic) subscriber2;
		this.test = test;
		this.numberOfMessages = MessagingTest.getNumberOfMessages();
		this.numberOfPublishers = test.getNumberOfPublishers();
	}

	public MQSubscriberThread(MQDestination subscriber2,
			MessagingTest isMtoMQManyToOneTest, String testName, LoggingUtilities logger2) {
		this.subscriber = (MQTopic) subscriber2;
		this.test = isMtoMQManyToOneTest;
		this.numberOfMessages = MessagingTest.getNumberOfMessages();
		this.numberOfPublishers = isMtoMQManyToOneTest.getNumberOfPublishers();
		this.testName = testName;
		this.logger = logger2;
	}


	public MQSubscriberThread(MQDestination subscription,
			MessagingTest isMtoMQManyToOneTest, int publisherNumber, String testName) {
		this.subscriber = (MQTopic) subscription;
		this.test = isMtoMQManyToOneTest;
		this.numberOfMessages = MessagingTest.getNumberOfMessages();
		this.numberOfPublishers = 1;
//		this.publisherNumber  = publisherNumber;
		this.testName = testName;
	}

	public MQSubscriberThread(MQDestination subscription,
			MessagingTest isMtoMQManyToOneTest, int publisherNumber, String testName, int numberOfPublishers, LoggingUtilities logger) {
		this.subscriber = (MQTopic) subscription;
		this.test = isMtoMQManyToOneTest;
		this.numberOfMessages = MessagingTest.getNumberOfMessages();
		this.numberOfPublishers = numberOfPublishers;
//		this.publisherNumber  = publisherNumber;
		this.testName = testName;
		this.logger = logger;
	}

	public MQSubscriberThread(MQDestination subscription,
			MessagingTest MessagingTest, int threadNumber, String testName2,
			LoggingUtilities logger2) {
		this(subscription,MessagingTest, threadNumber, testName2);
		this.logger = logger2;	
		this.threadNumber = threadNumber;
	}

	public void run() {
		try {
			boolean jms = false;
			MQMessage message = new MQMessage();
			logger.printString("subscriber: " + subscriber);
			topicString = subscriber.getName();
			logger.printString("subscribed to " + topicString);
			int messageCount =0;
			logger.printString("total messages = " + numberOfMessages);
			logger.printString("total publishers = " + numberOfPublishers);
			int consecFails =0;
			int messagePersistence =-2;
			boolean qosMismatch = false;
			test.canStartPublishing[threadNumber]=true;
//			Thread.sleep(50000);			
			while (consecFails < MessagingTest.waitInSeconds ) {
				try {
					if (jms) { 

					} else {
						message = new MQMessage();
						subscriber.get(message);
						byte[] mqPayload = new byte[message.getDataLength()];
						message.readFully(mqPayload);
						String mqPayloadString;
						if (MessagingTest.jmsPub) {
							mqPayloadString = new String (mqPayload, "UTF8");
						} else {
							mqPayloadString = new String (mqPayload);
						}
						//mqPayloadString = new String (mqPayload, "UTF8");
						// mqPayloadString = new String (mqPayload);
						//	String mqPayloadString2 = new String (mqPayload, "UTF8");
						//	logger.printString("mqPayloadString " +mqPayloadString2);
						String identifier = MessagingTest.messageUtilities.checkPayload(mqPayloadString);
						
//						String identifier="";
//					//logger.printString("mqPayloadString " +mqPayloadString);
//						String [] tokenisedPayloadString = mqPayloadString.split("TOKEN");
//						if (!(tokenisedPayloadString.length ==2)) {
//							logger.printString("tokenisedPayloadString has length " + tokenisedPayloadString.length);
//							MessagingTest.success = false;
//							MessagingTest.reason = MessagingTest.reason + "tokenisedPayloadString does not have 2 elements\n";
//						} else {
//							// check body OK
//							if  (tokenisedPayloadString[0].equalsIgnoreCase(MessagingTest.messageBody)) {
//								identifier = tokenisedPayloadString[1];
//								synchronized (MessagingTest.messageUtilities.payloadsLock) {
//								if (!MessagingTest.messageUtilities.getPayloads().remove(identifier)) {
//									MessagingTest.success = false;
//										if (MessagingTest.messageUtilities.getOriginalPayloads().contains(identifier)){
//											//  if (!MessagingTest.reason.contains("duplicate message received")) {
//											MessagingTest.reason = MessagingTest.reason + "duplicate message received: " + identifier+"\n";// + " MQ per: " + MessagingTest.messagePersistence +"\n";
//											//  }
//										} else {
//											MessagingTest.reason = MessagingTest.reason + "unexpected message received: " + identifier +"\n";//+ " MQ per: " + MessagingTest.messagePersistence +"\n";
//											MessagingTest.messageUtilities.getExtraPayloads().add(identifier);
//										}
//									}
//								}
//							} else {
//								logger.printString("corrupted");
//								MessagingTest.success = false;
//								MessagingTest.reason = MessagingTest.reason + "body of message corrupted: " + tokenisedPayloadString[0]
//								+ " should be " +MessagingTest.messageBody+"\n";
//							}
//						}
						messagePersistence = message.persistence;
						System.out.println("message received " + identifier);
						if (logger.isVerboseLogging()) {
							logger.printString("" + MessagingTest.timeNow() + " received message with identifier " + identifier + " from " + topicString + " with persistence " + messagePersistence);
						}
						// message persistence OK if
						// 1. qos 0 -> 0
						// 2. qos 1 or 2 -> 1

						if (messagePersistence == 0 && test.getQualityOfService() == 0) {
							logger.printString("qos matches persistence (0 and 0)");
						} else if ((test.getQualityOfService() == 1 || test.getQualityOfService() == 2) && messagePersistence == 1) {
							logger.printString("qos matches persistence (1 / 2 and 1)");
						} else {
							logger.printString("qos does not match persistence: MQPer " +messagePersistence + " qos " + test.getQualityOfService());
							MessagingTest.success = false;
							qosMismatch = true;
						}
						//subscriber2.get(message);
						messageCount++;
						consecFails=0;
					}
				} catch (MQException e) {
					if (e.reasonCode == 2033 && e.completionCode ==2) {
						if (messageCount < numberOfMessages * numberOfPublishers) {
							logger.printString("failed to receive message from " + topicString + " consecFails = " + consecFails);
						} else {
							logger.printString("No message. messageCount " + messageCount);
						}
						Thread.sleep(1000); // if MQJE001: Completion Code '2', Reason '2033' continue 
						consecFails++;
					}
					else {
						// different error - log and rethrow.
						logger.printString("MQException thrown: " + e.reasonCode + e.getErrorCode());  
						e.printStackTrace();
						throw e;
					}
				}
			}
			if (qosMismatch) {
				MessagingTest.reason = MessagingTest.reason + "Test: " + testName + "qos does not match persistence: MQPer " +messagePersistence + " qos " + test.getQualityOfService() + "\n";
			}

		} catch (MQException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

	}


}
