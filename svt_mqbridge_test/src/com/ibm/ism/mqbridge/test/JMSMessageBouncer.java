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

import com.ibm.mq.jms.MQConnectionFactory;
import com.ibm.msg.client.wmq.WMQConstants;

public class JMSMessageBouncer implements Runnable, MessageListener {

	private Connection mqConn;
	private Connection ismConnection;
	private String clientID = "JMSMessageBouncer";
	private Session mqSession;
	private String mqReceiveTopic;
	private String mqPutTopic;
	private MessageConsumer mqConsumer;
	private LoggingUtilities logger;
	private boolean messageBounced = false;
	private int sleepsPerformed =0;
	private MessageProducer mqProducer;

	JMSMessageBouncer (String topicToListenerOn, LoggingUtilities logger) {
		this.mqReceiveTopic = topicToListenerOn;
		this.logger = logger;
	}

	public void run() {
		createMQSubscriber();
		JMSRequestReplyISM2MQ2ISM.subStarted = true;
		while ((!messageBounced) && sleepsPerformed < 20) {
			try {
				Thread.sleep(1000);
				sleepsPerformed++;
				System.out.println("sleep " + sleepsPerformed);				
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		closeSubscriber();	
	}

	public void onMessage(Message message) {
		System.out.println("onMessage started");	
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
				String contents;
				try {
					contents = tm.getText();
					Destination replyDestination = tm.getJMSReplyTo();						
					String repDest = replyDestination.toString().substring(3);
					System.out.println("repDest " + repDest);
					Topic startTopic = mqSession.createTopic(repDest);			
					MessageProducer producer = mqSession.createProducer(startTopic);
					TextMessage replyMessage = mqSession.createTextMessage();
					replyMessage.setText(contents);
					replyMessage.setJMSCorrelationID(tm.getJMSMessageID());		
					System.out.println("repDest about to send");
					producer.send(replyMessage);
					System.out.println("repDest sent");
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
		messageBounced = true;	
	}

	private void createMQSubscriber() {
		ConnectionDetails.setUpMqClientConnectionDetails();
		MQConnectionFactory connFact = new MQConnectionFactory();		
		try {
			//We are connecting to queue manager using client mode
			connFact.setQueueManager(ConnectionDetails.queueManager);
			connFact.setHostName(ConnectionDetails.mqHostAddress);
			connFact.setPort(ConnectionDetails.mqPort);
			connFact.setTransportType(WMQConstants.WMQ_CM_CLIENT);
			mqConn = connFact.createConnection();			
			mqConn.setClientID(clientID );
			//	sess = mqConn.createSession(true, Session.SESSION_TRANSACTED);
			mqSession = mqConn.createSession(false, Session.AUTO_ACKNOWLEDGE);
			Topic myTopic = mqSession.createTopic(mqReceiveTopic);
			// consumer = sess.createDurableSubscriber(myTopic, "testSub");
			mqConsumer = mqSession.createConsumer(myTopic);
			mqConsumer.setMessageListener(this);
			logger.printString("JMS Subscriber started and subscribed to topic " + mqReceiveTopic);
			//Now we are ready to start receiving the messages. Hence start the connection
			mqConn.start();
		} catch (JMSException e) {
			e.printStackTrace();
			if(e.getLinkedException() != null) {
				e.getLinkedException().printStackTrace();
			}
			System.exit(1);
		}
	}

	private void closeSubscriber() {
		try {
			//	mqProducer.close();
			mqConn.stop();
			mqConsumer.close();
			mqSession.close();			
			mqConn.close();
			logger.printString("JMS Subscriber closed");
		} catch (JMSException e) {
			e.printStackTrace();
			if(e.getLinkedException() != null) {
				e.getLinkedException().printStackTrace();
			}
		}
	}

}
