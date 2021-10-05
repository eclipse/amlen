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
import com.ibm.mq.MQTopic;
import com.ibm.mq.headers.MQDataException;

/**
 * This test sends a number of messages to named IMA topics (base name + /<publisher number>)
 * and retrieves them from an MQ topic of the same base name
 */
public class ImaTopicSubtreeToMqTopicTest extends ImaTopicToMqDestination {

	public ImaTopicSubtreeToMqTopicTest() {
		super();
		this.testName = "ImaTopicSubtreeToMqTopicTest";
		this.clientID = "ImaTpSt2MqTpTest";
	}

	public static void main(String[] args) {
		boolean success = ImaTopicSubtreeToMqTopicTest.run(args);
		if (success==false) {
			StaticUtilities.printFail();
		} else {
			StaticUtilities.printSuccess();
		}
	}	

	public static boolean run(String[] args) {
		ImaTopicSubtreeToMqTopicTest test = null;	
		boolean runningSuccess = true;
		for (int x=0; x<=2; x++){
			try {
				test = new ImaTopicSubtreeToMqTopicTest();
				test.qualityOfService =x;
				test.setNumberOfMessages(401);
				test.topicString = "ISMTopicSubtree/To/MQTopic";
				test.setNumberOfPublishers(10);
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
		MQTopic mqTopic = mqConn.createSubscription(topicString, false);
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
				mqTopic.get(message);
				consecFails = 0;
				checkImaMqttToMQJavaQos(message.persistence);
				updateReceivedMessages(returnMqMessagePayloadAsString(message));
			} catch (MQException mqe ){
				consecFails++;
				StaticUtilities.sleep(1000);
				System.out.println("consecFails " + consecFails);
			}	
		}
	}
}
