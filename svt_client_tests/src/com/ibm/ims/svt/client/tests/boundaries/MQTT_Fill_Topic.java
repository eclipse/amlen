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
package com.ibm.ims.svt.client.tests.boundaries;



import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttPersistenceException;
import org.eclipse.paho.client.mqttv3.MqttSecurityException;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.config.ImaConfig.TYPE;
import static org.junit.Assert.assertTrue;
/*
 * Test to fill up a topic with messages, then publish a final message.
 * This checks that the 'Max Messages' value of an Messaging Policy is working correctly.
 */

public class MQTT_Fill_Topic {
	
	/*Set up information*/
	protected static String prefix = "FillTopicTest"; // default but each class can use a different one
	protected static String port = "23000"; // default but each class can use a different one
	private static String username = "admin";
	private static String password = "admin";
	private static String hub = null;
	private static String endpoint = null;
	private static String connectionPolicy = null;
	private static String messagingPolicy = null;
	private static String ipaddress = null;
	private static ImaConfig configHelper = null;
	
	/*Client information*/
	private static MqttClient subscriber = null;
	private static MqttClient publisher = null;	
	private static String TCPAddress = null;
	private static String topic = null;
	private static MqttConnectOptions conOpt = null;
	static MqttMessage message = null;
	protected static int numMessages = 5000;
	
	@BeforeClass
	public static void setUp() throws Exception {
		ipaddress = System.getProperty("ipaddress");
		if(ipaddress == null){
			System.out.println("Please pass in as system parameters the 'ipaddress' of the server. For example: -Dipaddress=9.3.177.15");
			System.exit(0);
		}
		
		//Username and password (default admin)
		if(System.getProperty("username") != null){
			username = System.getProperty("username");
		}
		
		if(System.getProperty("password") != null){
			password = System.getProperty("password");
		}
		
		//Connect to ima
		configHelper = new ImaConfig(ipaddress, username, password);
		configHelper.connectToServer();
		
		//Build up names for objects
		prefix = "FILLTOPIC"; // use the simple name to prefix all configuration items
		hub = prefix + "Hub";
		messagingPolicy = prefix + "MP";
		connectionPolicy = prefix + "CP";
		endpoint = prefix + "EP";
		
		
		
		//Delete objects if they exist		
		if(configHelper.endpointExists(endpoint)){
			configHelper.deleteEndpoint(endpoint);
		}
		
		if(configHelper.messagingPolicyExists(messagingPolicy)){
			configHelper.deleteMessagingPolicy(messagingPolicy);
		}
		
		if(configHelper.connectionPolicyExists(connectionPolicy)){
			configHelper.deleteConnectionPolicy(connectionPolicy);
		}
		
		if(configHelper.messageHubExists(hub)){
			configHelper.deleteMessageHub(hub);
		}
		
		configHelper.createHub(hub, hub);
		configHelper.createConnectionPolicy(connectionPolicy, connectionPolicy, TYPE.MQTT);
		configHelper.createMessagingPolicy(messagingPolicy, messagingPolicy, "*", TYPE.Topic, TYPE.MQTT);
		configHelper.createEndpoint(endpoint, endpoint, port, TYPE.MQTT, connectionPolicy, messagingPolicy, hub);
		
		//Configure Mqtt connection options, message, subscriber and publisher
		conOpt = new MqttConnectOptions();
		conOpt.setCleanSession(false);
		message = new MqttMessage();
		message.setQos(2);
		
		TCPAddress = "tcp://"+ipaddress+":"+port;
		subscriber = new MqttClient(TCPAddress, MQTT_Fill_Topic.class.getSimpleName()+"SUB");
		publisher = new MqttClient(TCPAddress, MQTT_Fill_Topic.class.getSimpleName()+"PUB");
		topic = "/" + MQTT_Fill_Topic.class.getSimpleName();
		
		//Connect and ensure that topic does not exist
		subscriber.connect();
		subscriber.unsubscribe(topic);
		subscriber.disconnect();
	}
	
	@Test
	public void fillTopicTest1() throws Exception{
		numMessages = 5000;
		assertTrue(publishMessages(configHelper, numMessages));
	}
	
	@Test
	public void fillTopicTest2() throws Exception{
		numMessages = 100;
		assertTrue(publishMessages(configHelper, numMessages));
	}
	
	@Test
	public void fillTopicTest3() throws Exception{
		numMessages = 10000;
		assertTrue(publishMessages(configHelper, numMessages));
	}
	
