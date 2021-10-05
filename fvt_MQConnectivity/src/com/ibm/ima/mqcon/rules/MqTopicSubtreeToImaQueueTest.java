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

import javax.jms.BytesMessage;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Queue;
import javax.jms.Session;

import com.ibm.ima.jms.impl.ImaBytesMessage;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.ima.mqcon.utils.JmsSession.CLIENT_TYPE;
import com.ibm.mq.MQException;
import com.ibm.mq.constants.CMQC;


/**
 * This test sends a number of messages to named MQ topics (base name + /<publisher number>)
 * and retrieves them from a named IMA queue
 */
public class MqTopicSubtreeToImaQueueTest extends MqTopicToImaDestination {
	private JmsSession jmss;
	public MqTopicSubtreeToImaQueueTest() {
		super();
		this.testName = "MqTopicSubtreeToImaQueueTest";
		this.imaServerPort =16104;
		this.waitTime =10000;
		this.clientID = "MqTSToIQTest";
	}
	// create 1 reader for IMA queue MQTSubtreeToIMAQueue
	// create n publishers on MQ topics MQTSubtreeToIMAQueue/1 and MQTSubtreeToIMAQueue/2 etc
	public static void main(String[] args) {
		boolean success = MqTopicSubtreeToImaQueueTest.run(args);
		if (success==false) {
			StaticUtilities.printFail();
		} else {
			StaticUtilities.printSuccess();
		}
	}


	public static boolean run(String[] args) {
		boolean runningSuccess = true;
		int [] mqPersistences = {CMQC.MQPER_NOT_PERSISTENT,CMQC.MQPER_PERSISTENT};
		for (int x : mqPersistences) {
			MqTopicSubtreeToImaQueueTest test = null;
			try {
				test = new MqTopicSubtreeToImaQueueTest();
				test.setNumberOfMessages(1);
				test.topicString = "MQTSubtreeToISMQueue";
				test.imaQueueName = "MQTSubtree.To.ISMQueue";
				test.setNumberOfPublishers(10);
				test.setNumberOfSubscribers(1);		
				test.mqper = x;
				if (args.length > 0) {
					test.parseCommandLineArguments(args);
				}		
				test.waitTime = 10000;
				test.execute();
				if (!test.checkMessagesOK()) {
					System.out.println("TEST FAILED - Message Mismatch");	
					test.setSuccess(false);
				}
			} catch (MQException e) {
				System.out.println("MQ exception - TEST FAILED");
				e.printStackTrace();
				test.setSuccess(false);
			} catch (JMSException e) {
				System.out.println("IMA JMS exception - TEST FAILED");
				e.printStackTrace();
				test.setSuccess(false);
			} catch (Exception e) {
				System.out.println("Exception - TEST FAILED");
				e.printStackTrace();
				test.setSuccess(false);
			} finally {
				try {
					if (test.jmss!=null) {
						test.jmss.closeConnection();
					}
				} catch (JMSException e) {
					System.out.println("IMA JMS exception - TEST FAILED");
					e.printStackTrace();
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

	private void execute() throws MQException, JMSException {
		createMQConnection();
		// IMA JMS conn
		// now go and get the messages
		jmss = new JmsSession();
		Session sess = jmss.getSession(CLIENT_TYPE.IMA,  imaServerHost,  imaServerPort, ""); // no QM name required for IMA
		MessageConsumer consumer;
		Queue imaQueue = sess.createQueue( imaQueueName);
		consumer = sess.createConsumer(imaQueue);
		//Now we are ready to start receiving the messages. Start the connection
		jmss.startConnection();
		String tn = testName;
		for (int x=1; x<= getNumberOfPublishers() ; x++) {
			testName = tn + x;
			createTopic(topicString+"/"+ x);
			sendMessages();
		}
		testName = tn;
		int messageCount = 1;
		while (true) {
			Message receivedMessage = null;
			try {
				receivedMessage = consumer.receive(waitTime);
				System.out.println("Received message # "+messageCount);
			} catch (JMSException e) {
				e.printStackTrace();
				System.out.println("JMS Exception reading message, linked exception is "+e.getLinkedException());
				return;
			}
			String text;
			if (receivedMessage!=null) {
				if (receivedMessage instanceof  ImaBytesMessage) {				
					int TEXT_LENGTH = new Long(((BytesMessage)receivedMessage).getBodyLength()).intValue();
					byte[] textBytes = new byte[TEXT_LENGTH];
					((BytesMessage)receivedMessage).readBytes(textBytes, TEXT_LENGTH);
					text = new String(textBytes);
					if (receivedMessages.contains(text)) {
						// got a dupe
						duplicateMessages.add(text);
					} else {
						receivedMessages.add(text);
					}
				} else {
					// unexpected message type
					System.out.println("Test FAILED. Message is "  + receivedMessage.getClass());
					receivedMessages.add("receivedMessage " + receivedMessage.getClass());
					setSuccess(false);
				}
				checkJmsPersistence(receivedMessage.getJMSDeliveryMode());
			} else {
				// no message within timeout - 
				break;
			}
			messageCount ++;

		}
		// wait for a while
		StaticUtilities.sleep(waitTime);	
	}
}
