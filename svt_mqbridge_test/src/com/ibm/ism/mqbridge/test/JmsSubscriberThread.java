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

import java.util.ArrayList;
import java.util.List;

import javax.jms.BytesMessage;
import javax.jms.Connection;
import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.ObjectMessage;
import javax.jms.Session;
import javax.jms.StreamMessage;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.mq.jms.MQConnectionFactory;
import com.ibm.msg.client.wmq.WMQConstants;


public class JmsSubscriberThread implements MessageListener, Runnable{
	private Connection conn;
	private Session sess;
	private MessageConsumer consumer;
	public static Connection ismConnection = null; // for ism
	String topicName;
	int count;
	int persistence;
//	String queueManager = "dttest2";
//	String hostName = ConnectionDetails.mqHostAddress;
//	int  port =1414;
	String clientID = "sherring";
	private LoggingUtilities logger;
	private List<String> receivedMessages = new ArrayList<String>();
	private int messagesProcessed=0;
	long timeLastCalled = System.currentTimeMillis();
	private int threadNumber;
	private MessagingTest test;


	public List<String> getReceivedMessages() {
		return receivedMessages;
	}
	public void setReceivedMessages(List<String> receivedMessages) {
		this.receivedMessages = receivedMessages;
	}

	public JmsSubscriberThread(String topic, MessagingTest test,String clientID, LoggingUtilities ismLogger, int threadNumber) {
		this.topicName = topic;
		this.clientID = clientID;
//		this.queueManager = queueManager;
//		this.hostName = hostName;
//		this.port = port;
		this.test = test;
		this.logger = ismLogger;
		this.threadNumber = threadNumber;
	}

