// Copyright (c) 2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
package com.ibm.ism.mqbridge.test;

import com.ibm.ism.mqbridge.test.MQPublisherThread.Target;
import com.ibm.mq.constants.CMQC;

public class MQtoISMTest extends MessagingTest{
	
public MQtoISMTest(String clientID) {
		super(clientID);
	}

	//	public static int messagePersistence;
	public static int fanIn =2;
	private static String baseMQTopic = "MQ/1";
	private static String baseISMTopic = "MQ/1";
//	public static boolean startPublishing =false;
//	public static boolean []canStartPublishing;

	static boolean isJMS;
	public static int qualityOfService;

	public static void main(String[] args) {
		//int persToTest[] = {CMQC.MQPER_NOT_PERSISTENT};
		MQtoISMTest mqISMTest  = new MQtoISMTest("MQtoISMTest");
		if (args.length>0) {
			mqISMTest.parseInput(args);
		}

//		boolean [] jms = {true};
		boolean [] jms = {false};
		int persToTest[]= {CMQC.MQPER_PERSISTENT};
		startingQualityOfService=2;
		sleepTimeout = 50000;
		MessagingTest.waitInSeconds = 50;
		
//		boolean [] jms = {false, true};
//		int persToTest[] =  {CMQC.MQPER_NOT_PERSISTENT, CMQC.MQPER_PERSISTENT};
		
		//isJMS = true;
		for (int x=startingQualityOfService; x <= finalQualityOfService; x++) { // subscription qos
			for (boolean testJMS : jms){ // publish using JMS
				isJMS = testJMS;
				for (int messagePersistence :persToTest) { // MQ Persistence					
					qualityOfService =x;
					MessagingTest.messagePersistence = messagePersistence;
					numberOfMessages = 10;
					mqISMTest.testTopicToSameTopic(messagePersistence);
					mqISMTest.testQueueToTopic(messagePersistence);
					mqISMTest.testTopicToTargetTopic(messagePersistence);
					mqISMTest.testWildCardTopicToTargetTopic(messagePersistence);
					mqISMTest.testWildCardTopicToWildCardTopic(messagePersistence);
					numberOfMessages = 10;
					mqISMTest.fanOutTopicToTopic(messagePersistence);
					mqISMTest.fanOutQueueToTopic(messagePersistence);
				}
			}
		}
		//		int qos =2;
		//		MQtoISMTest mqISMTest  = new MQtoISMTest();
		//		mqISMTest.ttTottTest(qos);

		mqISMTest.logger.printString("result is " + success);
		if (!success) {
			mqISMTest.logger.printString("reason is " + reason);
		}
	}  	

