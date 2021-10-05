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
package com.ibm.ism.mqbridge.test;

import javax.jms.BytesMessage;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.MessageProducer;
import javax.jms.ObjectMessage;
import javax.jms.Session;
import javax.jms.StreamMessage;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

public class JMSRequestReplyISM2MQ2ISM extends MessagingTest implements MessageListener{

	public JMSRequestReplyISM2MQ2ISM(String clientID) {
		super(clientID);
	}

	private Connection mqConn;
	private Connection ismConnection;
//	private String clientID = "JMSRRISM2MQ2ISM";
	private Session mqSession;
	private static String ismStartTopic = "Test/Test1" ;
	private static String ismEndTopic = "MQ/1" ;
	private MessageConsumer mqConsumer;
	private String clientIDPub = "JMSRRISM2MQ2ISMPub";
	private TestExceptionListener exceptionListener;
	private Session ismSession;
	private Topic startTopic;
	private Topic endTopic;
	private MessageProducer producer;
	private int delivMode = DeliveryMode.PERSISTENT;;
	private String publication = "Test Message";
	public static boolean subStarted = false;
	private MessageConsumer consumer;
	public static boolean messageReceived = false;
	TextMessage tmsg;
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		JMSRequestReplyISM2MQ2ISM test = new JMSRequestReplyISM2MQ2ISM("JMSRRISM2MQ2ISMPub");
		// create subscriber for MQ
		// listener on MQ picks up message and puts it back on replytoQ
		// create subscriber for ISM
		Thread mqSubThread = null;
		JMSMessageBouncer jmsSubThread= new JMSMessageBouncer(ismStartTopic, test.logger);
		mqSubThread = new Thread(jmsSubThread);
		mqSubThread.start();
		test.setupPublisherJMS();
		// only publish when subscriber has started
		while (!subStarted) {
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		// test sends single message to ISM.
		test.sendJMSMessage("1");		
		try {
			mqSubThread.join();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		// listener on ISM picks up from new Q and checks message properties
		while (!messageReceived) {
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}		
		test.tearDownPublisherJMS();
		
	}	
	
	private void setupPublisherJMS () {
		System.out.println("publishing with clientID " + clientIDPub);      
		try {
			ConnectionFactory connectionFactory = ImaJmsFactory.createConnectionFactory();
			((ImaProperties) connectionFactory).put("Server", ConnectionDetails.ismServerHost);	         
			((ImaProperties) connectionFactory).put("Port", ConnectionDetails.ismServerPort);
			// KeepAliveInterval is set to 0 to keep the connection from timing out
			((ImaProperties) connectionFactory).put("KeepAlive", 0);
			exceptionListener = new TestExceptionListener();
			ismConnection = connectionFactory.createConnection();
			ismConnection.setClientID(clientIDPub);
			ismConnection.setExceptionListener(exceptionListener);
		} catch (Throwable e) {
			e.printStackTrace();
			System.exit(1);
		}
		try {
			ismSession = ismConnection.createSession(false, Session.AUTO_ACKNOWLEDGE);
			startTopic = ismSession.createTopic(ismStartTopic);
			endTopic = ismSession.createTopic(ismEndTopic);			
			producer = ismSession.createProducer(startTopic);
			consumer = ismSession.createConsumer(endTopic);
			consumer.setMessageListener(this);
			ismConnection.start();
			producer.setDeliveryMode(delivMode);
			producer.setPriority(9);
		} catch (Throwable e) {
			System.out.println("Exception caught in JMS sample:  " + e.getMessage());
			e.printStackTrace();
		}
	}

	private void tearDownPublisherJMS() {
		/*
		 * close program handles
		 */
		try {
			System.out.println("stop: " + consumer);
			consumer.close();
			System.out.println("stop: " + ismConnection);
			ismConnection.stop();
			System.out.println("close producer: " + producer);
			producer.close();
			System.out.println("close session: " + ismSession);
			ismSession.close();
			System.out.println("ismConnection closed: " + ismConnection);
			ismConnection.close();
			System.out.println("");
		} catch (Throwable e) {
			e.printStackTrace();
		} 
	}

	private void sendJMSMessage(String messageID) {
		try {
			tmsg = ismSession.createTextMessage(messageID);
			System.out.println("Publishing \"" + messageID + "\" to topic " + ismStartTopic);
			tmsg = ismSession.createTextMessage(publication  + "TOKEN" + messageID);
			//		tmsg.setJMSDeliveryMode(delivMode);
			System.out.println("message is " + tmsg.getText());
			tmsg.setJMSCorrelationID("id" + messageID);
			Destination dest = endTopic;
			tmsg.setJMSReplyTo(dest);
			tmsg.setJMSPriority(9);
			tmsg.setStringProperty("key", "value");
			producer.send(tmsg);
			logger.printString("tmsg is " + ((Message)tmsg));
//			messagesSent++;
//			synchronized ( messageUtilities.payloadsLock) {
//				messageUtilities.getOriginalPayloads().add(messageID);
//				messageUtilities.getPayloads().add(messageID);
//				sentMessages.put(messageID, tmsg);
//			}
		} catch (JMSException e) {
			System.out.println("JMS Exception caught:  " + e.getMessage());
			e.printStackTrace();
			System.exit(1);	
		}
	}
	
	public void onMessage(Message message) {
		System.out.println("message returned");
		if (message instanceof BytesMessage) {
			BytesMessage bytesMessage = (BytesMessage)message;
			try {
				long bodyLength = bytesMessage.getBodyLength();
				if (bodyLength > Integer.MAX_VALUE) {
					System.out.println("The body length of the message received is too large: " + bodyLength);
					return;
				}
				byte[] msgBytes = new byte[(int)bodyLength];
				String messageData = new String();
				while(bytesMessage.readBytes(msgBytes) != -1) {
					String tmpStr = new String(msgBytes);
					messageData += tmpStr;
				}
				System.out.println("The message received is " + messageData);
			} catch (JMSException e) {
				e.printStackTrace();
				if(e.getLinkedException() != null) {
					e.getLinkedException().printStackTrace();
				}
			}
		} else if (message instanceof TextMessage) {
			{
				TextMessage tm = (TextMessage) message;
				try {
					System.out.println(tm.getText());
					System.out.println("TextMessage " + message);
					if (tmsg.getText().equalsIgnoreCase(tm.getText())) {
						System.out.println("round trip OK");
					} else {
						System.err.println("message mangled " + tm.getText());
					}
				} catch (JMSException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		} else if (message instanceof MapMessage) {
			{
				MapMessage mm = (MapMessage) message;
				System.out.println("map message " + mm);
			}
		} else if (message instanceof StreamMessage) {
			{
				StreamMessage sm = (StreamMessage) message;
				System.out.println("Stream Message " + sm);
			}
		} else if (message instanceof ObjectMessage) {
			{
				ObjectMessage om = (ObjectMessage) message;
				System.out.println("ObjectMessage " + om);
			}
		} else {
			System.out.println("message class is " + message.getClass());
		}
		messageReceived = true;
	}
	
	
}