	public void run() {

		createSubscriber();
		test.canStartPublishing[threadNumber] =true;
		while (messagesProcessed < MessagingTest.getNumberOfMessages() && (timeLastCalled +5000) > System.currentTimeMillis()) {
			try {
				Thread.sleep(1000);
				logger.printString("messagesProcessed " + messagesProcessed);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
		closeSubscriber();
		synchronized (MessagingTest.messageUtilities.payloadsLock) {
		if (MessagingTest.numberOfSubscribers==1) {			
				if (MessagingTest.messageUtilities.getPayloads().isEmpty()) {
					logger.printString("Finished - all messages received");
				} else {
					logger.printString("Finished, but the following remain:");
					MessagingTest.success = false;
					for (String messagePayload :MessagingTest.messageUtilities.getPayloads()) {
						logger.printString("message: " + messagePayload);
						MessagingTest.reason = MessagingTest.reason + " message did not arrive: " + messagePayload + "\n";
					}				
				}			
		} else {
			if (getReceivedMessages().size()< MessagingTest.getNumberOfMessages()) {
				MessagingTest.reason = MessagingTest.reason + " missing messages: \n";
				for (String payload: MessagingTest.messageUtilities.getOriginalPayloads()) {
					if (!getReceivedMessages().contains(payload)) {
						MessagingTest.reason = MessagingTest.reason + " message did not arrive: " + payload + "\n";
					}
				}
			} else {
				MessagingTest.reason = MessagingTest.reason + " extra messages: \n";
			}
		}
	}
	}

	private void closeSubscriber() {
		try {
			conn.stop();
			consumer.close();
			//	sess.unsubscribe(clientID);
			sess.close();			
			conn.close();
			logger.printString("JMS Subscriber closed");
		} catch (JMSException e) {
			e.printStackTrace();
			if(e.getLinkedException() != null) {
				e.getLinkedException().printStackTrace();
			}
		}
	}

	private void createSubscriber() {
		MQConnectionFactory connFact = new MQConnectionFactory();
		try {
			//We are connecting to queue manager using client mode
			connFact.setQueueManager(ConnectionDetails.queueManager);
			connFact.setHostName(ConnectionDetails.mqHostAddress);
			connFact.setPort(ConnectionDetails.mqPort);
			connFact.setTransportType(WMQConstants.WMQ_CM_CLIENT);
			conn = connFact.createConnection();			
			conn.setClientID(clientID);
			sess = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);
			Topic myTopic = sess.createTopic(topicName);
			// consumer = sess.createDurableSubscriber(myTopic, "testSub");
			consumer = sess.createConsumer(myTopic);
			consumer.setMessageListener(this);
			logger.printString("JMS Subscriber started and subscribed to topic " + topicName);
			//Now we are ready to start receiving the messages. Hence start the connection
			conn.start();
		} catch (JMSException e) {
			e.printStackTrace();
			if(e.getLinkedException() != null) {
				e.getLinkedException().printStackTrace();
			}
			System.exit(1);
		}
	}

	public void onMessage(Message message) {
		String messageData = new String();
		if (message instanceof BytesMessage) {
			BytesMessage bytesMessage = (BytesMessage)message;
			try {
				long bodyLength = bytesMessage.getBodyLength();
				if (bodyLength > Integer.MAX_VALUE) {
					logger.printString("The body length of the message received is too large: " + bodyLength);
					return;
				}
				byte[] msgBytes = new byte[(int)bodyLength];
				while(bytesMessage.readBytes(msgBytes) != -1) {
					String tmpStr = new String(msgBytes);
					messageData += tmpStr;
				}
				logger.printString("The message received is " + messageData);
			} catch (JMSException e) {
				e.printStackTrace();
				if(e.getLinkedException() != null) {
					e.getLinkedException().printStackTrace();
				}
			}
		} else if (message instanceof TextMessage) {
			{
				TextMessage tm = (TextMessage) message;
				try {
					messageData = tm.getText();
					logger.printString("The message received is " + messageData);
					logger.printString("TextMessage " + message);
				} catch (JMSException e) {
					e.printStackTrace();
				}
			}
		} else if (message instanceof MapMessage) {
			{
				MapMessage mm = (MapMessage) message;
				logger.printString("map message " + mm);
			}
		} else if (message instanceof StreamMessage) {
			{
				StreamMessage sm = (StreamMessage) message;
				logger.printString("Stream Message " + sm);
			}
		} else if (message instanceof ObjectMessage) {
			{
				ObjectMessage om = (ObjectMessage) message;
				logger.printString("ObjectMessage " + om);
				try {
					messageData = om.getObject().toString();
				} catch (JMSException e) {
					e.printStackTrace();
				}
				logger.printString("The message received is " + messageData);
			}
		} else {
			logger.printString("message class is " + message.getClass());
		}

		// int messageQos = message.
		// check correct qos
		//		if (MessagingTest.messagePersistence == CMQC.MQPER_NOT_PERSISTENT && messageQos ==0) {
		//			logger.verboseLoggingPrintString("Message qos = 0, which is correct");
		//		} else if (MessagingTest.messagePersistence == CMQC.MQPER_PERSISTENT && messageQos ==2){
		//			logger.verboseLoggingPrintString("Message qos = 2, which is correct");
		//		} else {
		//			logger.printString("Message qos mismatch");
		//			MessagingTest.success = false;
		//			String misMatch = "Message qos = " + messageQos + " but MQ persistence is " + (MessagingTest.messagePersistence == 1 ? "persistent" : "non persistent");
		//			if (!MessagingTest.reason.contains(misMatch)) {
		//				MessagingTest.reason = MessagingTest.reason + misMatch +"\n";
		//			}
		//			logger.printString(misMatch);
		//		}
//		String mqPayloadString;
//		byte[] mqPayload = null;
//		if (MessagingTest.jmsPub) {
//			try {
//				mqPayloadString = new String (mqPayload, "UTF8");
//			} catch (UnsupportedEncodingException e) {
//				e.printStackTrace();
//			}
//		} else {
//			mqPayloadString = new String (mqPayload);
//		}

		incMessagesProcessed();
		timeLastCalled = System.currentTimeMillis();
		String[] messageSplit = messageData.split("TOKEN");
		String identifier = message.toString();
		if (messageSplit.length > 1) {
			identifier = messageSplit[1];
		}
		logger.printString("messagesProcessed is " + getMessagesProcessed());
		logger.printString("identifier is " + identifier);
		logger.printString("message body is " + messageSplit[0]);
		
		String messageArrived = null;
		try {
			messageArrived = "Message arrived: \"" + identifier +
			"\" on " + message.getJMSDestination().toString() + "\" for instance \"" +messageData + "\"";
		} catch (JMSException e) {
			e.printStackTrace();
		}
		logger.printString(messageArrived);
		if (messageSplit[0].equalsIgnoreCase(MessagingTest.messageBody)) {
			logger.verboseLoggingPrintString("message body is OK");
		} else {
			MessagingTest.reason = MessagingTest.reason + "body of message corrupted: " ;
			MessagingTest.reason = MessagingTest.reason + messageSplit[0] + " should be " + MessagingTest.messageBody + "\n";
		}
		synchronized (MessagingTest.messageUtilities.payloadsLock) {
			if (MessagingTest.numberOfSubscribers ==1) {
				if (!MessagingTest.messageUtilities.getPayloads().remove(identifier)) {
					MessagingTest.success = false;
					logger.printString("identifier " + identifier);
					logger.printString("MessagingTest.originalPayloads " + MessagingTest.messageUtilities.getOriginalPayloads());
					if (MessagingTest.messageUtilities.getOriginalPayloads().contains(identifier)){
						//  if (!MessagingTest.reason.contains("duplicate message received")) {
						MessagingTest.reason = MessagingTest.reason + "duplicate message received: " + identifier + " MQ per: " + MessagingTest.messagePersistence +"\n";
						//  }
					} else {
						MessagingTest.reason = MessagingTest.reason + "unexpected message received: " + identifier + " MQ per: " + MessagingTest.messagePersistence +"\n";
					}
				}	
			} else {
				if (!MessagingTest.messageUtilities.getPayloads().contains(identifier)) {
					MessagingTest.success = false;
					logger.printString("identifier " + identifier);
					logger.printString("MessagingTest.originalPayloads " + MessagingTest.messageUtilities.getOriginalPayloads());
					if (MessagingTest.messageUtilities.getOriginalPayloads().contains(identifier)){
						//  if (!MessagingTest.reason.contains("duplicate message received")) {
						MessagingTest.reason = MessagingTest.reason + "duplicate message received: " + identifier + " MQ per: " + MessagingTest.messagePersistence +"\n";
						//  }
					} else {
						MessagingTest.reason = MessagingTest.reason + "unexpected message received: " + identifier + " MQ per: " + MessagingTest.messagePersistence +"\n";
					}
				} else {
					// add message to list of those received?
					if (receivedMessages.contains(identifier)) {
						MessagingTest.reason = MessagingTest.reason + "duplicate message received: " + identifier + " MQ per: " + MessagingTest.messagePersistence +"\n";
					} else {
						receivedMessages.add(identifier);
					}
				}
			}	
		}
	}

	public int getMessagesProcessed() {
		return messagesProcessed;
	}
	public void setMessagesProcessed(int messagesProcessed) {
		this.messagesProcessed = messagesProcessed;
	}
	public void incMessagesProcessed() {
		messagesProcessed++;
	}
}