	private void fanOutQueueToTopic(int messagePersistence2) {
		numberOfPublishers=1;
		numberOfSubscribers =10;
		testName = "fanOutQueueToTopic";
		messageUtilities = new MessageUtilities(logger);
		clearCanPublishArray();
		Thread[] subscriberThreads = new Thread[numberOfSubscribers];
		String clientID =testName;
//		multipleSubscribers = true;
		for (int threadNumber =0; threadNumber< numberOfSubscribers; threadNumber++) {			
			logger.printString("messagePersistence " + messagePersistence);
			ISMSubscriberThread subscriber = new ISMSubscriberThread("FANOUT/QUEUE/TO/TOPIC/TEST", clientID, this, logger, threadNumber);
			subscriberThreads [threadNumber] = new Thread(subscriber);
			subscriberThreads [threadNumber].start();
		}
		//	MQPublisherThread mqpt1 = new MQPublisherThread(baseMQTopic + "/fanout", Target.MQTopic, clientID +"P1", messagePersistence, 0, "fanOut", logger);
		MQPublisherThread mqpt1 = new MQPublisherThread("FANOUT.QUEUE.TO.TOPIC.TEST", Target.MQQueue, clientID +"P1", messagePersistence, 0, this, isJMS);
		Thread p1 = new Thread(mqpt1);
		p1.start();	
		try {
			p1.join();			
		} catch (InterruptedException e) {
			logger.printString("interrupted during join");
			e.printStackTrace();
		}		
		for (int threadNumber =0; threadNumber< numberOfSubscribers; threadNumber++) {
			try {
				subscriberThreads [threadNumber].join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}		
		//	checkMessages();
//		multipleSubscribers = false;
		
	}

	private void ttTottTest(int qos) {
		//startPublishing=false;
		String clientID =testName;
		logger.printString("messagePersistence " + messagePersistence);

		TTPublisherThread ttThread = new TTPublisherThread(baseMQTopic, clientID, qos);
		ttThread.setHostName("tcp://localhost:1883");
		// tcp://localhost:1883
		// MQPublisherThread mqpt1 = new MQPublisherThread(baseMQTopic, Target.MQTTTopic, clientID +"P1", messagePersistence, 0, "testTopicToTargetTopic", logger);			 		 
		ISMSubscriberThread ist1 = 	new ISMSubscriberThread(baseISMTopic, clientID +"S1", this, logger,0);
		Thread p1 = new Thread(ttThread);
		Thread s1 = new Thread(ist1);
		s1.start();
		p1.start();	
		try {
			p1.join();
			s1.join();			
		} catch (InterruptedException e) {
			logger.printString("interrupted during join");
			e.printStackTrace();
		}
		//checkMessages();
	}

	private void testTopicToTargetTopic(int messagePersistence) {
		numberOfPublishers=1;
		numberOfSubscribers =1;
		messageUtilities = new MessageUtilities(logger);
		clearCanPublishArray();
		String clientID ="MQ2ISMTTTT";
		testName = "testTopicToTargetTopic";
		logger.printString("messagePersistence " + messagePersistence);
		testName = "testTopicToTargetTopic";
		MQPublisherThread mqpt1 = new MQPublisherThread(baseMQTopic + "/From", Target.MQTopic, clientID +"P1", messagePersistence, 0, this, isJMS);			 		 
		ISMSubscriberThread ist1 = 	new ISMSubscriberThread(baseISMTopic + "/To", clientID +"S1", this, logger,0);
		Thread p1 = new Thread(mqpt1);
		Thread s1 = new Thread(ist1);
		s1.start();
		p1.start();	
		try {
			p1.join();
			s1.join();			
		} catch (InterruptedException e) {
			logger.printString("interrupted during join");
			e.printStackTrace();
		}
		checkMessages();
	}

	private void testWildCardTopicToTargetTopic(int messagePersistence) {
		numberOfPublishers=fanIn;
		numberOfSubscribers =1;
		messageUtilities = new MessageUtilities(logger);
		clearCanPublishArray();
		String clientID ="MQ2ISMWCTTT";
		testName = "testWildCardTopicToTargetTopic";
		// create publisher / subscriber pairs
		//		if (numberOfSubscribers != numberOfPublishers) {
		//			logger.printString("subscribers and publishers do not match - test failed");
		//			success = false;
		//			return;
		//		}
//		String testName = "testWildCardTopicToTargetTopic";
		Thread[] publisherThreads = new Thread[numberOfPublishers];		
		for (int threadNumber =0; threadNumber< numberOfPublishers; threadNumber++) {
			logger.printString("messagePersistence " + messagePersistence);
			MQPublisherThread publisher = new MQPublisherThread(baseMQTopic + "2/" + threadNumber, Target.MQTopic, clientID +threadNumber, messagePersistence, threadNumber, this, isJMS);		 		 			
			publisherThreads [threadNumber] = new Thread(publisher);			
			publisherThreads [threadNumber].start();
		}		
		ISMSubscriberThread subscriber = new ISMSubscriberThread(baseISMTopic + "2/Target", clientID, this ,logger,0);
		Thread subscriberThread = new Thread(subscriber);
		subscriberThread.start();
		for (int threadNumber =0; threadNumber< numberOfPublishers; threadNumber++) {
			try {
				publisherThreads [threadNumber].join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}		
		try {
			subscriberThread.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}			
		checkMessages();		
	}

	private void fanOutTopicToTopic(int messagePersistence) {
		numberOfPublishers=1;
		numberOfSubscribers =10;
		messageUtilities = new MessageUtilities(logger);
		clearCanPublishArray();
		Thread[] subscriberThreads = new Thread[numberOfSubscribers];
		String clientID ="MQ2ISMFOTTT";
		testName = "fanOutTopicToTopic";
//		multipleSubscribers = true;
		for (int threadNumber =0; threadNumber< numberOfSubscribers; threadNumber++) {			
			logger.printString("messagePersistence " + messagePersistence);
			ISMSubscriberThread subscriber = new ISMSubscriberThread(baseISMTopic + "/fanout", clientID, this, logger, threadNumber);		 		 
			subscriberThreads [threadNumber] = new Thread(subscriber);
			subscriberThreads [threadNumber].start();
		}
		MQPublisherThread mqpt1 = new MQPublisherThread(baseMQTopic + "/fanout", Target.MQTopic, clientID +"P1", messagePersistence, 0, this, isJMS);
		Thread p1 = new Thread(mqpt1);
		p1.start();	
		try {
			p1.join();			
		} catch (InterruptedException e) {
			logger.printString("interrupted during join");
			e.printStackTrace();
		}		
		for (int threadNumber =0; threadNumber < numberOfSubscribers; threadNumber++) {
			try {
				subscriberThreads [threadNumber].join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}		
		//	checkMessages();
//		multipleSubscribers = false;
		numberOfSubscribers =1;
	}



	private void testWildCardTopicToWildCardTopic(int messagePersistence) {
		numberOfPublishers=fanIn;
		numberOfSubscribers =1;
		messageUtilities = new MessageUtilities(logger);
		clearCanPublishArray();
		// create publisher / subscriber pairs
		//		if (numberOfSubscribers != numberOfPublishers) {
		//			logger.printString("subscribers and publishers do not match - test failed");
		//			success = false;
		//			return;
		//		}
		//	Thread[] subscriberThreads = new Thread[numberOfSubscribers];
		Thread[] publisherThreads = new Thread[numberOfPublishers];
		//		multipleSubscribers = true;
		String clientID ="MQ2ISMWCTWT";
		testName = "testWildCardTopicToWildCardTopic";
		for (int threadNumber =0; threadNumber< numberOfPublishers; threadNumber++) {			
			logger.printString("messagePersistence " + messagePersistence);
			MQPublisherThread publisher = new MQPublisherThread(baseMQTopic + "1/" + threadNumber, Target.MQTopic, clientID +threadNumber, messagePersistence, threadNumber, this, isJMS);		 		 
			publisherThreads [threadNumber] = new Thread(publisher);
			publisherThreads [threadNumber].start();
		}	

		ISMSubscriberThread subscriber = new ISMSubscriberThread(baseISMTopic + "1/#", clientID, this, logger,0);
		Thread subscriberThread = new Thread(subscriber);
		subscriberThread.start();

		for (int threadNumber =0; threadNumber< numberOfPublishers; threadNumber++) {
			try {
				publisherThreads [threadNumber].join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}		

		try {
			subscriberThread.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}

		checkMessages();
	}

	private void testTopicToSameTopic (int messagePersistence) {
		numberOfPublishers=1;
		numberOfSubscribers =1;
		messageUtilities = new MessageUtilities(logger);
		clearCanPublishArray();
		String clientID ="MQ2ISMTST";
		testName = "testTopicToSameTopic";
		logger.printString("messagePersistence " + messagePersistence);
		MQPublisherThread mqpt1 = new MQPublisherThread(baseMQTopic, Target.MQTopic, clientID +"P1", messagePersistence, 0, this, isJMS);			 		 
		ISMSubscriberThread ist1 = 	new ISMSubscriberThread(baseISMTopic, clientID +"S1", this, logger);
		Thread p1 = new Thread(mqpt1);
		Thread s1 = new Thread(ist1);
		s1.start();
		p1.start();	
		try {
			p1.join();
			s1.join();			
		} catch (InterruptedException e) {
			logger.printString("interrupted during join");
			e.printStackTrace();
		}
		checkMessages();
	}

	private void testQueueToTopic (int messagePersistence) {
		numberOfPublishers=1;
		numberOfSubscribers =1;
		messageUtilities = new MessageUtilities(logger);
		clearCanPublishArray();
		String clientID ="MQ2ISMQTT";
		testName = "testQueueToTopic";
		MQPublisherThread mqpt1 = new MQPublisherThread("QUEUE.TO.TOPIC.TEST", Target.MQQueue, clientID +"P1", messagePersistence, 0, this, isJMS);	 		 
		ISMSubscriberThread ist1 = 	new ISMSubscriberThread("QUEUE/TO/TOPIC/TEST", clientID +"S1", this, logger,0);
		Thread p1 = new Thread(mqpt1);
		Thread s1 = new Thread(ist1);
		s1.start();
		p1.start();	
		try {
			p1.join();
			s1.join();			
		} catch (InterruptedException e) {
			logger.printString("interrupted during join");
			e.printStackTrace();
		}
		checkMessages();
	}

//	private void parseArgs(String[] args) {
//		// iterate through arguments and assign parameters
//		try {
//			for (int x = 0; x< args.length;x++) {
//				if (args[x].startsWith("-")){
//					char parameter = args[x].charAt(1);
//					switch (parameter) {
//					case 'm':
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
//					}
//				} else {
//					System.out.println("invalid argument");
//					System.exit(1);
//				}
//			}
//		} catch (Exception e) {
//			System.out.println("exception caught " + e.toString());
//			System.exit(1);
//		}
//	}
}
