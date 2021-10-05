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
import com.ibm.micro.client.mqttv3.MqttDeliveryToken;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttMessage;
import com.ibm.micro.client.mqttv3.MqttTopic;

public class TTPublisherThread implements Runnable {

	String topicString = "";
	String clientID ="";
	String threadName="";
	ISMtoMQTest test;
	int qualityOfService = 2;
//	private PrintStream out;
	private long sleepTimeout =10000;
	private String hostName= "tcp://localhost:16102";
	private LoggingUtilities logger;
	
	public String getHostName() {
		return hostName;
	}

	public void setHostName(String hostName) {
		this.hostName = hostName;
	}

	public TTPublisherThread(String topicString, String clientID) {
		this.topicString = topicString;
		this.clientID = clientID;
	}

	public TTPublisherThread(String topicName,
			ISMtoMQTest isMtoMQManyToOneTest, int threadNumber) {
		// TODO Auto-generated constructor stub
		this.topicString = topicName;
		this.test = isMtoMQManyToOneTest;
		this.clientID = isMtoMQManyToOneTest.getClientID();
		this.threadName = clientID + threadNumber;
		this.qualityOfService = isMtoMQManyToOneTest.getQualityOfService();
	}
	
	public TTPublisherThread(String topicName, String clientID, int qos) {
		// TODO Auto-generated constructor stub
		this.topicString = topicName;		
		this.clientID = clientID;
		this.qualityOfService =qos;
		this.threadName = "MQTTPublisher";
	}
	
	public void run() {
//		System.out = ISMtoMQTest.getSystem.out();
		System.out.println("starting publisher thread");
		try {

			System.out.println("threadName is " + threadName);
		//	String localhost = "tcp://localhost:16102";
			MqttClient client = new MqttClient(hostName,threadName,null);
			//	MqttClient client = new MqttClient(Example.TCPAddress,clientID);
			MqttTopic topic;
			MqttMessage message;  
			MqttConnectOptions mco = new MqttConnectOptions();
			boolean publisherConnected = false;
			while (!publisherConnected) {
			try {
				System.out.println("Trying to connect to hostName " + hostName);
			client.connect(mco);
			publisherConnected= true;
			}
			catch (MqttException mqtte) {
				if (!mqtte.getLocalizedMessage().contains("32103")) {
					System.out.println("MqttException occurred:");
					mqtte.printStackTrace();
					System.exit(1);
				}
			}
			}



//			List<String> sent = new ArrayList<String>();
//			sent = ISMtoMQTest.getSent();
//			String [] topics = new String[sent.size()];
//			int topicNumber = 0;
//			for (String topicString: sent) {			
//				topics[topicNumber] = topicString;
//				topicNumber++;
//			}
			
			String publication = "Hello World";

			byte [] publicationBytes = publication.getBytes();
			message = new MqttMessage(publicationBytes);
			//message.setQos(2);
			MqttDeliveryToken token;
			
			for (int count =0; count < MessagingTest.getNumberOfMessages() ; count++) {
				topic = client.getTopic(topicString + count);
				System.out.println("publishing to " + topic.getName());
				publication = ISMtoMQTest.messageBody +count+threadName;
				publicationBytes = publication.getBytes();
				message = new MqttMessage(publicationBytes);
				message.setQos(qualityOfService);

				if (logger.isVerboseLogging()) {
				System.out.println("publication of \"" + message.toString()
						+ "\" with QoS = " + message.getQos() + " to " + topic.getName());
				}

				boolean published = false;
				token = topic.publish(message);



				while (!published) {
					try { 

						token.waitForCompletion(sleepTimeout);
						if (logger.isVerboseLogging()) {
							System.out.println("Delivery token \"" + token.hashCode()
									+ "\" has been received: " + token.isComplete());
						}
						published = true;	
						MQtoISMTest.messageUtilities.getPayloads().add(publication);
						MQtoISMTest.messageUtilities.getOriginalPayloads().add(publication);
					} 
					catch (MqttException mqtte) {
						mqtte.printStackTrace();
						Thread.sleep(sleepTimeout);
					}
				}



			}
			client.disconnect();
		} catch (Exception e) {
			System.out.println("*******************************************************************************************");
			e.printStackTrace();
			Class<? extends Exception> clazz = e.getClass();
			String className = clazz.toString();
			System.out.println("className is " + className);
			System.out.println("*******************************************************************************************");
			System.exit(1);
		}		
	}
}

