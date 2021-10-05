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

import com.ibm.mq.MQDestination;
import com.ibm.mq.MQException;
import com.ibm.mq.MQQueue;
import com.ibm.mq.MQTopic;
import com.ibm.mq.constants.CMQC;

public class ISMtoMQTest extends MessagingTest {

	private  MQSubscriberThread  subscriberThread;
	private  MQDestination subscriber;
//	private  String clientID;
//	private int numberOfPublishers;
	//	public  static boolean testResult = true;
//	public  List<String> sent;
	public  String topicName;
	public  String queueName;
	public  PublisherThread publisherThread;
	public  MQQueueReaderThread mqQueueReaderThread;
	MQQueue reader;
	private String topicToQueue;

	public static boolean subscribed = false;
//	private static boolean inputMessagesFromFile = false;
	private static boolean loggingToScreen = true;
//	static MessageUtilities messageUtilities = new MessageUtilities();
	public static String messagePersistence;
//	private String queueManager = ConnectionDetails.queueManager;
//	String hostAddress = ConnectionDetails.mqHostAddress;
//	private boolean mqClientConn = true;

//	String getClientID() {
//		return clientID;
//	}

	public ISMtoMQTest(String clientID, int qualityOfService) {
		super(clientID);
		this.queueName = "TESTQUEUE";
		this.topicName = "Test/Test1";
		this.topicToQueue = "Test/Queue";
		this.setNumberOfPublishers(1);
		this.setQualityOfService(qualityOfService);	
	}
	
//	
//	public ISMtoMQTest(int qualityOfService) {
//	//	this.clientID ="ISMtoMQTest";
////		this.sent = new ArrayList<String>();
//x				
//	}

	public static void main(String[] args) {
		numberOfMessages = 10;
		ISMtoMQTest testCase = null;
//		startingQualityOfService =0;
//		finalQualityOfService = 2;		
//		logger = new LoggingUtilities();
//		messageUtilities = new MessageUtilities();
		//	inputMessagesFromFile = true;
//		if (inputMessagesFromFile) {
//			readMessageBodyFromFile();
//		} else {
//			//	messageBody = MessageUtilities.generateUnicodeChars(0, 10000);
//			//	messageBody =  MessageUtilities.duplicateMessage(messageBody, 1);
//		}

//		try {
//			Thread.sleep(2000);
//		} catch (InterruptedException e1) {
//			e1.printStackTrace();
//		}
		//jmsPub = true;
		System.out.println("messageBody " + messageBody.length());
		for (int noOfTests =1; noOfTests <= 2; noOfTests++){
			// topic to topic tests
			for (int qos = startingQualityOfService; qos <= finalQualityOfService; qos++) {
				if (qos==1 && jmsPub) {
					qos=2;
				}
				testCase = new ISMtoMQTest("ISMtoMQTestTs", qos);			
				testCase.basicMQSetup();
				try {
					testCase.topicToTopicTest();
					if (jmsSub) {
						continue;
					}
					testCase.topicToDifferentTopicTest();
					testCase.topicToTopicWildCardToTargetTest();
					testCase.topicToTopicWildCardToWildCardTest();
//					testCase.ManyWildCardsTest();
				} catch (MQException e) {
					e.printStackTrace();
					success = false;
					reason = reason + "MQ Exception\n";
				}
				testCase.mqTearDown();
			}

			// topic to queue tests
			for (int qos = startingQualityOfService; qos <= finalQualityOfService; qos++) {
				if (qos==1 && jmsPub) {
					qos=2;
				}
				testCase = new ISMtoMQTest("ISMtoMQTestQs", qos);			
				testCase.basicMQSetup();
				try {
					testCase.topicToQueueTest();
					testCase.wildcardTopicToQueueTest();
				} catch (MQException e) {
					e.printStackTrace();
				}
				testCase.mqTearDown();
			} 	
			jmsPub =true;
			//jmsPub = false;
		}
		if (success== false) {
			if (!loggingToScreen) {
				System.out.println("test has failed:");
				System.out.println(reason);
			}
			testCase.logger.printString("test has failed:");
			testCase.logger.printString(reason);
		} else {
			if (!loggingToScreen) {
				System.out.println("test has passed");
			}
			testCase.logger.printString("test has passed");
		}
		//	      if (log != System.out)
		//	          log.close();

	}  

