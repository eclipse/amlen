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

import javax.jms.BytesMessage;
import javax.jms.Connection;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.Session;

import com.ibm.mq.MQDestination;
import com.ibm.mq.MQException;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQQueueManager;
import com.ibm.mq.constants.CMQC;
import com.ibm.mq.jms.MQTopicConnectionFactory;
import com.ibm.msg.client.wmq.WMQConstants;

public class MQPublisherThread implements Runnable{

	private String targetName = "";
	private String clientID ="";
	private MQMessage message;
	enum Target {MQTopic, MQQueue, MQTTTopic };
	Target pubTarget;
	//private boolean isTopic;
	private int persistence;
	private int threadNumber =0;
	private String testName;
	private LoggingUtilities logger;
//	static final String messageBody = MessageUtilities.duplicateMessage("1234567890-=[]'#,./\\!\"£$%^&*()_+{}:@~<>?|qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM`¬", 400);
	// static final String messageBody =  MessageUtilities.duplicateMessage(MessageUtilities.generateUnicodeChars(0, 10000), 1);
	// static final String messageBody ="test";
	// jms classes
	private Connection conn;
	private Session sess;
	private MessageProducer publisher;
	boolean isJMS;
	private boolean mqClientConn =true;
	MQDestination mqDest = null;
	MQQueueManager mqqm = null;
	MessagingTest test;

	public MQPublisherThread(String targetName, Target mqtarget ,String clientID, int persistence, int threadNumber, MessagingTest test, boolean isJMS) {
		this.targetName = targetName;
		this.clientID = clientID;
		//this.isTopic = isTopic;
		this.pubTarget = mqtarget;
		this.persistence = persistence;
		this.threadNumber = threadNumber;
		this.test = test;
		this.testName = test.testName;
		this.logger = test.logger;
		this.isJMS = isJMS;
	}

	public void run() {
		message = new MQMessage();
		mqDest = null;
		mqqm = null;
		logger.printString("starting publisher thread. JMS = " + isJMS);
		try {
			if (isJMS && pubTarget !=Target.MQQueue) {
				createPublisher();
			} else {
				logger.printString("clientID is " + clientID);
				if (mqClientConn) {
					ConnectionDetails.setUpMqClientConnectionDetails();					
				} 
				connectToMQ();
				logger.printString("qm is " + mqqm.getName());
			}

			logger.printString("publishing to " + targetName);



			//	message = ISMtoMQTest.getMessages().get(count);
			//	messageBody = MessageUtilities.generateUnicodeChars(0, 10000);

			// quick sleep to allow subscriber thread to attach

//			if (MessagingTest.multipleSubscribers) {
				
				while (!test.canStartPublishing()) {
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}			
//			} 
//			else {
//				while (!MessagingTest.startPublishing) {
//					try {
//						Thread.sleep(1000);
//					} catch (InterruptedException e) {
//						// TODO Auto-generated catch block
//						e.printStackTrace();
//					}
//				}
//			}
			String messageString;
			String identifier ="";
			byte [] byteArray;
			logger.printString("length is " +  MessagingTest.messageBody.length());
			for (int count =0; count < MessagingTest.numberOfMessages; count++) {
//				if (count%300==0) {
//					System.err.println("pull the plug!");
//					Thread.sleep(20000);
//				}
				message = new MQMessage();
				identifier = testName + count+"pub" + threadNumber+ "persistence"+persistence + "JMS" +isJMS;
				//	messageString = messageBody +count+"pub" + threadNumber;
				//	System.out.println("message added to list is " + messageString);
				synchronized (MessagingTest.messageUtilities.payloadsLock) {
					MessagingTest.messageUtilities.getPayloads().add(identifier);
					MessagingTest.messageUtilities.getOriginalPayloads().add(identifier);
				}
				messageString = MessagingTest.messageBody + "TOKEN" + identifier;
				byteArray  = messageString.getBytes();
				message.write(byteArray);
				//	if (!isTopic) {
				// message.persistence = CMQC.MQPER_PERSISTENT;
				message.persistence = persistence ;
				// CMQC.MQPER_NOT_PERSISTENT;
				//	}
				//	if (ISMtoMQManyToOneTest.verboseLogging) {
				logger.printString("publication of " + identifier
						+ " to topic " + targetName + " with persistence " + message.persistence + " message length " + messageString.length());
				//	}
				if (isJMS && pubTarget !=Target.MQQueue) {					
					BytesMessage bm = sess.createBytesMessage();
					bm.writeBytes(byteArray);
					//	TextMessage tm = sess.createTextMessage(messageString);
					publisher.send(bm);
				} else {
					boolean messagePut = false;
					while (!messagePut) {
						try { 
							
							mqDest.put(message);
							messagePut = true;
						} catch (MQException mqe) {
							if (mqe.getMessage().contains("2009")) {
								ConnectionDetails.setMqQueueManager(null);
								connectToMQ();
								continue;
							} else {
								break;
							}
						}
					}
				}
			}
			// now add an extra message
			// publisher.put(message);
			if (isJMS && pubTarget !=Target.MQQueue) {
				closePublisher();
			} else {
				disconnectFromMQ();
			}

		} catch (MQException mqe) {
			
		} catch (IOException ioe) {
			
		} catch (JMSException jmse) {
			
		}
//		catch (Exception e) {
//			e.printStackTrace();
//			System.exit(1);
//		}	

	}

