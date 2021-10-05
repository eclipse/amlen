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
package com.ibm.ima.mqcon.utils;

import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttPersistenceException;
import org.eclipse.paho.client.mqttv3.MqttSecurityException;

public class MqttConnection {

	private MqttClient client;
	private boolean cleanSession;
	private int qualityOfService;
	private int imaServerPort;
	private String imaServerHost;
	private String clientID;
	

	public MqttConnection (String imaServerHost, int imaServerPort, String clientID, boolean cleanSession, int qualityOfService) {
		this.imaServerHost = imaServerHost;
		this.imaServerPort = imaServerPort;
		this.clientID = clientID;	
		this.cleanSession = cleanSession;
		this.qualityOfService = qualityOfService;
	}
	/**
	 * creates a client connection to MQTT, using ttcallback to process messages
	 * @param ttCallback
	 * @return
	 * @throws MqttException
	 */
	public MqttClient createClient(MqttCallback ttCallback) throws MqttException {
		try {
			String target = "tcp://" + imaServerHost +":"  + imaServerPort;
			client = new MqttClient(target , clientID,null);
			client.setCallback(ttCallback);
			MqttConnectOptions conOptions = new MqttConnectOptions();
			conOptions.setCleanSession(cleanSession);
//			System.out.println("creating conn for " + target);
			client.connect(conOptions);
			client.setCallback(ttCallback);
		} 
		catch (MqttException e) {
			System.out.println("unable to create connection");
			e.printStackTrace();
			throw e;
		}
		return client;		
	}


	/**
	 * creates a subscription to the topic specified using the connection's qos value
	 * @param topicString
	 * @throws MqttSecurityException
	 * @throws MqttException
	 */
	public void createSubscription(String topicString) throws MqttSecurityException, MqttException {
		if(Trace.traceOn()){
			System.out.println("Subscribing to topic \"" + topicString
					+ "\" for client instance \"" + client.getClientId()
					+ "\" using QoS " + qualityOfService 
					+ ". Clean session is " + cleanSession);	
		}
		client.subscribe(topicString, qualityOfService);
	}

	/**
	 * creates a subscription to the topic specified using the connection's qos value
	 * @param topicString
	 * @throws MqttSecurityException
	 * @throws MqttException
	 */
	public void createWildCardSubscription(String topicString) throws MqttSecurityException, MqttException {
		if(Trace.traceOn()){
			System.out.println("Subscribing to topic \"" + topicString
					+ "\" for client instance \"" + client.getClientId()
					+ "\" using QoS " + qualityOfService 
					+ ". Clean session is " + cleanSession);	
		}
		client.subscribe(topicString , qualityOfService);
	}
	
	/**
	 * creates a set of subscriptions using the topics specified in topicFilters
	 * using the qos values specified in qosArray
	 * @param topicFilters
	 * @param qosArray
	 * @throws MqttSecurityException
	 * @throws MqttException
	 */
	public void createMultipleSubscriptions(String []topicFilters, int [] qosArray) throws MqttSecurityException, MqttException {
		if(Trace.traceOn()){
			System.out.println("Subscribing to topic \"" + topicFilters
					+ "\" for client instance \"" + client.getClientId()
					+ "\" using QoS " + qosArray 
					+ ". Clean session is " + cleanSession);	
		}
		client.subscribe(topicFilters, qosArray);
	}
	/**
	 * unsubscribes from the specified topic
	 * @param topicString
	 * @throws MqttException
	 */
	public void unsubscribe (String topicString) throws MqttException {
		client.getDebug().dumpClientDebug();
		client.unsubscribe(topicString);
	}
	/**
	 * unsubscribes from the specified topics
	 * @param topicFilters
	 * @throws MqttException
	 */
	public void unsubscribeFromMultipleSubscriptions (String []topicFilters) throws MqttException {
		client.unsubscribe(topicFilters);
	}	


	/**
	 * disconnects the client
	 * @throws MqttException
	 */
	public void destroyClient () throws MqttException {
		client.disconnect();
	}
	/**
	 * sends a message (messageData) to the specified topic (topicString)
	 * @param topicString
	 * @param messageData
	 * @throws MqttPersistenceException
	 * @throws MqttException
	 */
	public  void sendMessage(String topicString, MqttMessage messageData) throws MqttPersistenceException, MqttException {
		client.publish(topicString, messageData);
	}
	/**
	 * 
	 * @return
	 */
	public MqttClient getClient() {
		return client;
	}

	public void setClient(MqttClient client) {
		this.client = client;
	}
	/**
	 * creates a new client connection without a callback
	 * @throws MqttException
	 */
	public void setPublisher() throws MqttException {
		client = new MqttClient("tcp://" +
				imaServerHost +
				":"  + imaServerPort , clientID,null);
		MqttConnectOptions mco = new MqttConnectOptions();
		boolean publisherConnected = false;
		while (!publisherConnected) {
//			System.out.println("connecting to: " + imaServerHost);
			client.connect(mco);
			publisherConnected= true;
		}
	}
	public boolean isCleanSession() {
		return cleanSession;
	}
	public void setCleanSession(boolean cleanSession) {
		this.cleanSession = cleanSession;
	}
	public int getQualityOfService() {
		return qualityOfService;
	}
	public void setQualityOfService(int qualityOfService) {
		this.qualityOfService = qualityOfService;
	}
	public int getImaServerPort() {
		return imaServerPort;
	}
	public void setImaServerPort(int imaServerPort) {
		this.imaServerPort = imaServerPort;
	}
	public String getImaServerHost() {
		return imaServerHost;
	}
	public void setImaServerHost(String imaServerHost) {
		this.imaServerHost = imaServerHost;
	}
	public String getClientID() {
		return clientID;
	}
	public void setClientID(String clientID) {
		this.clientID = clientID;
	}
}