	private void ManyWildCardsTest() throws MQException {
		logger.printString("starting ManyWildCardsTest");
		messageUtilities = new MessageUtilities(logger);
		clearCanPublishArray();
		String testName = "ManyWildCardsTest";
		// map one publisher to one subscriber
		try {			
			//	topicName = "Test/Test";	
			//	String wildCardTopicName = "Test/Test/#";
			String wildCardTopicName = topicName+"/#";
			logger.printString("wildCardTopicName is " + wildCardTopicName);
			int options = CMQC.MQSO_CREATE | CMQC.MQSO_WILDCARD_TOPIC; //CMQC.MQSO_WILDCARD_CHAR
			int openAs = CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION;
			MQTopic subscription =
				targetQueueManager.accessTopic(wildCardTopicName, null,openAs, options);
			logger.printString("created subscription " + subscription.getName());
			MQSubscriberThread sub = new MQSubscriberThread(subscription, this,0, testName, getNumberOfPublishers(), logger);
			logger.printString("sub " + subscription.getName());
			Thread subscriberThread = new Thread(sub);
			subscriberThread.start();
			while (!subscription.isOpen()) {Thread.sleep(500);}
			//	TTPublisherThread pub = new TTPublisherThread(topicName+"/", this, 0);
			TTPublisherThread pub = new TTPublisherThread(topicName+"/Wildcard/", this, 0);
			Thread publisherThread = new Thread(pub);
			publisherThread.start();
			publisherThread.join();
			subscriberThread.join();
			subscription.close();
		}  catch (InterruptedException e) {
			e.printStackTrace();
		}
		checkMessages();
	}

