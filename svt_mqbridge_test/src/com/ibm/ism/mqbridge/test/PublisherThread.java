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
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.micro.client.mqttv3.MqttClient;
import com.ibm.micro.client.mqttv3.MqttConnectOptions;
import com.ibm.micro.client.mqttv3.MqttDeliveryToken;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttMessage;
import com.ibm.micro.client.mqttv3.MqttTopic;

public class PublisherThread implements Runnable {

	String topicString = "";
	String clientID ="";
	String threadName="";
	MessagingTest test;
	static int qualityOfService;
//	private long sleepTimeout= 10000;
	private LoggingUtilities logger;
//	private String testName;
	Connection ismConnection = null; // for ism
	//static String topicName;
	static int messageCount =1;
	static int persistence;
	ConnectionFactory connectionFactory;
	Session session;
    Topic topic;
    MessageProducer producer;
	
	public PublisherThread(String topicString, String clientID) {
		this.topicString = topicString;
		this.clientID = clientID;

	}

	public PublisherThread(String topicName,
			MessagingTest test, int threadNumber) {
		// TODO Auto-generated constructor stub
		this.topicString = topicName;
		this.test = test;
		this.clientID = test.getClientID();
		this.threadName = clientID + threadNumber;
//		this.testName = isMtoMQManyToOneTest.testName;
		qualityOfService = test.getQualityOfService();
	}

	public PublisherThread(String string, MessagingTest MessagingTest,
			int threadNumber, LoggingUtilities logger) {
		this(string, MessagingTest,threadNumber);
		this.logger = logger;
	}

	public void run() {
	//	log = MessagingTest.getLog();
		messageCount = MessagingTest.getNumberOfMessages();
		logger.printString("starting publisher thread");
		MqttMessage message; 
		MqttTopic topic = null;
		MqttClient client = null;
		try {

			logger.printString("threadName is " + threadName);
			boolean isJms = MessagingTest.jmsPub;
			if (!isJms)
			{
				String host = "tcp://" + ConnectionDetails.ismServerHost + ":" + ConnectionDetails.ismServerPort;	
			//String host = "tcp://9.20.230.123:16102";
			// String host = "tcp://192.168.56.2:16102";
			// String host = "tcp://localhost:16102";
			//	threadName = "__MQConnectivity";
			client = new MqttClient(host,threadName,null);
			//	MqttClient client = new MqttClient(Example.TCPAddress,clientID);
			
			 
			MqttConnectOptions mco = new MqttConnectOptions();
			boolean publisherConnected = false;
			while (!publisherConnected) {
			try {
				logger.printString("connecting to: " + host);
			client.connect(mco);
			publisherConnected= true;
			}
			catch (MqttException mqtte) {
				if (!mqtte.getLocalizedMessage().contains("2033")) {
					logger.printString(mqtte.getLocalizedMessage());
					logger.printString("MqttException occurred:");
					mqtte.printStackTrace();
			//		System.exit(1);
				}
			}
			}
			// String [] topics;
			// {"/Sport/Football/Results/Chelsea", "/Sport/Rugby/Results", Example.topicString };


//			List<String> sent = new ArrayList<String>();
//			sent = MessagingTest.getSent();
//			String [] topics = new String[sent.size()];
//			int topicNumber = 0;
//			for (String topicString: sent) {			
//				topics[topicNumber] = topicString;
//				topicNumber++;
//			}
			// sleep to allow sub to be set up

			topic = client.getTopic(topicString);
		//	while (!test.getSubscriber().isOpen()) {Thread.sleep(500);}
		}
			else {
				System.out.println("set up threadName " + threadName );
				setupJMS();
			}
//			if (MessagingTest.jmsSub)
//			{
//				while (!MessagingTest.subscribed) {
//				Thread.sleep(1000);
//				}
//			} else {
			while (!test.canStartPublishing()) {
				logger.printString("waiting for subscribers");
				Thread.sleep(1000);
			}
//			}
			String publication = "";

			byte [] publicationBytes = publication.getBytes();
			message = new MqttMessage(publicationBytes);
			//message.setQos(2);
			MqttDeliveryToken token;
			logger.printString("publishing to " + topicString);
			String identifier="";
			for (int count =0; count < messageCount ; count++) {
				Thread.sleep(5);  // TODO - remove this line
			//	message = ISMtoMQOneToOneTest.getMessages().get(count);
			//	int qos = message.getQos();
			//	publication = "HelloWorld"+count+threadName;
				identifier = ""+count+test.testName + "JMS" + isJms;
				publication = MessagingTest.messageBody + "TOKEN" +identifier ;
				
				publicationBytes = publication.getBytes();
				logger.printString("publicationBytes " + publicationBytes.length);
				synchronized (MessagingTest.messageUtilities.payloadsLock) {
					MessagingTest.messageUtilities.getOriginalPayloads().add(identifier);
					MessagingTest.messageUtilities.getPayloads().add(identifier);
					}				
				if (!isJms)
				{
				message = new MqttMessage(publicationBytes);
				message.setQos(qualityOfService);
				String messageToPrint="";
			//	if (logger.isVerboseLogging()) {
					if (message.toString().length() > 1000) {
					messageToPrint = "long message, ID " + identifier; 
					} else {messageToPrint = message.toString();}
				logger.printString("" + MessagingTest.timeNow()  +  " publication of mqtt\"" + messageToPrint
						+ "\" with QoS = " + message.getQos() + " to " + topic.getName());
				
			//	}
				//		  logger.printString(" on topic \"" + topic.getName()
				//				  + "\" for client instance: \"" + client.getClientId()
				//				  + "\" on address " + client.getServerURI() + "\"");
				//	topic.publish(message);
				boolean published = false;
				token = topic.publish(message);


				while (!published) {
					try { 

						token.waitForCompletion(MessagingTest.sleepTimeout);  
				//		if (MessagingTest.verboseLogging) {
						logger.printString("Delivery token \"" + token.hashCode()
								+ "\" has been received: " + token.isComplete());
				//		}
						published = true;	  
					} 
					catch (MqttException mqtte) {
						mqtte.printStackTrace();
						Thread.sleep(MessagingTest.sleepTimeout);
					}
				}
				} else {
					// send JMS message
					ismJmsSend(topicString, publication);
					logger.printString("" + MessagingTest.timeNow()  +  " publication of jms\"" + publication
							+ " to " + topicString);
				}						
			}
			if (!isJms) {
			client.disconnect();
			} else {
				tearDownJMS();
			}
		} catch (Exception e) {
			logger.printString("*******************************************************************************************");
			e.printStackTrace();
			Class<? extends Exception> clazz = e.getClass();
			String className = clazz.toString();
			logger.printString("className is " + className);
			logger.printString("*******************************************************************************************");
			System.exit(1);
		}		
	}
	
