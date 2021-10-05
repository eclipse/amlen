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

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import javax.jms.BytesMessage;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.ExceptionListener;
import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.MessageProducer;
import javax.jms.ObjectMessage;
import javax.jms.Queue;
import javax.jms.Session;
import javax.jms.StreamMessage;
import javax.jms.TextMessage;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

public class ISMQueueToISMQueue extends MessagingTest implements MessageListener {
	Connection ismConnection;
	private Session sess;
	private String queueName = "myqueue";
	private MessageConsumer consumer;
	private String clientIDPub= "Pub4ISMQ2ISMQ";
	private Session session;
	private Queue queue;
	private MessageProducer producer;
	static ISMQueueToISMQueue test;
	//static int  delivMode  =  DeliveryMode.PERSISTENT;
		static int delivMode  =  DeliveryMode.NON_PERSISTENT;
	private static long timeLastCalled;
	private Set<String> receivedMessages = new HashSet<String>();
	private Map<String, Message> sentMessages= new HashMap<String, Message>();
	private int messageArrived =0;
	private int messagesSent =0;
	private int messagesToSend =50;
	private ExceptionListener exceptionListener;
	public ISMQueueToISMQueue(String clientID) {
		super(clientID);
	}

	public static void main(String[] args) {
		
		try {
			test = new ISMQueueToISMQueue("ISMQ2ISMQ");
			// publish to ISM
			test.setupPublisherJMS();
			// create ISM subscription
			test.createSubscriber();
			for (int x=1; x<= test.messagesToSend ;x++) {	
				test.sendJMSMessage("DM" + delivMode + "messageNo", x);
			}
				// collect messages 
				timeLastCalled = System.currentTimeMillis();
				while ((timeLastCalled > (System.currentTimeMillis() - 15000))){// || test.messageArrived < test.messagesToSend) {
					try {
						Thread.sleep(1000);
						test.logger.printString("waiting for messages: " + test.messageArrived + " arrived");
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}			
				// check they all arrived	
				synchronized (messageUtilities.payloadsLock) {
					test.logger.printString("messageUtilities.getOriginalPayloads().size() " + messageUtilities.getOriginalPayloads().size());			
					test.logger.printString("messageUtilities.getPayloads().size() " + messageUtilities.getPayloads().size());
					test.logger.printString("extra received messages " + test.receivedMessages.size());
					if (messageUtilities.getPayloads().size() > 0) {
						for (String message : messageUtilities.getPayloads()) {
							test.logger.printString("missing message " + message);
						}
					}
				}
		} catch (Throwable e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}finally {
			// close ISM connection
			test.tearDownPublisherJMS();
		}
	}

	private void createSubscriber() throws JMSException {	
		try {
			sess = ismConnection.createSession(false, Session.AUTO_ACKNOWLEDGE);			
			consumer = sess.createConsumer(queue);
			consumer.setMessageListener(this);		
			//Now we are ready to start receiving the messages. Hence start the connection
			ismConnection.start();	
			logger.printString("JMS Subscriber started and subscribed to queue " + queueName);
		} catch (JMSException e) {
			e.printStackTrace();
			if(e.getLinkedException() != null) {
				e.getLinkedException().printStackTrace();
			}
			throw e;
		}
	}

	private void setupPublisherJMS () throws Throwable {
		System.out.println("publishing with clientID " + clientIDPub);      
		try {
			ConnectionFactory connectionFactory = ImaJmsFactory.createConnectionFactory();
			((ImaProperties) connectionFactory).put("Server", ConnectionDetails.ismServerHost);	         
			((ImaProperties) connectionFactory).put("Port", ConnectionDetails.ismServerQueuePort);
			// KeepAliveInterval is set to 0 to keep the connection from timing out
			((ImaProperties) connectionFactory).put("KeepAlive", 0);
			exceptionListener = new TestExceptionListener();
			ismConnection = connectionFactory.createConnection();
			ismConnection.setClientID(clientIDPub);
			ismConnection.setExceptionListener(exceptionListener);
		} catch (Throwable e) {
			e.printStackTrace();
			throw e;
		}
		try {
			session = ismConnection.createSession(true, 0);
			queue = session.createQueue(queueName);
			producer = session.createProducer(queue);
			System.out.println("delivMode " + delivMode);
			producer.setDeliveryMode(delivMode);
			producer.setPriority(9);
		} catch (Throwable e) {
			System.out.println("Exception caught in JMS sample:  " + e.getMessage());
			e.printStackTrace();
			throw e;
		}
	}

	private void tearDownPublisherJMS() {
		/*
		 * close program handles
		 */
		try {
			System.out.println("stop: " + ismConnection);
			ismConnection.stop();
			System.out.println("close producer: " + producer);
			if (producer !=null) {
			producer.close();
			}
			System.out.println("close session: " + session);
			if (session !=null) {
			session.close();
			}
			System.out.println("ismConnection closed: " + ismConnection);
			ismConnection.close();
			System.out.println("");
		} catch (Throwable e) {
			e.printStackTrace();
			// if something has gone wrong, there is not much we can do here
		} 
	}

	private void sendJMSMessage(String messageID, int x) {
		messageID = messageID + x;
		try {
			TextMessage tmsg = session.createTextMessage(messageID);
			System.out.println("Publishing \"" + messageID + "\" to topic " + queueName);
			tmsg = session.createTextMessage(MessagingTest.messageBody + "TOKEN" + messageID);
			//		tmsg.setJMSDeliveryMode(delivMode);
			System.out.println("message is " + tmsg.getText());
			tmsg.setJMSCorrelationID("id" + messageID);
			Destination dest = queue;
			tmsg.setJMSReplyTo(dest);
			tmsg.setJMSPriority(9);
			tmsg.setStringProperty("key", "value");
			producer.send(tmsg);
			logger.printString("tmsg is " + ((Message)tmsg));
				messagesSent++;
				synchronized (messageUtilities.payloadsLock) {
					messageUtilities.getOriginalPayloads().add(messageID);
					messageUtilities.getPayloads().add(messageID);
					sentMessages.put(messageID, tmsg);
				}
				logger.printString("tmsg is committed " + ((Message)tmsg));
				session.commit();			
		} catch (JMSException e) {
			System.out.println("JMS Exception caught:  " + e.getMessage());
			e.printStackTrace();
			System.exit(1);	
		}
	}
	public void onMessage(Message message) {	
		try {
			Thread.sleep(2000);
		} catch (InterruptedException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}		
		timeLastCalled = System.currentTimeMillis();
		messageArrived ++;
		String messageData = new String();
		if (message instanceof BytesMessage) {
			BytesMessage bytesMessage = (BytesMessage)message;
			try {
				long bodyLength = bytesMessage.getBodyLength();
				if (bodyLength > Integer.MAX_VALUE) {
					logger.printString("The body length of the message received is too large: " + bodyLength);
					return;
				}
				byte[] msgBytes = new byte[(int)bodyLength];
				while(bytesMessage.readBytes(msgBytes) != -1) {
					String tmpStr = new String(msgBytes);
					messageData += tmpStr;
				}
				logger.printString("The message received is " + messageData);
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
					messageData = tm.getText();
					logger.printString("The message received is " + messageData);
					logger.printString("TextMessage " + message);
				} catch (JMSException e) {
					e.printStackTrace();
				}
			}
		} else if (message instanceof MapMessage) {
			{
				MapMessage mm = (MapMessage) message;
				logger.printString("map message " + mm);
			}
		} else if (message instanceof StreamMessage) {
			{
				StreamMessage sm = (StreamMessage) message;
				logger.printString("Stream Message " + sm);
			}
		} else if (message instanceof ObjectMessage) {
			{
				ObjectMessage om = (ObjectMessage) message;
				logger.printString("ObjectMessage " + om);
				try {
					messageData = om.getObject().toString();
				} catch (JMSException e) {
					e.printStackTrace();
				}
				logger.printString("The message received is " + messageData);
			}
		} else {
			logger.printString("message class is " + message.getClass());
		}
		String[] messageSplit = messageData.split("TOKEN");
		String identifier = message.toString();
		if (messageSplit.length > 1) {
			identifier = messageSplit[1];
		}
		logger.printString("messagesProcessed is " + messageArrived);
		logger.printString("identifier is " + identifier);
		logger.printString("message body is " + messageSplit[0]);	
		String messageArrived = null;
		try {
			messageArrived = "Message arrived: \"" + identifier +
			"\" on " + message.getJMSDestination().toString() + "\" for instance \"" +messageData + "\"";
		} catch (JMSException e) {
			e.printStackTrace();
		}
		logger.printString(messageArrived);
		if (messageSplit[0].equalsIgnoreCase( MessagingTest.messageBody)) {
			logger.printString("message body is OK");
		} else {
			reason =  reason + "body of message corrupted: " ;
			reason =  reason + messageSplit[0] + " should be " +  MessagingTest.messageBody + "\n";
		}

		synchronized (messageUtilities.payloadsLock) {
			Message sentMessage = null;
			sentMessage = sentMessages.get(identifier);
			checkMessageProperties(sentMessage, message);
			if (! messageUtilities.getPayloads().contains(identifier)) {
				success = false;
				logger.printString("identifier " + identifier);
				logger.printString(" originalPayloads " +  messageUtilities.getOriginalPayloads());
				if ( messageUtilities.getOriginalPayloads().contains(identifier)){
					//  if (! reason.contains("duplicate message received")) {
					reason =  reason + "duplicate message received: " + identifier + " MQ per: " +  delivMode +"\n";
					//  }
				} else {
					reason =  reason + "unexpected message received: " + identifier + " MQ per: " +  delivMode +"\n";
				}
				receivedMessages.add(identifier);
			} else {
				messageUtilities.getPayloads().remove(identifier);				
			}
		}	
	}

