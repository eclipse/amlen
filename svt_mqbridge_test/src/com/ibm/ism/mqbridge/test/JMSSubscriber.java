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
import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.ObjectMessage;
import javax.jms.Session;
import javax.jms.StreamMessage;
import javax.jms.TextMessage;

import com.ibm.mq.jms.MQConnectionFactory;

public class JMSSubscriber implements MessageListener {
	private Connection conn;
	private Session sess;
	private MessageConsumer consumer;
	public static Connection ismConnection = null; // for ism
	static String topicName;
	static int count;
	static int persistence;

	public static void main(String[] args) {
		JMSSubscriber jmssub = new JMSSubscriber();
		jmssub.createSubscriber();
		try {
			Thread.sleep(1000*60*5); // Run for 5 minutes
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		jmssub.closeSubscriber();
	}

	private void closeSubscriber() {
		try {
			conn.stop();
			consumer.close();
			sess.unsubscribe("testSub");
			sess.close();			
			conn.close();
			System.out.println("JMS Subscriber closed");
		} catch (JMSException e) {
			e.printStackTrace();
			if(e.getLinkedException() != null) {
				e.getLinkedException().printStackTrace();
			}
		}
	}

	private void createSubscriber() {
		MQConnectionFactory connFact = new MQConnectionFactory();
		try {
			//We are connecting to queue manager using client mode
			connFact.setQueueManager("lptptest");
			connFact.setHostName("127.0.0.1");
			connFact.setPort(1414);
			conn = connFact.createConnection();		
			conn.setClientID("myClient");
			sess = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);
			// consumer = sess.createDurableSubscriber(myTopic, "testSub");
			consumer = sess.createConsumer(sess.createTopic("routes/Truck01"));
			consumer.setMessageListener(this);
			System.out.println("JMS Subscriber started and subscribed to topic routes/Truck01");
			//Now we are ready to start receiving the messages. Hence start the connection
			conn.start();
		} catch (JMSException e) {
			e.printStackTrace();
			if(e.getLinkedException() != null) {
				e.getLinkedException().printStackTrace();
			}
		}
	}

	public void onMessage(Message message) {
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
	}
	
	
}
