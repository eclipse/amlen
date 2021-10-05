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

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.mq.jms.MQConnectionFactory;
public class JMSPublisher {
	private Connection conn;
	private Session sess;
	private MessageProducer publisher;
	public static Connection ismConnection = null; // for ism
	//static String topicName;
	static int count =1;
	static int persistence;

	public static void main(String[] args) {
		boolean ism = false;
		if (ism) {
			ismJmsSend("routes/Truck01");
		} else {
			JMSPublisher jmspub = new JMSPublisher();
			jmspub.createPublisher();
			for (int count =0; count < 10; count++) {
				try {
					jmspub.publisher.send(jmspub.sess.createTextMessage("Hello World: " + count));
					jmspub.publisher.send(jmspub.sess.createObjectMessage("Hello World object: " + count));
					jmspub.publisher.send(jmspub.sess.createObjectMessage(new Boolean(true)));
					MapMessage mapMessage = jmspub.sess.createMapMessage();
					mapMessage.setInt("number 1", 1);
					jmspub.publisher.send(mapMessage);
				} catch (JMSException e) {
					e.printStackTrace();
					if(e.getLinkedException() != null) {
						e.getLinkedException().printStackTrace();
					}
				}
			}
			System.out.println("JMS Publisher started and published message to topic routes/Truck01");
			jmspub.closePublisher();
		}	
	}

	private void closePublisher() {
		try {
			publisher.close();
			sess.close();
			conn.close();
			System.out.println("JMS Publisher closed");
		} catch (JMSException e) {
			e.printStackTrace();
			if(e.getLinkedException() != null) {
				e.getLinkedException().printStackTrace();
			}
		}
	}

	private void createPublisher() {
		MQConnectionFactory connFact = new MQConnectionFactory();
		try {
			//We are connecting to queue manager using client mode
			connFact.setQueueManager("lptptest");
			connFact.setHostName("127.0.0.1");
			connFact.setPort(1414);
			conn = connFact.createConnection();
			sess = conn.createSession(false, Session.AUTO_ACKNOWLEDGE);
			publisher = sess.createProducer(sess.createTopic("routes/Truck01"));
		} catch (JMSException e) {
			e.printStackTrace();
			if(e.getLinkedException() != null) {
				e.getLinkedException().printStackTrace();
			}
		}
	}
	
	private static void ismJmsSend(String topicName) {

	      try {
	         ConnectionFactory connectionFactory = ImaJmsFactory.createConnectionFactory();
	         String server = "192.168.56.2";
	         ((ImaProperties) connectionFactory).put("Server", server);
	         String port = "16102";
	         ((ImaProperties) connectionFactory).put("Port", port);

	         // KeepAliveInterval is set to 0 to keep the connection from timing out
	         ((ImaProperties) connectionFactory).put("KeepAlive", 0);
	         ismConnection = connectionFactory.createConnection();
	         ismConnection.setClientID("clientId");
	      } catch (Throwable e) {
	         e.printStackTrace();
	         return;
	      }

	      /*
	       * Depending on action executes the send or receive logic
	       */
	      try {
//	         if ("publish".equals(action)) {


		      Session session = ismConnection.createSession(false, 0);
		      Topic topic = session.createTopic(topicName);
		      MessageProducer producer = session.createProducer(topic);

		      if (persistence == 1) {
		         producer.setDeliveryMode(DeliveryMode.PERSISTENT);
		      } else {
		         producer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
		      }
		      String payload = "message";
		      TextMessage tmsg = session.createTextMessage(payload);

		      for (int i = 0; i < count; i++) {
		        
		            System.out.println(i + ": Publishing \"" + payload + "\" to topic " + topicName);
		            tmsg = session.createTextMessage(payload+ count);
		         producer.send(tmsg);
		      }

		      System.out.println("Published " + count + " messages to topic " + topicName);

		      ismConnection.stop();
		      producer.close();
		      session.close();
		   
//	         } else { // If action is not publish then it must be subscribe
//	        //    JMSSampleReceive.doReceive(this);
//	         }
	      } catch (Throwable e) {
	         System.out.println("Exception caught in JMS sample:  " + e.getMessage());
	         e.printStackTrace();
	      }

	      /*
	       * close program handles
	       */
	      try {
	    	  ismConnection.close();
	      } catch (Throwable e) {
	         e.printStackTrace();
	      }   
	}	
	
}
