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

import java.util.Random;
import javax.jms.JMSException;
import com.ibm.ima.mqcon.utils.MqttConnection;
import com.ibm.ima.mqcon.utils.Trace;
import com.ibm.mq.MQException;

/**
 * base class to cover all cases where messages are sent from an MQ Topic to an IMA destination
 * specific tests will inherit from this
 */
public class MqTopicToImaDestination extends MqJavaConnectivityTest {

	protected String clientID;
	protected boolean cleanSession = false;
	protected int qualityOfService = 2;

	public MqTopicToImaDestination() {
		super();
	}	

	protected void sendMessages() {
		try {
			if (Trace.traceOn()){	
				System.out.println("Sending " + testName + " " + + getNumberOfMessages() + " times: " );
			}
			for (int count = 1; count <= getNumberOfMessages(); count++) {
				String publication = testName + "messNo" +count;
				mqConn.sendMessage(mqTopic,publication , mqper);
				addMessage(publication);
			}
			if (Trace.traceOn()){	
				System.out.println("all messages sent");
			}
		} catch (MQException e) {
			System.out.println("FAILED");
			e.printStackTrace();
		}
	}

	public static String createLargeMessage(int size, int extraBits) throws JMSException{
		//Create random message of specified size
		Random randomGenerator = new Random();
		StringBuilder messageBody = new StringBuilder();
		long messageSizeBytes = size * 1024 * 1024 + extraBits;
		for(int x = 0;x<messageSizeBytes;x++){
			int randomInt = randomGenerator.nextInt(10);
			messageBody.append(randomInt);
		}	
		return messageBody.toString();
	}

	protected void sendRetainedMessage() {
		try {
			if (Trace.traceOn()){	
				System.out.println("Sending " + testName + " " + + getNumberOfMessages() + " times: " );
			}
			String publication = testName + "messRetained";
			System.out.println(publication);
			mqConn.sendRetainedMessage(mqTopic,publication , mqper);
			addMessage(publication);
			if (Trace.traceOn()){	
				System.out.println("all messages sent");
			}
		} catch (MQException e) {
			System.out.println("FAILED");
			e.printStackTrace();
		}
	}

	protected MqttConnection createMqttConnection() {
		return new MqttConnection(imaServerHost, imaServerPort, clientID, cleanSession, qualityOfService );
	}

	protected void createTopic() throws MQException {
		createTopic(topicString);
	}

	protected void createTopic(String topicString) throws MQException {
		mqTopic = mqConn.accessTopic(topicString);
	}
}