	private void checkMessageProperties(Message sentMessage, Message receivedMessage) {
		logger.printString("sent message is " + sentMessage);
		try {
			logger.printString("receivedMessage message is " + ((TextMessage)receivedMessage).getText());
		} catch (JMSException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
//		try {
//			if (receivedMessage.getJMSDeliveryMode() == sentMessage.getJMSDeliveryMode()) {
//				logger.printString("delivery modes match: " + receivedMessage.getJMSDeliveryMode());
//			} else {
//				logger.printString("delivery mode mismatch\n sent:" + sentMessage.getJMSDeliveryMode() + " received: " + receivedMessage.getJMSDeliveryMode());
//			}
//			if (receivedMessage.getJMSCorrelationID().equalsIgnoreCase(sentMessage.getJMSCorrelationID())) {
//				logger.printString("correl ids match: " + receivedMessage.getJMSCorrelationID());
//			} else {
//				logger.printString("correl ids mismatch\n sent:" + sentMessage.getJMSCorrelationID() + " received: " + receivedMessage.getJMSCorrelationID());
//			}
//			String rm = receivedMessage.getJMSReplyTo().toString();
//			if (rm.startsWith("://")) {
//				rm = rm.substring(3);
//				String sm = sentMessage.getJMSReplyTo().toString();
//				if (rm.equalsIgnoreCase(sm)) {
//					logger.printString("reply to topics match: " + sm);
//				} else {
//					logger.printString("reply to mismatch\n sent:" + sm + " received: " + rm);
//				}
//			} else {
//				if (receivedMessage.getJMSReplyTo() == sentMessage.getJMSReplyTo()) {
//					logger.printString("reply to topics match: " + sentMessage.getJMSReplyTo());
//				} else {
//					logger.printString("reply to topics mismatch\n sent:" + sentMessage.getJMSReplyTo() + " received: " + receivedMessage.getJMSReplyTo());
//				}	
//			}
//			if (receivedMessage.getJMSPriority() == sentMessage.getJMSPriority()) {
//				logger.printString("priorities match: " + sentMessage.getJMSPriority());
//			} else {
//				logger.printString("priorities mismatch\n sent:" + sentMessage.getJMSPriority() + " received: " + receivedMessage.getJMSPriority());
//			}		
//			if (receivedMessage.getStringProperty("key") .equals(sentMessage.getStringProperty("key")) ) {
//				logger.printString("keys match: " + sentMessage.getStringProperty("key"));
//			} else {
//				logger.printString("keys mismatch\n sent:" + sentMessage.getStringProperty("key") + " received: " + receivedMessage.getStringProperty("key"));
//			}		
//			if (receivedMessage.getJMSExpiration() == (sentMessage.getJMSExpiration()) ) {
//				logger.printString("expirations match: " + sentMessage.getJMSExpiration());
//			} else {
//				logger.printString("expirations mismatch\n sent:" + sentMessage.getJMSExpiration() + " received: " + receivedMessage.getJMSExpiration());
//			}
//		} catch (JMSException e) {
//			// TODO Auto-generated catch block
//			e.printStackTrace();
//		}
	}
}


