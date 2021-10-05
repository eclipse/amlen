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

import com.ibm.micro.client.mqttv3.MqttCallback;
import com.ibm.micro.client.mqttv3.MqttDeliveryToken;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttMessage;
import com.ibm.micro.client.mqttv3.MqttTopic;

public class TTCallBack implements MqttCallback {
	private String instanceData = "";
	private int messagesProcessed =0;
	private int messagesCleared =0;
	long timeLastCalled = System.currentTimeMillis();
//	private int threadNumber =0;
	private List<String> receivedMessages = new ArrayList<String>();
	private boolean connectionLost = false;
	private MessageUtilities messUtil;
	
	
public boolean isConnectionLost() {
		return connectionLost;
	}
	public void setConnectionLost(boolean connectionLost) {
		this.connectionLost = connectionLost;
	}
public List<String> getReceivedMessages() {
		return receivedMessages;
	}
	public void setReceivedMessages(List<String> receivedMessages) {
		this.receivedMessages = receivedMessages;
	}
public long getTimeLastCalled() {
		return timeLastCalled;
	}
	public void setTimeLastCalled(long timeLastCalled) {
		this.timeLastCalled = timeLastCalled;
	}

	//	private PrintStream log = System.out;
	private LoggingUtilities logger;
	private boolean drainingMessages;
	private int expectedQos;

	public boolean isDrainingMessages() {
	return drainingMessages;
}
public void setDrainingMessages(boolean drainingMessages) {
	this.drainingMessages = drainingMessages;
}
	public int getMessagesCleared() {
	return messagesCleared;
}
public void setMessagesCleared(int messagesCleared) {
	this.messagesCleared = messagesCleared;
}
public void incMessagesCleared() {
	messagesCleared++;
}

	public int getMessagesProcessed() {
		return messagesProcessed;
	}
	public void setMessagesProcessed(int messagesProcessed) {
		this.messagesProcessed = messagesProcessed;
	}
	public void incMessagesProcessed() {
		messagesProcessed++;
		timeLastCalled = System.currentTimeMillis();
	}

	public TTCallBack(String instance, String testName, LoggingUtilities logger) {
		instanceData = instance;   
		this.logger = logger;
	//	log = MessagingTest.getLog();
	//	this.expectedQos = MessagingTest.expectedQos[MessagingTest.messagePersistence][MessagingTest.qualityOfService];
		System.out.println("expectedQos " + expectedQos);
	}
	
//	public TTCallBack(String instance, String testName, LoggingUtilities logger, int threadNumber) { 
//		this (instance, testName, logger) ;
//	//	this.threadNumber = threadNumber;
//	}
	
	public void messageArrived(MqttTopic topic, MqttMessage message) {
		try {
			if (!drainingMessages) {
			int messageQos = message.getQos();
			// check correct qos
			if (messageQos == expectedQos) {
				logger.printString("Message qos = "+ messageQos +", which is correct");
			} else {
				logger.printString("Message qos mismatch");
				MessagingTest.success = false;
				String misMatch = "Message qos = " + messageQos + " but MQ persistence is " + (MessagingTest.messagePersistence);// == 1 ? "persistent" : "non persistent");
				misMatch = misMatch + " expectedQos " + expectedQos;
				if (!MessagingTest.reason.contains(misMatch)) {
					MessagingTest.reason = MessagingTest.reason + misMatch +"\n";
				}
				logger.printString(misMatch);
			}			
			incMessagesProcessed();			
			//String identifier=MessagingTest.messageUtilities.checkPayload(message.toString());
			
			String[] messageSplit = message.toString().split("TOKEN");
			String identifier = message.toString();
			if (messageSplit.length > 1) {
			identifier = messageSplit[1];
			}
			String messageArrived = "Message arrived: \"" + identifier +
			"\" on topic \"" + topic.toString() + "\" for instance \"" +instanceData + "\"";
			logger.printString(messageArrived);
			if (messageSplit[0].equalsIgnoreCase(MessagingTest.messageBody)) {
				logger.verboseLoggingPrintString("message body is OK");
			} else {
				MessagingTest.reason = MessagingTest.reason + "body of message corrupted" ;
			}
			
			synchronized (messUtil.payloadsLock) {
			if (MessagingTest.numberOfSubscribers ==1) {
			if (!messUtil.getPayloads().remove(identifier)) {
				MessagingTest.success = false;
				System.out.println("identifier " + identifier);
				System.out.println("MessagingTest.originalPayloads " + messUtil.getOriginalPayloads());
				if (messUtil.getOriginalPayloads().contains(identifier)){
					//  if (!MessagingTest.reason.contains("duplicate message received")) {
					MessagingTest.reason = MessagingTest.reason + "duplicate message received: " + identifier + " MQ per: " + MessagingTest.messagePersistence +"\n";
					//  }
				} else {
					MessagingTest.reason = MessagingTest.reason + "unexpected message received: " + identifier + " MQ per: " + MessagingTest.messagePersistence +"\n";
				}
			} else {
				System.out.println("message array size now: " + messUtil.getPayloads().size());
			}
			
			} else {
				if (!messUtil.getPayloads().contains(identifier)) {
					MessagingTest.success = false;
					System.out.println("identifier " + identifier);
					System.out.println("MessagingTest.originalPayloads " + messUtil.getOriginalPayloads());
					if (messUtil.getOriginalPayloads().contains(identifier)){
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
		} else {
			String messageArrived = "Clearing message: \"" + message.toString() +
			"\" on topic \"" + topic.toString() + "\" for instance \"" +instanceData + "\"";
			logger.verboseLoggingPrintString(messageArrived);
			timeLastCalled = System.currentTimeMillis();
			incMessagesCleared();
		}
		} catch (Exception e) {
			e.printStackTrace();
			System.exit(1);
		}
	}

	public void connectionLost(Throwable cause) {
		logger.printString("Connection lost on instance \"" + instanceData
				+ "\" with cause \"" + cause.getMessage() + "\" Reason code " 
				+ ((MqttException)cause).getReasonCode() + "\" Cause \"" 
				+ ((MqttException)cause).getCause() +  "\"");    
		cause.printStackTrace();
		connectionLost = true;
	}

	public void deliveryComplete(MqttDeliveryToken token) {
		try {
			logger.printString("Delivery token \"" + token.hashCode()
					+ "\" received by instance \"" + instanceData + "\"");
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public int getLastCalled() {
		int milliSecondsSinceLastMessage = (int) (System.currentTimeMillis() - timeLastCalled);
	//	logger.printString("time  = " + milliSecondsSinceLastMessage/1000);
		return milliSecondsSinceLastMessage/1000;
	}
	public MessageUtilities getMessUtil() {
		return messUtil;
	}
	public void setMessUtil(MessageUtilities messUtil) {
		this.messUtil = messUtil;
	}
	public int getExpectedQos() {
		return expectedQos;
	}
	public void setExpectedQos(int expectedQos) {
		this.expectedQos = expectedQos;
	}

}
