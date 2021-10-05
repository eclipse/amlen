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

import com.ibm.ima.mqcon.utils.MQConnection;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.mq.MQException;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQTopic;
import com.ibm.mq.headers.MQDataException;

public class ImaTopicToMqTopicTwoQmsTest extends ImaTopicToMqDestination {
	public ImaTopicToMqTopicTwoQmsTest(String clientID) {
		super();
		this.testName = "ImaTopicToMqTopicTwoQmsTest";
		this.clientID = "ImaTopicToMqTopic2Qm";
	}
	private static int noOfMessagesFromSub1 =0;
	private static int noOfMessagesFromSub2 =0;
	static ImaTopicToMqTopicTwoQmsTest test;
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		try {
			test = new ImaTopicToMqTopicTwoQmsTest("qmfanout");
			test.setNumberOfMessages(5001);
			test.setNumberOfPublishers(1);
			test.setNumberOfSubscribers(1);
			test.topicString = "ISMTopic/To/MQTopic/fanout";
			test.setSecondmqChannel("IMA.SC");
			test.setSecondmqHostName("9.20.230.213");
			test.setSecondQueueManagerName("q1");
			test.setSecondmqPort(1414);
			test.waitTime = 10000;
			if (args.length > 0) {
				test.parseCommandLineArguments(args);
			}			
			test.execute();
			// check all messages arrived, with no duplication	
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
		} catch (NumberFormatException nfe) {
			System.out.println("NumberFormatException - TEST FAILED");
			nfe.printStackTrace();
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
			if (test.isSuccess()==false) {
				System.exit(1);
			} else {
				StaticUtilities.printSuccess();
			}
		}
	}

	private void execute() throws MqttException, MQException, IOException, MQDataException  {	
		final boolean qm1 = true;
		final boolean qm2 = true;
		// subscribe to 2 QMs
		// publish messages to IMA
		// collect messages from both QMs	
		mqttcon = createMqttConnection();
		mqttcon.setPublisher();
		MQTopic mqTopic1;
		MQTopic mqTopic2;
		if (qm1) {
			createMQConnection();
			mqTopic1 = mqConn.createSubscription(topicString, false);		
		}
		if (qm2) {
			createSecondMQConnection();
			mqTopic2 = secondMqConn.createSubscription(topicString, false);
		}
		// check subscription(s) are open before publishing messages
		if (qm1) {
			while ((!mqTopic1.isOpen())) {
				StaticUtilities.sleep(1000);
			}
		}
		if (qm2) {
			while ((!mqTopic2.isOpen())) {
				StaticUtilities.sleep(1000);
			}
		}
		System.out.println("*** sending messages ***");
		sendMessages();	
		int seconds = 0;
		if (qm1) {
			while (seconds < test.waitTime/1000) {
				MQMessage message1 = new MQMessage();
				try {
					mqTopic1.get(message1);
					seconds = 0;
					checkImaMqttToMQJavaQos(message1.persistence);
					updateReceivedMessages(returnMqMessagePayloadAsString(message1));
				} catch (MQException e) {
					if (e.getLocalizedMessage().contains("2033")) {
						try {
							Thread.sleep(1000);
						} catch (InterruptedException ie) {
							ie.printStackTrace();
						}
						seconds++;
						continue;
					}
					seconds++;
					e.printStackTrace();
				}
				noOfMessagesFromSub1++;
			}
		}
		if (qm2) {
			seconds=0;
			while (seconds < test.waitTime/1000) {
				MQMessage message2 = new MQMessage();	
				try {
					mqTopic2.get(message2);
					seconds = 0;
					checkImaMqttToMQJavaQos(message2.persistence);
					updateReceivedMessages(returnMqMessagePayloadAsString(message2));
				} catch (MQException e) {
					if (e.getLocalizedMessage().contains("2033")) {
						try {
							Thread.sleep(1000);
						} catch (InterruptedException ie) {
							ie.printStackTrace();
						}
						seconds++;
						continue;
					}
					e.printStackTrace();
				}
				noOfMessagesFromSub2++;
			}
		}
		if (qm1) {
			try {
				mqTopic1.close();
				mqConn.disconnect();
			} catch (MQException e) {
				e.printStackTrace();
			}	
		}
		if (qm2) {
			try {			
				mqTopic2.close();
				secondMqConn.disconnect();
			} catch (MQException e) {
				e.printStackTrace();
			}
		}		
		System.out.println("sub1 messages " + noOfMessagesFromSub1);
		System.out.println("sub2 messages " + noOfMessagesFromSub2);
	}

	protected void createSecondMQConnection() throws MQException {
		this.secondMqConn = new MQConnection(secondQueueManagerName, secondmqHostName, secondmqPort, secondmqChannel);
	}
}

