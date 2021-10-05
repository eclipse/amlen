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

import java.io.IOException;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttSecurityException;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.mq.MQException;
import com.ibm.mq.MQGetMessageOptions;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQQueue;
import com.ibm.mq.constants.CMQC;
import com.ibm.mq.headers.MQDataException;

/**
 * This test sends a number of messages to a named IMA topic 
 * and retrieves them from a named MQ queue
 */
public class ImaTopicToMqQueueTestNoMessages extends ImaTopicToMqDestination {

	public ImaTopicToMqQueueTestNoMessages() {
		super();
		this.testName = "ImaTopicToMqQueueTest";
		this.clientID = "ImaTopicToMqQueueTest";
	}

	public static void main(String[] args) {
		boolean success = ImaTopicToMqQueueTestNoMessages.run(args);
		if (success==false) {
			StaticUtilities.printFail();
		} else {
			StaticUtilities.printSuccess();
		}	
	}

	public static boolean run(String[] args) {
		ImaTopicToMqQueueTestNoMessages test = null;	
		try {
			test = new ImaTopicToMqQueueTestNoMessages();
			test.qualityOfService = 2;
			test.setNumberOfMessages(1);
			test.topicString = "ISMTopic/To/MQQueue1";
			test.mqQueueName="ISMTopic.To.MQQueue";
			test.waitTime =20000;
			if (args.length > 0) {
				test.parseCommandLineArguments(args);
			}			
			test.setNumberOfPublishers(1);
			test.setNumberOfSubscribers(1);
			test.execute();
		} catch (MQException e) {
			System.out.println("MQ exception - TEST FAILED");
			e.printStackTrace();
			test.setSuccess(false);
		} catch (MqttException e) {
			System.out.println("MQTT exception - TEST FAILED");
			e.printStackTrace();
			test.setSuccess(false);
		} catch (IOException e) {
			System.out.println("IO exception - TEST FAILED");
			e.printStackTrace();
			test.setSuccess(false);
		} catch (MQDataException e) {
			System.out.println("MQDataException - TEST FAILED");
			e.printStackTrace();
			test.setSuccess(false);
		} catch (Exception e) {
			System.out.println("Exception - TEST FAILED");
			e.printStackTrace();
			test.setSuccess(false);
		} finally {
			try {
				if (test.mqttcon!=null) {
					test.mqttcon.destroyClient();
				}
			} catch (MqttSecurityException mse) {
				mse.printStackTrace();
				System.out.println("MQTT Security exception - TEST FAILED");
				test.setSuccess(false);
			} catch (MqttException me) {
				me.printStackTrace();
				System.out.println("MQTT exception - TEST FAILED");
				test.setSuccess(false);
			}
			try {
				if (test.mqConn !=null) {
					test.mqConn.disconnect();
				}
			} catch (MQException e) {
				System.out.println("MQ exception - TEST FAILED");
				test.setSuccess(false);
			}
		}
		return test.isSuccess();
	}

	private void execute() throws MQException, MqttException, IOException, MQDataException {
		setSuccess(true);
		mqttcon = createMqttConnection();
		mqttcon.setPublisher();
		System.out.println("sending");
		sendMessages();
		System.out.println("all sent");
		// now check they arrived
		createMQConnection();
		MQQueue mqQueue = mqConn.getInputQueueConnection(mqQueueName);
		MQGetMessageOptions messageOptions = new MQGetMessageOptions();
		messageOptions.options = CMQC.MQGMO_WAIT;
		messageOptions.waitInterval = 10000;
		MQMessage message = new MQMessage();
		try {
			while(true){
				mqQueue.get(message, messageOptions);
				// we got a message, therefore test failed.
				System.out.println("message " + returnMqMessagePayloadAsString(message));
				setSuccess(false);
			}
		} catch (MQException mqe ){
			// OK if 2033, no message, bad otherwise
			if (mqe.reasonCode==2033) {
				// expected
			} else {
				mqe.printStackTrace();
				setSuccess(false);
				throw mqe;
			}
		}
	}
}