	private void closePublisher() {
		try {
			publisher.close();
			sess.close();
			conn.close();
			System.out.println("JMS Publisher closed");
		} catch (JMSException e) {
			e.printStackTrace();
			if(e.getLinkedException() != null) {
				e.getLinkedException().printStackTrace();
			}
		}
	}

	private void createPublisher() throws MQException {
		MQTopicConnectionFactory connFact = new MQTopicConnectionFactory();
		try {
			//We are connecting to queue manager using client mode
			try {
				ConnectionDetails.setUpMqClientConnectionDetails();
				connFact.setQueueManager(ConnectionDetails.getMqQueueManager().getName());
			} catch (MQException e) {
				e.printStackTrace();
				throw e;
			}
			connFact.setHostName(ConnectionDetails.mqHostAddress);
			connFact.setPort(ConnectionDetails.mqPort);
			connFact.setTransportType(WMQConstants.WMQ_CM_CLIENT);
			connFact.setQueueManager(ConnectionDetails.queueManager);
			connFact.setChannel(ConnectionDetails.mqChannel);
			//connFact.
			conn = connFact.createConnection();
			sess = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);
			publisher = sess.createProducer(sess.createTopic(targetName));
			publisher.setDeliveryMode(MQtoISMTest.messagePersistence+1);
			//int x = DeliveryMode.PERSISTENT;
		} catch (JMSException e) {
			e.printStackTrace();
			if(e.getLinkedException() != null) {
				e.getLinkedException().printStackTrace();
			}
		}
	}

	private void connectToMQ()  {
		boolean connected = false;
		mqqm=null;
		mqDest=null;
		while (!connected) {		
			try {			
				while (mqqm == null ) {
				mqqm = ConnectionDetails.getMqQueueManager();
				}
				while (!mqqm.isOpen()) {
					System.out.println("qm not open");
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e1) {
						// TODO Auto-generated catch block
						e1.printStackTrace();
					}
				}
				if (pubTarget == Target.MQTopic) {
					mqDest =
						mqqm.accessTopic(targetName,null,
								CMQC.MQTOPIC_OPEN_AS_PUBLICATION, CMQC.MQOO_OUTPUT);
				} else if (pubTarget == Target.MQQueue) {
					System.out.println("accessing queue");
					if (mqDest !=null) {
					mqDest.close();
					}
					mqDest = mqqm.accessQueue(targetName, CMQC.MQOO_OUTPUT);
					
				} else if (pubTarget == Target.MQTTTopic) {
					// TODO publish to MQ TT
					//	mqDest = mqqm.accessQueue(targetName, CMQC.MQOO_OUTPUT);
				} 
				connected = true;
			} catch (MQException e) {
				// TODO Auto-generated catch block
				if (e.getMessage().contains("2009")) {
					logger.printString("connection broken - retry in 10 seconds");
					ConnectionDetails.setMqQueueManager(null);
					try {
						Thread.sleep(10000);
					} catch (InterruptedException e1) {
						// TODO Auto-generated catch block
						e1.printStackTrace();
					}
				}
			}
		}
	}
	
	private void disconnectFromMQ() {
		try {
			mqDest.close();
			mqqm.disconnect();
		} catch (MQException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
}
