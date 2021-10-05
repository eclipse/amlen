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
package com.ibm.ima.mqcon.rules;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttPersistenceException;
import com.ibm.ima.mqcon.utils.MqttConnection;
import com.ibm.ima.mqcon.utils.Trace;
import com.ibm.mq.MQException;
import com.ibm.mq.constants.CMQC;

/**
 * base class to cover all cases where messages are sent from an IMA Topic to an MQ destination
 * specific tests will inherit from this
 */
public class ImaTopicToMqDestination extends MqJavaConnectivityTest {

	protected MqttClient client;
	protected String clientID = "ImaTopicToMqDestination";
	protected boolean cleanSession = false;
	protected int qualityOfService = 2; 

	public  ImaTopicToMqDestination() {
		super();
	}	

	protected void sendMessages() throws MqttPersistenceException, MqttException {	
		sendMessages(topicString, testName);
	}

	protected void sendMessages(String topicName, String testString) throws MqttPersistenceException, MqttException {	
		if (Trace.traceOn()){	
			System.out.println("Sending " + testString + " " + + getNumberOfMessages() + " times: " );
		}
		for (int count = 1; count <= getNumberOfMessages(); count++) {
			String publication = testString + "messNo" +count;
			MqttMessage mqttm = new MqttMessage(publication.getBytes());
			mqttm.setQos(qualityOfService);
			mqttcon.sendMessage(topicName, mqttm);
			addMessage(publication);
			if (Trace.traceOn()){
				System.out.println("message sent: " + publication + " qos " + mqttm.getQos());
			}
		}
		if (Trace.traceOn()){	
			System.out.println("all messages sent");
		}	
	}

	protected MqttConnection createMqttConnection() {
		this.mqttcon = new MqttConnection(imaServerHost, imaServerPort, clientID, cleanSession, qualityOfService );
		return mqttcon;
	}

	protected void createTopic() throws MQException {
		createTopic(topicString);
	}

	protected void createTopic(String topicString) throws MQException {
		mqTopic = mqConn.accessTopic(topicString);
	}

	protected void checkImaMqttToMQJavaQos (int messagePersistence) {
		if (messagePersistence == CMQC.MQPER_NOT_PERSISTENT && qualityOfService == 0) {
			// OK
		} else if ((qualityOfService == 1 || qualityOfService == 2) && messagePersistence == CMQC.MQPER_PERSISTENT) {
			// OK
		} else {
			System.out.println("qos does not match persistence: MQPer " +messagePersistence + " qos " + qualityOfService);
			setSuccess(false);
		}
	}
}