	@Test
	public void fillTopicTest4() throws Exception{
		numMessages = 1;
		assertTrue(publishMessages(configHelper, numMessages));
	}
	
	@Test
	public void fillTopicTest5() throws Exception{
		numMessages = 50000;
		assertTrue(publishMessages(configHelper, numMessages));
	}
	
	@Test
	public void fillTopicTest6() throws Exception{
		numMessages = 5000;
		assertTrue(publishMessages(configHelper, numMessages));
	}	
	
	/*
	 * In order to 'clear' the messages after every run, the client must connect and unsubscribe
	 * from the topic. (Alternately, the client could receive all messages, but this will be
	 * faster).
	 */
	@After
	public void tearDown() throws Exception{
		//Connect subscribing client
		subscriber.connect(conOpt);
		
		//Unsubscribe from topic
		subscriber.unsubscribe(topic);
		
		//Disconnect MQTT subscribing client
		subscriber.disconnect();
	}
	
	
	/*
	 * Performing clean up on IMA system. Delete all configuration once every test has
	 * been run.
	 */
	@AfterClass
	public static void tearDownClass() throws Exception {
		//Delete objects if they exist		
		if(configHelper.endpointExists(endpoint)){
			try {
				configHelper.deleteEndpoint(endpoint);
			} catch (Exception e) {
				System.out.println("Could not delete endpoint " + endpoint);
				System.out.println(e.getMessage());
			}
		}
		
		if(configHelper.messagingPolicyExists(messagingPolicy)){
			try {
				configHelper.deleteMessagingPolicy(messagingPolicy);
			} catch (Exception e) {
				System.out.println("Could not delete messaging policy " + messagingPolicy);
				System.out.println(e.getMessage());
			}
		}
		
		if(configHelper.connectionPolicyExists(connectionPolicy)){
			try {
				configHelper.deleteConnectionPolicy(connectionPolicy);
			} catch (Exception e) {
				System.out.println("Could not delete connection policy " + connectionPolicy);
				System.out.println(e.getMessage());
			}
		}
		
		if(configHelper.messageHubExists(hub)){
			try {
				configHelper.deleteMessageHub(hub);
			} catch (Exception e) {
				System.out.println("Could not delete message hub " + hub);
				System.out.println(e.getMessage());
			}
		}
	}
	
	/*
	 * To publish messages, an ImaConfig object is required to set the 'Max Messages'
	 * value of a messaging policy to the 'numMessages' value.
	 * 
	 * The 'numMessages' value is then used when publishing messages to a topic.
	 * 
	 * Once the topic is assumed to be full, one extra message is published. This should
	 * cause the publisher to lose connection. If the connection is lost, the returned 
	 * value is 'true', else it is false.
	 */
	public boolean publishMessages(ImaConfig configHelper, int numMessages){
		boolean connectionLost = false;
		
		try {
			//Update the 'Max Messages' value of the messaging policy.
			configHelper.updateMessagingPolicy(messagingPolicy, "MaxMessages", Integer.toString(numMessages));
					
			//Connect and subscribe to a topic (durable)
			subscriber.connect(conOpt);
			subscriber.subscribe(topic);
			
			//Disconnect MQTT subscribing client. This leaves the durable subscription in place
			//which will fill up with messages.
			subscriber.disconnect();
			
			//Connect and Publish MQTT messages up to limit. This number of messages should
			//completely fill the topic.
			publisher.connect();
			for(int i=0;i<numMessages;i++){
				publisher.publish(topic, message);
			}
			
			//Publish one more message and see if you get disconnected
			try {
				publisher.publish(topic, message);
			} catch (Exception e) {
				/*System.out.println("Could not publish the extra message (Expected result)");
				System.out.println(e.getMessage());*/
				if(e.getMessage().equals("Connection lost")){
					connectionLost = true;
				}
			}
			
		} catch (MqttSecurityException e) {
			System.out.println("A Mqtt security exception has occured");
			System.out.println(e.getMessage());
		} catch (MqttPersistenceException e) {
			System.out.println("A Mqtt persistence exception has occured");
			System.out.println(e.getMessage());
		} catch (MqttException e) {
			System.out.println("A Mqtt exception has occured");
		} catch (Exception e) {
			System.out.println("An exception has occured");
			System.out.println(e.getMessage());
		}
		
		return connectionLost;
	}
	
}
