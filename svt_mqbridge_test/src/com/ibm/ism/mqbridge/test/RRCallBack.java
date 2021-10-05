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

import java.util.HashSet;
import java.util.Set;
import com.ibm.micro.client.mqttv3.*;

public class RRCallBack implements MqttCallback {
	private String instanceData = "";
	private int messagesProcessed =0;
	private int messagesCleared =0;
	long timeLastCalled = System.currentTimeMillis();
	private int threadNumber =0;
	private Set<String> receivedMessages = new HashSet<String>();
	
	
public Set<String> getReceivedMessages() {
		return receivedMessages;
	}
	public void setReceivedMessages(Set<String> receivedMessages) {
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
	private boolean drainingMessages = false;

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
	}

	public RRCallBack(String instance, String testName, LoggingUtilities logger) {
		instanceData = instance;   
		this.logger = logger;
	//	log = MQtoISMTest.getLog();
	}
	
	public RRCallBack(String instance, String testName, LoggingUtilities logger, int threadNumber) { 
		this (instance, testName, logger) ;
		this.threadNumber = threadNumber;
	}
	

	public void messageArrived(MqttTopic topic, MqttMessage message) {
		try {
			if (!drainingMessages) {
			int messageQos = message.getQos();
			// check correct qos
			if  (messageQos == BasicRequestReplyTest.qos) {
				logger.printString("qos matches - " + messageQos);
			} else {
				logger.printString("qos mismatch - received qos is " + messageQos +
						" but original qos was " + BasicRequestReplyTest.qos);
			}

			incMessagesProcessed();
			timeLastCalled = System.currentTimeMillis();
			String[] messageSplit = message.toString().split("REPLY");
			String identifier = message.toString();
			if (messageSplit.length > 1) {
			identifier = messageSplit[1];
			}
			String messageArrived = "Message arrived: \"" + identifier +
			"\" on topic \"" + topic.toString() + "\" for instance \"" +instanceData + "\"";
			logger.printString(messageArrived);
			if (messageSplit[0].equalsIgnoreCase(BasicRequestReplyTest.messageBody)) {
				logger.printString("message body is OK");
			} else {
				logger.printString("body of message corrupted") ;
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

}

