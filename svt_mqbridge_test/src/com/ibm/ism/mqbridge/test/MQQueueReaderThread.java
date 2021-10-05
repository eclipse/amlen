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
import java.io.PrintStream;
import com.ibm.mq.MQDestination;
import com.ibm.mq.MQException;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQQueue;

public class MQQueueReaderThread implements Runnable {

	String queueName = "";
	MQQueue reader;
	MessagingTest test;
	int extraMessages =0;
	private PrintStream log;
	private String testName;
	private LoggingUtilities logger;
	private int threadNumber;

	public MQQueueReaderThread(MQDestination subscriber, MessagingTest isMtoMQManyToOneTest, String testName, LoggingUtilities logger, int threadNumber) {
		this.test = isMtoMQManyToOneTest;
		this.reader = (MQQueue) subscriber;
		this.testName = testName;
		this.logger = logger;
		this.threadNumber = threadNumber;
	}
	public MQQueueReaderThread(String string) {
		// TODO Auto-generated constructor stub
	}
	public void run() {
		//	log = ISMtoMQTest.getLog();
		
//		try {
//			Thread.sleep(10000);
//		} catch (InterruptedException e1) {
//			// TODO Auto-generated catch block
//			e1.printStackTrace();
//		}
		
		try {
			queueName = reader.getName();
			//	MQQueueManager mqqm = new MQQueueManager("test1");
			//	System.out.println("qm is " + mqqm.getName());
			//	MQQueue mqq = mqqm.accessQueue("CHELSEA.RESULTS", CMQC.MQOO_INPUT_SHARED);
			MQMessage message = new MQMessage();
			//  mqq.get(message);
			//System.out.println("message is " + message.readLine());
			//mqq.close();

			//	MQQueue reader = mqqm.accessQueue("CHELSEA.RESULTS", CMQC.MQOO_INPUT_SHARED);

			//	MQTopic subscriber2 =
			//	      mqqm.accessTopic("/Sport/Football/Results/Chelsea",null,
			//	      CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION, CMQC.MQSO_CREATE);		

			logger.printString("connected to " + queueName);
			test.canStartPublishing[threadNumber]=true;
			//			HashMap<String,MQMessage> receivedMessages = new HashMap<String, MQMessage>();
			int messageCount =0;
			int failCount = 0;
			while (failCount < MessagingTest.waitInSeconds) { // messageCount < test.numberOfMessages
				try {
					message = new MQMessage();
					reader.get(message);
					byte[] mqPayload = new byte[message.getDataLength()];
					message.readFully(mqPayload);
					String mqPayloadString;
					if (ISMtoMQTest.jmsPub) {
						mqPayloadString = new String (mqPayload, "UTF8");
					} else {
						mqPayloadString = new String (mqPayload);
					}
					String identifier="";
					//					receivedMessages.put(mqPayloadString, message);
					String [] tokenisedPayloadString = mqPayloadString.split("TOKEN");
					if (!(tokenisedPayloadString.length ==2)) {
						logger.printString("tokenisedPayloadString has length " + tokenisedPayloadString.length);
						ISMtoMQTest.success = false;
						ISMtoMQTest.reason = ISMtoMQTest.reason + "tokenisedPayloadString does not have 2 elements\n";
					} else { 
						if  (tokenisedPayloadString[0].equalsIgnoreCase(ISMtoMQTest.messageBody)) {
							identifier = tokenisedPayloadString[1];
							synchronized (ISMtoMQTest.messageUtilities.payloadsLock) {
								if (!ISMtoMQTest.messageUtilities.getPayloads().remove(identifier)) {
									ISMtoMQTest.success = false;
									System.out.println("identifier " + identifier);
									//	System.out.println("ISMtoMQTest.originalPayloads " + ISMtoMQTest.messageUtilities.getOriginalPayloads());

									if (ISMtoMQTest.messageUtilities.getOriginalPayloads().contains(identifier)){
										//  if (!ISMtoMQTest.reason.contains("duplicate message received")) {
										ISMtoMQTest.reason = ISMtoMQTest.reason + "duplicate message received: " + identifier+"\n";// + " MQ per: " + ISMtoMQTest.messagePersistence +"\n";
										//  }
									} else {
										ISMtoMQTest.reason = ISMtoMQTest.reason + "unexpected message received: " + identifier +"\n";//+ " MQ per: " + ISMtoMQTest.messagePersistence +"\n";
										ISMtoMQTest.messageUtilities.getExtraPayloads().add(identifier);
									}
								}
							}
						} else {
							ISMtoMQTest.reason = ISMtoMQTest.reason + "body of message corrupted: " + tokenisedPayloadString[0]
							                                                                 								+ " should be " +ISMtoMQTest.messageBody+"\n";
						}
					}
					//					receivedMessages.put(mqPayloadString, message);
					if (logger.isVerboseLogging()) {
						logger.printString("" + ISMtoMQTest.timeNow() + " received message " + mqPayloadString + " from " + queueName);

					} else {
						logger.printString("" + ISMtoMQTest.timeNow() + " received message number " + messageCount + " from " + queueName);
					}
					//subscriber2.get(message);
					messageCount++;
					failCount =0;
				} catch (MQException e) {
					if (e.reasonCode == 2033 && e.completionCode ==2) {
						//logger.printString("failed to receive message from " + queueName);
						Thread.sleep(1000); // if MQJE001: Completion Code '2', Reason '2033' continue
						failCount++;
					}
					else {
						// different error - log and rethrow.
						logger.printString("MQException thrown: " + e.reasonCode + e.getErrorCode());  
						e.printStackTrace();

					}
				}
			}
			logger.printString("breaking out of loop");
			if (messageCount != test.getNumberOfMessages() * test.getNumberOfPublishers()) {
				if( test.getQualityOfService() ==2) {
					logger.printString("messages received does not match messages sent for qos 2 case");
					ISMtoMQTest.success = false;
				}
				if( test.getQualityOfService() ==1) {
					extraMessages = messageCount - test.getNumberOfMessages();
				}
			}
			//	reader.close();
			// subscriber2.close();
			//	mqqm.disconnect();
			//	String messageLine = message.readLine();
			//logger.printString("message is " + messageLine);
			//			List<MqttMessage> messages = ISMtoMQTest.getMessages();
			//			boolean success = true;
			//			for (int pubNo=0; pubNo <  test.getNumberOfPublishers() ; pubNo++) {
			//			for (int messNo=0; messNo <test.getNumberOfMessages() ; messNo++) {
			//				// take the first message published and find a match
			//	//			MqttMessage testMessage = messages.get(messNo);			
			//	//			byte[] mqttPayload = testMessage.getPayload();
			//	//			String mqttPayloadString =  new String (mqttPayload);
			//				mqttPayloadString = mqttPayloadString + test.getClientID() + pubNo;
			//				if (receivedMessages.containsKey(mqttPayloadString)) {
			//					//	logger.printString("matched message " + mqttPayloadString);
			//					//	logger.printString("success!");			
			//					receivedMessages.remove(mqttPayloadString);
			//				}	 else {
			//					if (ISMtoMQTest.verboseLogging) {
			//					logger.printString("failed to match message " + mqttPayloadString);
			//					} else {
			//						logger.printString("failed to match message. PubNo " +  pubNo  + " messNumber " + messNo);
			//					}
			//					success = false;
			//				}
			//			}
			//			}
			//			logger.printString("extraMessages = " + extraMessages);
			//			
			//			if (!success) {
			//				logger.printString("failed to match messages ");
			//				ISMtoMQTest.reason = ISMtoMQTest.reason + "failed to match messages in test" + testName + "\n";
			//				Set<String> unmatchedMessages = receivedMessages.keySet();
			//				if (ISMtoMQTest.verboseLogging) {
			//				for (String key : unmatchedMessages) {
			//					logger.printString("" + key);
			//				}
			//				}
			//				ISMtoMQTest.testResult = false;
			//
			//			} else {
			//				logger.printString("matched all queue messages");
			//			}



			//			synchronized (ISMtoMQTest.messageUtilities.getPayloads()) {
			//				if (ISMtoMQTest.messageUtilities.getPayloads().isEmpty()) {
			//					logger.printString("Finished - all messages received");
			//				} else {
			//					logger.printString("Finished, but the following remain:");
			//
			//					for (String payloadMessage :ISMtoMQTest.messageUtilities.getPayloads()) {
			//						logger.printString("message: " + payloadMessage);
			//					}				
			//					ISMtoMQTest.success = false;
			//					ISMtoMQTest.reason = ISMtoMQTest.reason + " some messages did not arrive\n";
			//				}
			//			}
			//			
			//			synchronized (ISMtoMQTest.messageUtilities.getExtraPayloads()) {
			//				if (ISMtoMQTest.messageUtilities.getExtraPayloads().isEmpty()) {
			//					logger.printString("Finished - no extra messages received");
			//				} else {
			//					logger.printString("Finished, but the following unexpected messages arrived:");
			//
			//					for (String payloadMessage :ISMtoMQTest.messageUtilities.getExtraPayloads()) {
			//						logger.printString("message: " + payloadMessage);
			//					}				
			//					ISMtoMQTest.success = false;
			//					ISMtoMQTest.reason = ISMtoMQTest.reason + " some extra messages arrived\n";
			//				}
			//			}


		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (MQException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

	}

}
