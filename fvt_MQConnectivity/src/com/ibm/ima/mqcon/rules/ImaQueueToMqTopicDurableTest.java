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

import javax.jms.JMSException;

import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.mq.MQException;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQTopic;
import com.ibm.mq.constants.CMQC;
import com.ibm.mq.headers.MQDataException;

/**
 * This test sends a number of messages to a named IMA queue 
 * and retrieves them from a named MQ topic
 */
public class ImaQueueToMqTopicDurableTest extends ImaQueueToMqDestination {
	public ImaQueueToMqTopicDurableTest() {
		super();
		this.testName = "ImaQueueToMqTopicTest";
	}

	public static void main(String[] args) {
		boolean success = ImaQueueToMqTopicDurableTest.run(args);
		if (success==false) {
			StaticUtilities.printFail();
		} else {
			StaticUtilities.printSuccess();
		}
	}

	public static boolean run(String[] args) {	
		ImaQueueToMqTopicDurableTest test = null;		
		try {
			test = new ImaQueueToMqTopicDurableTest();
			test.setNumberOfMessages(5000);
			test.setNumberOfPublishers(1);
			test.setNumberOfSubscribers(1);
			test.waitTime =10000;
			test.mqTopicString = "ISMQueue/To/MQTopic";
			test.imaQueueName = "ISMQueue.To.MQTopic";
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
		} catch (IOException e) {
			System.out.println("IO exception - TEST FAILED");
			e.printStackTrace();
			test.setSuccess(false);
		} catch (JMSException e) {
			System.out.println("JMS exception - TEST FAILED");
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

		}
		return test.isSuccess();
	}

	private void execute() throws MQException, IOException, JMSException, MQDataException {
		createMQConnection();
		MQTopic mqTopic = mqConn.createDurableSubscription(mqTopicString, false, testName);
		mqTopic.close();
		System.out.println("sending");
		sendMessages();
		System.out.println("sent");
		// now check they arrived
		StaticUtilities.sleep(60000);
		//		System.out.println("300 seconds to go");
		//		StaticUtilities.sleep(300000);
		System.out.println("connecting to MQ");
		try {
			mqTopic = mqConn.createDurableSubscription(mqTopicString, false, testName);
		} catch (MQException mqee) {
			createMQConnection();
			mqTopic = mqConn.createDurableSubscription(mqTopicString, false, testName);
		}

		int consecFails = 0;
		while (consecFails  < waitTime/1000 ) {
			MQMessage message = new MQMessage();		
			try {
				mqTopic.get(message);
				consecFails = 0;
				updateReceivedMessages(returnMqMessagePayloadAsString(message));
			} catch (MQException mqe ){
				consecFails++;
				if (mqe.getErrorCode().contains("2009") || mqe.getReason()==2009) {					
					StaticUtilities.sleep(10000);
					try {
						createMQConnection();						
						mqTopic = mqConn.createDurableSubscription(mqTopicString, false, testName);
					} catch (MQException mqException) {

					}
				} else {
					StaticUtilities.sleep(1000);
				}
				System.out.println("consecFails " + consecFails);
			}	
		}
		mqTopic.getSubscriptionReference().setCloseOptions(CMQC.MQCO_REMOVE_SUB);
		mqTopic.close();		
		try {
			if (mqConn !=null) {
				mqConn.disconnect();
			}
		} catch (MQException e) {
			System.out.println("MQ exception - TEST FAILED");
			setSuccess(false);
		}
	}
}

