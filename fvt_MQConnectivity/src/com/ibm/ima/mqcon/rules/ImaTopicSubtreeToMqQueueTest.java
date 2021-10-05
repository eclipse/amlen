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
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQQueue;
import com.ibm.mq.headers.MQDataException;

/**
 * This test sends a number of messages to named IMA topics (base name + /<publisher number>)
 * and retrieves them from a named MQ queue
 */
public class ImaTopicSubtreeToMqQueueTest extends ImaTopicToMqDestination {

	public ImaTopicSubtreeToMqQueueTest() {
		super();
		this.testName = "ImaTopicSubtreeToMqQueueTest";
		this.waitTime =15000;
	}

	public static void main(String[] args) {
		boolean success = ImaTopicSubtreeToMqQueueTest.run(args);
		if (success==false) {
			StaticUtilities.printFail();
		} else {
			StaticUtilities.printSuccess();
		}	
	}

	public static boolean run(String[] args) {
		ImaTopicSubtreeToMqQueueTest test = null;	
		boolean runningSuccess = true;
		for (int x=0; x<=2; x++){
			try {
				test = new ImaTopicSubtreeToMqQueueTest();
				test.qualityOfService = x;
				test.setNumberOfMessages(5010);
				test.topicString = "ISMTopicSubtree/To/MQQueue";
				test.mqQueueName="ISMTopicSubtree.To.MQQueue";
				test.setNumberOfPublishers(5);
				test.setNumberOfSubscribers(1);
				if (args.length > 0) {
					test.parseCommandLineArguments(args);
				}			
				test.execute();
				if (!test.checkMessagesOK()) {
					System.out.println("TEST FAILED - Message Mismatch");	
					test.setSuccess(false);
				}
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
			runningSuccess = runningSuccess && test.isSuccess(); 
		}
		return runningSuccess;
	}

	private void execute() throws MQException, MqttException, IOException, MQDataException {
		createMQConnection();
		MQQueue mqQueue = mqConn.getInputQueueConnection(mqQueueName);
		mqttcon = createMqttConnection();
		mqttcon.setPublisher();
		for (int x=1; x<= getNumberOfPublishers() ; x++) {
			sendMessages(topicString+"/"+ x, testName + x);
		}		
		// now check they arrived
		int consecFails = 0;
		while (consecFails  < waitTime/1000 ) {
			MQMessage message = new MQMessage();		
			try {
				mqQueue.get(message);
				consecFails = 0;
				checkImaMqttToMQJavaQos(message.persistence);
				updateReceivedMessages(returnMqMessagePayloadAsString(message));
			} catch (MQException mqe ){
				consecFails++;
				StaticUtilities.sleep(1000);
			}
		}
	}
}