	private void setupJMS () {
		System.out.println("threadName " + threadName);
	      try {
		         connectionFactory = ImaJmsFactory.createConnectionFactory();
		       //  String server = "192.168.56.2";
		        // String server = "9.20.230.123";
		            ((ImaProperties) connectionFactory).put("Server", ConnectionDetails.ismServerHost);
		         
		            ((ImaProperties) connectionFactory).put("Port", ConnectionDetails.ismServerPort);

		         // KeepAliveInterval is set to 0 to keep the connection from timing out
		         ((ImaProperties) connectionFactory).put("KeepAlive", 0);
		         ismConnection = connectionFactory.createConnection();

		         ismConnection.setClientID(threadName);
		      } catch (Throwable e) {
		         e.printStackTrace();
		         return;
		      }
		      try {
			      session = ismConnection.createSession(false, 0);
			      topic = session.createTopic(topicString);
			      producer = session.createProducer(topic);

			      if (qualityOfService == 1 || qualityOfService == 2) {
			         producer.setDeliveryMode(DeliveryMode.PERSISTENT);
			      } else {
			         producer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
			      }
		      } catch (Throwable e) {
			         System.out.println("Exception caught in JMS sample:  " + e.getMessage());
			         e.printStackTrace();
			      }
	}
	private void tearDownJMS() {
    /*
     * close program handles
     */
    try {
    	System.out.println("ismConnection.stop();" + ismConnection);
	      ismConnection.stop();
	      System.out.println(" producer.close();" + producer);
	      producer.close();
	      System.out.println("session.close();" + session);
	      session.close();
	      System.out.println("ismConnection.close();" + ismConnection);
  	  ismConnection.close();
  	System.out.println("");
    } catch (Throwable e) {
       e.printStackTrace();
    } 
	}
	private void ismJmsSend(String topicName, String publication) {

	      try {
		      TextMessage tmsg = session.createTextMessage(publication);
		    //        System.out.println("Publishing \"" + publication + "\" to topic " + topicName);
		            tmsg = session.createTextMessage(publication);
		//            System.out.println("publication is " + publication);
		         producer.send(tmsg);
		         logger.printString("producer class " + producer.getClass());
		         logger.printString("producer DM " + producer.getDeliveryMode());
		         logger.printString("TextMessage JMS DM " + tmsg.getJMSDeliveryMode());
		   //      System.out.println("tmsg is " + tmsg.getText());
		    //  System.out.println("Published " + messageCount + " messages to topic " + topicName);
		      //  session.unsubscribe(topicName);

		   
//	         } else { // If action is not publish then it must be subscribe
//	        //    JMSSampleReceive.doReceive(this);
//	         }
	      } catch (Throwable e) {
	         System.out.println("Exception caught in JMS sample:  " + e.getMessage());
	         e.printStackTrace();
	      }  
	}
	
	
	
	
}