	private void wildcardTopicToQueueTest() throws MQException {
		messageUtilities = new MessageUtilities(logger);
		testName = "wildcardTopicToQueueTest";
		numberOfSubscribers=1;
		clearCanPublishArray();
		try {
			logger.printString("accessing queueName " + queueName);
			subscriber = targetQueueManager.accessQueue(queueName+".WILDCARDTEST", CMQC.MQOO_INPUT_SHARED);
		} catch (MQException e1) {
			e1.printStackTrace();
		}			
		logger.printString("accessing reader " + subscriber);
		mqQueueReaderThread  = new MQQueueReaderThread(subscriber, this, testName, logger, 0);		
		Thread[] publisherThreads = new Thread[getNumberOfPublishers()];
		for (int threadNumber =0; threadNumber < getNumberOfPublishers() ; threadNumber++) {
			PublisherThread pub = new PublisherThread(topicToQueue+"/WildcardTopic/" + threadNumber , this, threadNumber, logger);
			publisherThreads[threadNumber] = new Thread(pub);
			publisherThreads[threadNumber].start();
		}

		logger.printString("sub " + subscriber.getName());		
		Thread s1 = new Thread(mqQueueReaderThread);		 
		s1.start();
		try {										
			for (int threadNumber =0; threadNumber < getNumberOfPublishers() ; threadNumber++) {
				publisherThreads[threadNumber].join();
			}
			s1.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		try {
			subscriber.close();
		} catch (MQException e) {
			e.printStackTrace();
		}	
		checkMessages();
		subscribed =false;
	}

	private void topicToTopicWildCardToWildCardTest() throws MQException {
		messageUtilities = new MessageUtilities(logger);
		testName = "topicToTopicWildCardToWildCardTest";
		numberOfSubscribers=1;
		clearCanPublishArray();
		// map one publisher to one subscriber
		numberOfSubscribers = getNumberOfPublishers();
		Thread[] subscriberThreads = new Thread[numberOfSubscribers];
		MQDestination[] subscriptions = new MQDestination[numberOfSubscribers];
		try {
			for (int threadNumber =0; threadNumber < numberOfSubscribers ; threadNumber++) {
				
				MQDestination subscription =
					targetQueueManager.accessTopic(topicName+"/Wildcard/"+ threadNumber,null,
							CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION, CMQC.MQSO_CREATE);
				
//				MQDestination subscription =
//					targetQueueManager.accessTopic(topicName+"/"+ threadNumber,null,
//							CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION, CMQC.MQSO_CREATE);
				subscriptions[threadNumber] = subscription;
				MQSubscriberThread sub = new MQSubscriberThread(subscription, this,threadNumber, testName, logger);
				logger.printString("sub " + subscription.getName());
				subscriberThreads[threadNumber] = new Thread(sub);
				subscriberThreads[threadNumber].start();
			}
		} catch (MQException e1) {
			e1.printStackTrace();
		}		
		Thread[] publisherThreads = new Thread[getNumberOfPublishers()];
		for (int threadNumber =0; threadNumber < getNumberOfPublishers() ; threadNumber++) {
			//PublisherThread pub = new PublisherThread(topicName+"/"+ threadNumber, this, threadNumber, logger);
			PublisherThread pub = new PublisherThread(topicName+"/Wildcard/"+ threadNumber, this, threadNumber, logger);
			publisherThreads[threadNumber] = new Thread(pub);
			publisherThreads[threadNumber].start();
		}
		try {											
			for (int threadNumber =0; threadNumber < getNumberOfPublishers() ; threadNumber++) {
				publisherThreads[threadNumber].join();
			}			
			for (int threadNumber =0; threadNumber < numberOfSubscribers ; threadNumber++) {
				subscriberThreads[threadNumber].join();
			}
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		try {
			for (int threadNumber =0; threadNumber < numberOfSubscribers ; threadNumber++) {
				subscriptions[threadNumber].close();
			}
		} catch (MQException e) {
			e.printStackTrace();
		}	
		checkMessages();
		subscribed =false;
	}

	// checks messages have gone from topicName/<threadNumber> to topicName/Target for multiple threads
	private void topicToTopicWildCardToTargetTest() throws MQException {
		messageUtilities = new MessageUtilities(logger);
		testName = "topicToTopicWildCardToTargetTest";
		numberOfSubscribers=1;
		clearCanPublishArray();
		try {
			subscriber =
				targetQueueManager.accessTopic(topicName+"/Target",null,
						CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION, CMQC.MQSO_CREATE);
		} catch (MQException e1) {
			e1.printStackTrace();
		}		
		subscriberThread  = new MQSubscriberThread(subscriber, this,0, testName, logger);			
		Thread[] publisherThreads = new Thread[getNumberOfPublishers()];
		for (int threadNumber =0; threadNumber < getNumberOfPublishers() ; threadNumber++) {
			PublisherThread pub = new PublisherThread(topicName+"/Topic/"+ threadNumber, this, threadNumber, logger);
			publisherThreads[threadNumber] = new Thread(pub);
			publisherThreads[threadNumber].start();
		}		
		logger.printString("sub " + subscriber.getName());		
		Thread s1 = new Thread(subscriberThread);
		s1.start();	
		try {											
			for (int threadNumber =0; threadNumber < getNumberOfPublishers() ; threadNumber++) {
				publisherThreads[threadNumber].join();
			}			
			s1.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		try {
			subscriber.close();
		} catch (MQException e) {
			e.printStackTrace();
		}	
		checkMessages();
		subscribed =false;
	}

	private void topicToTopicTest () throws MQException {	
		testName = "topicToTopicTest";
		messageUtilities = new MessageUtilities(logger);
		String testName = "topicToTopicTest";
		numberOfSubscribers=1;
		clearCanPublishArray();
		Thread s1 = null;
		if (jmsSub) {
			JmsSubscriberThread jmsSubThread= new JmsSubscriberThread(topicName, this, testName, logger,0);
			s1 = new Thread(jmsSubThread);
		} else {
			try {
				subscriber =
					targetQueueManager.accessTopic(topicName,null,
							CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION, CMQC.MQSO_CREATE);
			} catch (MQException e1) {
				e1.printStackTrace();
			}		
			subscriberThread  = new MQSubscriberThread(subscriber, this,0,testName, logger);	
			logger.printString("sub " + subscriber.getName());
			
			s1 = new Thread(subscriberThread);
			while (!subscriber.isOpen()) {
				try {
				Thread.sleep(100);
			} catch (InterruptedException e1) {
				e1.printStackTrace();
			}	
			}
			subscribed = true;
		}		
		s1.start();
		System.out.println("thread name is "+ s1.getName());
		Thread[] publisherThreads = new Thread[getNumberOfPublishers()];
		for (int threadNumber =0; threadNumber < getNumberOfPublishers() ; threadNumber++) {
			PublisherThread pub = new PublisherThread(topicName , this, threadNumber, logger);
			publisherThreads[threadNumber] = new Thread(pub);
			publisherThreads[threadNumber].start();
		}			
		try {											
			for (int threadNumber =0; threadNumber < getNumberOfPublishers() ; threadNumber++) {
				publisherThreads[threadNumber].join();
			}			
			s1.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		if (!jmsSub) {
			try {
				subscriber.close();
			} catch (MQException e) {
				e.printStackTrace();
			}
		}
		checkMessages();
		subscribed =false;
	}

	private void topicToQueueTest () throws MQException {	
		messageUtilities = new MessageUtilities(logger);
		testName = "topicToQueueTest";
		numberOfSubscribers=1;
		clearCanPublishArray();
		try {
			logger.printString("accessing queueName " + queueName);
			subscriber = targetQueueManager.accessQueue(queueName, CMQC.MQOO_INPUT_SHARED);
		} catch (MQException e1) {
			e1.printStackTrace();
		}			
		logger.printString("accessing reader " + subscriber);
		mqQueueReaderThread  = new MQQueueReaderThread(subscriber, this, testName, logger, 0);
		Thread[] publisherThreads = new Thread[getNumberOfPublishers()];
		for (int threadNumber =0; threadNumber < getNumberOfPublishers() ; threadNumber++) {
			PublisherThread pub = new PublisherThread(topicToQueue , this, threadNumber, logger);
			publisherThreads[threadNumber] = new Thread(pub);
			publisherThreads[threadNumber].start();
		}		
		logger.printString("sub " + subscriber.getName());	
		Thread s1 = new Thread(mqQueueReaderThread);	 
		s1.start();
		try {										
			for (int threadNumber =0; threadNumber < getNumberOfPublishers() ; threadNumber++) {
				publisherThreads[threadNumber].join();
			}
			s1.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		try {
			subscriber.close();
		} catch (MQException e) {
			e.printStackTrace();
		}
		checkMessages();
		subscribed =false;
	}

	private void topicToDifferentTopicTest () throws MQException {
		messageUtilities = new MessageUtilities(logger);
		testName = "topicToDiffTopicTest";
		numberOfSubscribers=1;
		clearCanPublishArray();
		try {
			subscriber =
				targetQueueManager.accessTopic(topicName+"/Redirect",null,
						CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION, CMQC.MQSO_CREATE);
		} catch (MQException e1) {
			e1.printStackTrace();
		}	
		subscriberThread  = new MQSubscriberThread(subscriber, this,0, testName, logger);			
		Thread[] publisherThreads = new Thread[getNumberOfPublishers()];
		for (int threadNumber =0; threadNumber < getNumberOfPublishers() ; threadNumber++) {
			PublisherThread pub = new PublisherThread(topicName+"/Original" , this, threadNumber, logger);
			publisherThreads[threadNumber] = new Thread(pub);
			publisherThreads[threadNumber].start();
		}		
		logger.printString("sub " + subscriber.getName());		
		Thread s1 = new Thread(subscriberThread);
		s1.start();	
		try {											
			for (int threadNumber =0; threadNumber < getNumberOfPublishers() ; threadNumber++) {
				publisherThreads[threadNumber].join();
			}			
			s1.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		try {
			subscriber.close();
		} catch (MQException e) {
			e.printStackTrace();
		}
		checkMessages();
		subscribed =false;
	}	
}



