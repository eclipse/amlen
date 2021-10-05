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

import com.ibm.micro.client.mqttv3.MqttClient;
import com.ibm.micro.client.mqttv3.MqttConnectOptions;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttSecurityException;

public class ISMSubscriberThread implements Runnable {
	private String topicName;
	private String clientID;
	private String testName;
	private LoggingUtilities ismLogger;
	private int threadNumber =0;
	int subscriptionQos;
	int expectedQos;
	String ismServerAddress = ConnectionDetails.ismServerHost;
	String ismServerPort = ConnectionDetails.ismServerPort;
	MqttClient client;
	MessagingTest test;
//	int delay =10;

	public ISMSubscriberThread(String topicName, String clientID, MessagingTest test, LoggingUtilities logger) {
		this.topicName = topicName;
		this.clientID = clientID;
		this.testName = test.testName;
		this.test  = test;
		this.ismLogger = logger;
		this.subscriptionQos = MessagingTest.currentQualityOfService;
	}

	public ISMSubscriberThread(String topicName, String clientID, MessagingTest test, LoggingUtilities logger, int threadNumber) { 
		this(topicName, clientID, test, logger);
		this.threadNumber = threadNumber;
	}

	public void run() {
		ismLogger.printString("starting subscriber thread");
		try {
			client = new MqttClient("tcp://" +
					ismServerAddress +
					":"  + ismServerPort , 
					clientID+threadNumber
					,null);	

		
			// MqttClient client = new MqttClient("tcp://localhost:16102", clientID+threadNumber,null);
			TTCallBack callback = new TTCallBack(clientID+threadNumber,testName, ismLogger);
			callback.setMessUtil(MessagingTest.messageUtilities);
			client.setCallback(callback);
			MqttConnectOptions conOptions = new MqttConnectOptions();
			conOptions.setCleanSession(false);
			client.connect(conOptions);
			expectedQos = MessagingTest.expectedQos[MessagingTest.messagePersistence][MessagingTest.currentQualityOfService];
			callback.setExpectedQos(expectedQos);			
			//messagePersistence == 1 ? 2 : 0;
			ismLogger.printString("Subscribing to topic \"" + topicName
					+ "\" for client instance \"" + client.getClientId()
					+ "\" using QoS " + subscriptionQos
					+ ". Clean session is " + false);			
			client.subscribe(topicName, subscriptionQos);
			// now drain the queue to ensure a clean start - if no new message inside 5 seconds assume queue is empty
			callback.setDrainingMessages(true);
			drainQueue(callback, 3);
			callback.setTimeLastCalled(System.currentTimeMillis());
			// tell publishers that they can start
//			if (MessagingTest.multipleSubscribers) {
				// set correct number to true
			test.canStartPublishing[threadNumber] = true;
//			} else {
//				MessagingTest.startPublishing = true;
//			}			
			callback.setDrainingMessages(false);			
			if (topicName.toUpperCase().contains("FANOUT")) {
				MessagingTest.waitInSeconds =15;
			}
			drainQueue(callback,MessagingTest.waitInSeconds);
			client.unsubscribe(topicName);
			client.disconnect();
			if (MessagingTest.numberOfSubscribers ==1) {
				
				synchronized (MessagingTest.messageUtilities.payloadsLock) {
					if (MessagingTest.messageUtilities.getPayloads().isEmpty()) {
						ismLogger.printString("Finished - all messages received");
					} else {
						ismLogger.printString("Finished, but the following remain:");
						MessagingTest.success = false;
						for (String message :MessagingTest.messageUtilities.getPayloads()) {
							ismLogger.printString("message: " + message);
							MessagingTest.reason = MessagingTest.reason + " message did not arrive: " + message + "\n";
						}				
					}
				}
			} else {
				if (callback.getReceivedMessages().size()< MessagingTest.numberOfMessages) {
					MessagingTest.reason = MessagingTest.reason + " missing messages: \n";
					for (String payload: MessagingTest.messageUtilities.getOriginalPayloads()) {
						if (!callback.getReceivedMessages().contains(payload)) {
							MessagingTest.reason = MessagingTest.reason + " message did not arrive: " + payload + "\n";
						}
					}
				} else if (callback.getReceivedMessages().size()> MessagingTest.numberOfMessages){
					MessagingTest.reason = MessagingTest.reason + " extra messages: \n";
				}
			}

		} catch (Exception e) {
			ismLogger.printString("Finished - no connection");
			e.printStackTrace();
			System.exit(1);
		}
	}

	private void drainQueue(TTCallBack callback, int delay) {	
		int messagesProcessed =0;
		while (callback.getLastCalled() < delay) {
			if (callback.isConnectionLost()) {
				// wait to allow server to be restarted
				try {
					Thread.sleep(20000);
				} catch (InterruptedException e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
				// now reconnect
				ismLogger.printString("Subscribing to topic \"" + topicName
						+ "\" for client instance \"" + client.getClientId()
						+ "\" using QoS " + subscriptionQos
						+ ". Clean session is " + false);			
				try {
					MqttConnectOptions conOptions = new MqttConnectOptions();
					conOptions.setCleanSession(false);
					client.connect(conOptions);
					client.subscribe(topicName, subscriptionQos);
					callback.setConnectionLost(false);
				} catch (MqttSecurityException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (MqttException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			} else 
			{
			messagesProcessed = callback.getMessagesProcessed();
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
			if (callback.isDrainingMessages()) {
				ismLogger.printString("messages cleared = " + callback.getMessagesCleared());
			} else {
				ismLogger.printString("messages processed = " + messagesProcessed);
			}
		}
		}
	}
}
