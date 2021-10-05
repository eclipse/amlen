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
import java.util.UUID;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.Queue;
import javax.jms.Session;
import javax.jms.TextMessage;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.mqcon.boundaries.LargeMessageTestJmsQueueToQueue_ImaToIma;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.Trace;

public class ImaQueueToMqDestination extends MqJavaConnectivityTest {

	protected String clientID = "ImaTopicToMqDestination";
	protected boolean cleanSession = false;
	protected int qualityOfService = 2;
	protected int delivMode = DeliveryMode.PERSISTENT; 

	public  ImaQueueToMqDestination() {
		super();
		this.imaServerPort=16104;
	}	

	protected void sendMessages() throws JMSException  {	
		sendMessages(imaQueueName, testName);
	}

	protected void sendMessages(String queueName, String testString) throws JMSException {	
		if (Trace.traceOn()){
			Trace.trace("publishing with clientID " + clientID);
			Trace.trace("Sending " + testString + " " + + getNumberOfMessages() + " times: " );
			Trace.trace("Creating the default producer session for IMA");
		}
		JmsSession jmss = new JmsSession();
		Session producerSession = jmss.getSession(JmsSession.CLIENT_TYPE.IMA, imaServerHost, imaServerPort, "");				
		if(Trace.traceOn())
		{
			Trace.trace("Creating the message producer");
		}		
		Queue imaQueue = producerSession.createQueue(queueName);
		MessageProducer producer = producerSession.createProducer(imaQueue);
		producer.setDeliveryMode(delivMode );
		for (int count = 1; count <= getNumberOfMessages(); count++) {			
			String publication = testString + "messNo" +count;
			TextMessage msg = producerSession.createTextMessage(publication);
			// now send jms message
			producer.send(msg);
			addMessage(publication);
			if (Trace.traceOn()){
				Trace.trace("message sent: " + publication);
			}
		}
		jmss.closeConnection();		
		if (Trace.traceOn()){	
			Trace.trace("all messages sent");
		}	
	}

	protected void sendHaMessages(String host1, String host2) throws JMSException  {	
		sendHaMessages(imaQueueName, testName);
	}

	protected void sendHaMessages(String queueName, String testString, String host1, String host2, int portNumber, int startNumber) throws Exception {	
		int currentMessageNo=startNumber;
		if (Trace.traceOn()){
			Trace.trace("publishing with clientID " + clientID);
			Trace.trace("Sending " + testString + " " + + getNumberOfMessages() + " times: " );
			Trace.trace("Creating the default producer session for IMA");
		}
		ConnectionFactory connectionFactory = null;
		connectionFactory = ImaJmsFactory.createConnectionFactory();
		((ImaProperties) connectionFactory).put("Server", host1 + ", " + host2);
		((ImaProperties) connectionFactory).put("Port", portNumber);
		try {
			Connection connection = connectionFactory.createConnection();
			String clientID = UUID.randomUUID().toString().substring(0, 10);
			System.out.println("ClientID="+clientID);
			connection.setClientID(clientID);			
			Session session = connection.createSession(true, Session.SESSION_TRANSACTED);
			Queue destination = session.createQueue(queueName);
			MessageProducer producer = session.createProducer(destination);
			connection.start();
			if(Trace.traceOn())
			{
				System.out.println("Creating the message producer " + startNumber);
			}		

			producer.setDeliveryMode(delivMode );
			for (int count = startNumber; count <= getNumberOfMessages(); count++) {	
				currentMessageNo = count;
				String publication = testString + "messNo" +count;
				TextMessage msg = session.createTextMessage(publication);
				msg.setJMSDeliveryMode(DeliveryMode.PERSISTENT);
				// now send jms message
				producer.send(msg);
				session.commit();
				addMessage(publication);
				if (Trace.traceOn()) {
					System.out.println("message sent: " + publication);
				}
			}
			System.out.println("all messages sent");
			producer.close();
			session.close();
			connection.close();
		}
		catch (Exception e) {
			if (e.getMessage().contains("CWLNC") || e.getMessage().contains("Unexpected client failure")
					|| e.getMessage().contains("Connections attempts for all specified servers have failed")) {
				e.printStackTrace();
				System.out.println("currentMessageNo " + currentMessageNo);
				this.sendHaMessages(queueName, testString, host1, host2, portNumber, currentMessageNo);
			} else {
				System.out.println("message " + e.getLocalizedMessage());
				throw e;
			}
		}
		if (Trace.traceOn()){	
			System.out.println("all messages sent");
		}	
	}

	protected void sendLargeHaMessages(String queueName, String testString, String host1, String host2, int portNumber, int startNumber, int megaBytes) throws Exception {	
		int currentMessageNo=startNumber;
		if (Trace.traceOn()){
			Trace.trace("publishing with clientID " + clientID);
			Trace.trace("Sending " + testString + " " + + getNumberOfMessages() + " times: " );
			Trace.trace("Creating the default producer session for IMA");
		}
		ConnectionFactory connectionFactory = null;
		connectionFactory = ImaJmsFactory.createConnectionFactory();
		((ImaProperties) connectionFactory).put("Server", host1 + ", " + host2);
		((ImaProperties) connectionFactory).put("Port", portNumber);
		try {
			Connection connection = connectionFactory.createConnection();
			String clientID = UUID.randomUUID().toString().substring(0, 10);
			System.out.println("ClientID="+clientID);
			connection.setClientID(clientID);			
			Session session = connection.createSession(true, Session.SESSION_TRANSACTED);

			Queue destination = session.createQueue(queueName);
			MessageProducer producer = session.createProducer(destination);
			connection.start();
			if(Trace.traceOn())
			{
				System.out.println("Creating the message producer " + startNumber);
			}		
			String publication = LargeMessageTestJmsQueueToQueue_ImaToIma.createLargeMessage(megaBytes, 0);
			producer.setDeliveryMode(delivMode );
			for (int count = startNumber; count <= getNumberOfMessages(); count++) {	
				currentMessageNo = count;
				TextMessage msg = session.createTextMessage(publication);
				msg.setJMSDeliveryMode(DeliveryMode.PERSISTENT);
				// now send jms message
				producer.send(msg);
				session.commit();
				System.out.println("message sent: " + publication.length());
			}
			System.out.println("all messages sent");
			producer.close();
			session.close();
			connection.close();
		}
		catch (Exception e) {
			if (e.getMessage().contains("CWLNC") || e.getMessage().contains("Unexpected client failure")
					|| e.getMessage().contains("Connections attempts for all specified servers have failed") 
					|| e.getMessage().contains("Send message to server has failed")
					|| e instanceof IOException) {
				e.printStackTrace();
				System.out.println("currentMessageNo " + currentMessageNo);
				Thread.sleep(2000);
				this.sendLargeHaMessages(imaQueueName, testName, host1 , host2, imaServerPort,currentMessageNo,megaBytes);
			} else {
				System.out.println("message " + e.getLocalizedMessage());
				throw e;
			}
		}
		if (Trace.traceOn()){	
			System.out.println("all messages sent");
		}	
	}	
}
