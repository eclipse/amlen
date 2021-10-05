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
import com.ibm.ima.jms.impl.ImaMessageConsumer;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.ima.mqcon.utils.JmsSession.CLIENT_TYPE;
import com.ibm.mq.MQException;
import com.ibm.mq.constants.CMQC;

/**
 * This test sends a number of messages to a named MQ queue and retrieves them from
 * an IMA queue of the same name
 */

public class MqQueueToImaQueue extends MqQueueToImaDestination {
	private JmsSession jmss;
	String secondImaServerHost = null;

	public MqQueueToImaQueue() {
		super();
		this.testName = "MqQueueToImaQueue";
		this.imaServerPort =16104;
	}

	public static void main(String[] args) {
		boolean success = MqQueueToImaQueue.run(args);
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
			MqQueueToImaQueue test = null;
			try {
				test = new MqQueueToImaQueue();
				test.setNumberOfMessages(250);
				test.setNumberOfPublishers(1);
				test.setNumberOfSubscribers(1);
				test.mqQueueName = "MQQueue.To.ISMQueue";
				test.imaQueueName = "MQQueue.To.ISMQueue";
				test.waitTime = 10000;
				test.mqper = x;
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
			} catch (JMSException e) {
				System.out.println("JMS exception - TEST FAILED");
				e.printStackTrace();
				test.setSuccess(false);
			} catch (Exception e) {
				System.out.println("Exception - TEST FAILED");
				e.printStackTrace();
				test.setSuccess(false);
			} finally {
				try {
					if (test.mqConn !=null) {
						test.mqConn.disconnect();
					}
				} catch (MQException e) {
					System.out.println("MQ exception on disconnect - TEST FAILED");
					test.setSuccess(false);
				}
				if (test.jmss!=null) {
					try {
						test.jmss.closeConnection();
					} catch (JMSException e) {
						System.out.println("JMS exception on close - TEST FAILED");
						test.setSuccess(false);
					}
				}
			}
			runningSuccess = runningSuccess && test.isSuccess();
		}
		return runningSuccess;
	}

	private void execute() throws MQException, JMSException {
		if (haAware)
		{	
			queueManagerName = "QM_DIET";
			mqHostName = "9.3.177.102";
			mqPort = 1434;
			mqChannel = "JAVA.CHANNEL";
			imaServerHost= "9.3.177.39";
			secondImaServerHost = "9.3.177.40";
			imaServerPort = 16104 ;
		}
		createMQConnection();
		sendMessages();
		boolean transacted = false;
		receiveMessages(transacted);
	}

	private void receiveMessages(boolean transacted) throws JMSException {
		try {
			// connect to IMA and get messages
			jmss = new JmsSession();
			Session sess;
			if (haAware) {
				sess = jmss.getHaSession(CLIENT_TYPE.IMA,  imaServerHost, secondImaServerHost, imaServerPort, "", transacted); // no QM name required for IMA
			} else {
				sess = jmss.getSession(CLIENT_TYPE.IMA,  imaServerHost,  imaServerPort, ""); // no QM name required for IMA
			}
			MessageConsumer consumer;
			Queue imaQueue = sess.createQueue( imaQueueName);
			consumer = sess.createConsumer(imaQueue);
			//Now we are ready to start receiving the messages. Start the connection
			jmss.startConnection();
			int messageCount = 1;
			while (true) {
				Message receivedMessage = null;
				messageCount ++;
				try {
					receivedMessage = consumer.receive(waitTime);
					System.out.println("Received message # "+messageCount);
				} catch (JMSException e) {
					e.printStackTrace();
					System.out.println("JMS Exception reading message, linked exception is "+e.getLinkedException());
					return;
				}
				String text = null;
				if (receivedMessage!=null) {
					if (receivedMessage instanceof  ImaBytesMessage) {				
						int TEXT_LENGTH = new Long(((BytesMessage)receivedMessage).getBodyLength()).intValue();
						byte[] textBytes = new byte[TEXT_LENGTH];
						((BytesMessage)receivedMessage).readBytes(textBytes, TEXT_LENGTH);
						text = new String(textBytes);
						if (transacted) {
							try {
								sess.commit();
								updateReceivedMessages(text);
							} catch (JMSException e) {
								e.printStackTrace();
								System.out.println("commit failed");
							}
						} else {
							updateReceivedMessages(text);
						}
						checkJmsPersistence(receivedMessage.getJMSDeliveryMode());
					} else {
						// unexpected message type
						System.out.println("Test FAILED. Message is "  + receivedMessage.getClass());
						if (transacted) {
							try {
								sess.commit();
								receivedMessages.add("receivedMessage " + receivedMessage.getClass());
							} catch (JMSException e) {
								System.out.println("commit failed");
							}
						}				
						setSuccess(false);
					}			
				} else {	
					if (haAware && receivedMessages.size() < numberOfMessages ){
						try {
							System.out.println("receivedMessage under HA is null");
							((ImaMessageConsumer)consumer).getMessageListener();
						} catch (JMSException e) {
							e.printStackTrace();
							this.receiveMessages(transacted);
						} catch (IllegalStateException e) {
							e.printStackTrace();
							this.receiveMessages(transacted);
						}
					} else {
						// no message within timeout - 
						break;
					}
				}
			}
		}catch (Exception e) {
			e.printStackTrace();
			if (haAware) {
				this.receiveMessages(transacted);
			}
		}
	}
}


