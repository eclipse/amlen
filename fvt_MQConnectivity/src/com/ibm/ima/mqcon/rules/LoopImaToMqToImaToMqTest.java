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
import com.ibm.ima.mqcon.utils.MqttConnection;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.mq.MQException;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQTopic;
import com.ibm.mq.headers.MQDataException;

/**
 * This test sends a number of messages to a named IMA topic 
 * and retrieves them from an MQ topic of the same name
 */
public class LoopImaToMqToImaToMqTest extends ImaTopicToMqDestination {
	String endTopicString = "MQTopic/To/Loop3";
	String intermediateTopicString ="IMATopic/Loop2";
	String startTopicString ="IMATopic/LoopStart";
	String subscriptionString;
	MqttConnection secondMqttcon = null;
//	MqttConnection mqttcon = null;;
	public LoopImaToMqToImaToMqTest() {
		super();
		this.testName = "ImaTopicToMqTopicTest";
		this.clientID = "ImaTopicToMqTopicTest";
	}

	public static void main(String[] args) {
		boolean overallSuccess = true;		
		for (int testNumber=1; testNumber<=3; testNumber++) {
			LoopImaToMqToImaToMqTest test = null;		
			try {
				test = new LoopImaToMqToImaToMqTest();
				test.setNumberOfMessages(1);
				test.setNumberOfPublishers(1);
				test.setNumberOfSubscribers(1);
				test.waitTime = 20000;
				if (args.length > 0) { //TODO ensure args length is correct
					test.parseCommandLineArguments(args);
				}		
				switch (testNumber)	{
				case 1 :
					// A -> B -> C  - should work
					test.topicString = test.startTopicString;
					test.subscriptionString = test.intermediateTopicString;
					test.ima2imaexecute();
					if (!test.checkMessagesOK()) {
						System.out.println("C1: TEST FAILED - No Messages Arrived");	
						test.setSuccess(false);
					} else {
						System.out.println("Case 1 success");
					}
					break;
				case 2 :	
					// C -> D  - should work
					test = new LoopImaToMqToImaToMqTest();
					test.setNumberOfMessages(1);
					test.setNumberOfPublishers(1);
					test.setNumberOfSubscribers(1);
					test.waitTime = 20000;
					test.topicString = test.intermediateTopicString;
					test.subscriptionString = test.endTopicString;
					test.execute();
					if (!test.checkMessagesOK()) {
						System.out.println("2:TEST FAILED - No Messages Arrived");	
						test.setSuccess(false);
					} else {
						System.out.println("Case 2 success");
					}
					break;
				case 3 :
					// A -> B -> C -> D should fail
					test.topicString = test.startTopicString;
					test.subscriptionString = test.endTopicString;
					test.execute();
					if (test.getReceivedMessages() == null || test.getReceivedMessages().size()==0) {
						// no messages, which may be a good thing!
						System.out.println("Case 3 success");
					} else {
						System.out.println("TEST FAILED - Messages Arrived");	
						test.setSuccess(false);
					}
					break;
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
			} 
			finally {
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
				overallSuccess = test.isSuccess() && overallSuccess;

			}
		}
		if (overallSuccess) {
			StaticUtilities.printSuccess();
		} else {
			StaticUtilities.printFail();
		}
	}

	private void ima2imaexecute() throws MqttException {
		mqttcon = createMqttConnection();
		secondMqttcon = new MqttConnection(imaServerHost, imaServerPort, "intClient", cleanSession, qualityOfService );
		mqttcon.setPublisher();
		MqconMqttCallBack ttCallback = new MqconMqttCallBack(this);
		try {
			secondMqttcon.createClient(ttCallback);
			secondMqttcon.createSubscription(subscriptionString);
		} catch (MqttSecurityException mse) {
			mse.printStackTrace();
			System.out.println("FAILED");
			setSuccess(false);
		} catch (MqttException me) {
			me.printStackTrace();
			System.out.println("FAILED");
			setSuccess(false);
		}
		while (!mqttcon.getClient().isConnected())
		{
			StaticUtilities.sleep(1000);
		}
		sendMessages();		
		StaticUtilities.sleep(waitTime);
	}

	private void execute() throws MQException, MqttException, IOException, MQDataException {
		createMQConnection();
		MQTopic mqTopic = mqConn.createSubscription(endTopicString, false);
		mqttcon = createMqttConnection();
		mqttcon.setPublisher();
		sendMessages();
		// now check whether they arrive - if they do, its a fail
		int consecFails = 0;
		while (consecFails  < waitTime/1000 ) {
			MQMessage message = new MQMessage();		
			try {
				mqTopic.get(message);
				consecFails = 0;
				updateReceivedMessages(returnMqMessagePayloadAsString(message));
				int messagePersistence = message.persistence;
				if (messagePersistence == 0 && qualityOfService == 0) {
					// OK
				} else if ((qualityOfService == 1 || qualityOfService == 2) && messagePersistence == 1) {
					// OK
				} else {
					System.out.println("qos does not match persistence: MQPer " +messagePersistence + " qos " + qualityOfService);
					setSuccess(false);
				}
			} catch (MQException mqe ){
				consecFails++;
				StaticUtilities.sleep(1000);
				System.out.println("consecFails " + consecFails);
			}	
		}
	}

//	public void parseCommandLineArguments(String [] inputArgs) throws NumberFormatException {
//		super.parseCommandLineArguments(inputArgs);
//		for (int x=0;x < inputArgs.length; x++) {
//			if (inputArgs[x].equalsIgnoreCase("-topicString")) {
//				x++;
//				topicString = inputArgs[x];
//			}
//		}
//	}
}

