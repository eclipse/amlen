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
package com.ibm.ima.mqcon.enabledediting;

import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.Topic;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttMessage;


import com.ibm.ima.mqcon.enabledediting.JmsToJms;
import com.ibm.ima.mqcon.msgconversion.ReturnCodes;
import com.ibm.ima.mqcon.utils.JmsMessage;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.Trace;

/**
 * Test 7 - MQ Topic Subtree to IMA Topic Subtree
 * 
 * This test will set up a destination mapping rule from an MQ topic Subtree
 * to an IMA topic Subtree. It will update the 'RetainedMessages' value from 'None'
 * to 'All' and back again while the rule is still Enabled.
 * 
 * Messages will be checked to ensure they are retained only when appropriate
 *
 */

public class Retained_MQTopicSubtreeToIMATopicSubtree extends JmsToJms {
	
	static EnabledEditConfig config;
	static String testName = "Retained_MQTopicSubtreeToIMATopicSubtree";
	static MqttMessage mqttMsg;
	static MqttClient mqttClient;
	
	public static void main(String[] args) {
		
		new Trace().traceAutomation();
		Retained_MQTopicSubtreeToIMATopicSubtree testClass = new Retained_MQTopicSubtreeToIMATopicSubtree();
		testClass.setNumberOfMessages(1);
		testClass.setNumberOfPublishers(1);
		testClass.setNumberOfSubscribers(1);
		testClass.setTestName(testName);
		
		// Get the hostname and port passed in as a parameter
		try
		{
			if (args.length==0)
			{
				if(Trace.traceOn())
				{
					Trace.trace("\nTest aborted, test requires command line arguments!! (IMA IP address, IMA port, MQ IP address, MQ port, MQ queue manager, IMA topic, MQ topic, timeout)");
				}
				RC = ReturnCodes.INCORRECT_PARAMS;
				System.exit(RC);
			}
			
			else if(args.length == 7)
			{
				imahostname = args[0];
				imaport = new Integer((String)args[1]);
				mqhostname = args[2];
				mqport = new Integer((String)args[3]);
				mqqueuemanager = args[4];
				imadestinationName = args[5];
				mqDestinationName = args[6];
				
				if(Trace.traceOn())
				{
					Trace.trace("The arguments passed into test class are: " + imahostname + ", " + imaport + ", " + mqhostname + ", " + mqport + ", " + mqqueuemanager + ", " + imadestinationName + ", " + mqDestinationName);
				}
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("Invalid Commandline arguments! System Exit.");
				}
				RC = ReturnCodes.INCORRECT_PARAMS;
				System.exit(RC);
			}
			config = new EnabledEditConfig(imahostname, imaport, mqhostname, mqport, mqqueuemanager, imadestinationName, mqDestinationName, testClass.getTestName());
			config.setup();
			testClass.startTest();
			config.teardown();
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}	
	}

	
	/*
	 * Constructors
	 */
	public Retained_MQTopicSubtreeToIMATopicSubtree(){
		//TODO - Fill this in (with what?)
	}
	
	public Retained_MQTopicSubtreeToIMATopicSubtree(EnabledEditConfig configObject){
		config = configObject;
	}
	
	/*
	 * Kicks off test using values from 'config'
	 */
	
	public void startTest(){
		try {
			//Create mapping rules
			if(!config.mqConn.mappingRuleExists(testName)){
				Trace.trace("Creating destination mapping rule");
				config.mqConn.createMappingRule(testName, config.prefixName, "9", mqDestinationName, imadestinationName);
				if(config.mqConn.mappingRuleExists(testName)){
					Trace.trace("Destination mapping rule created");
				} else {
					Trace.trace("Could not create destination mapping rule");
				}
			} else {
				Trace.trace("Destination mapping rule already exists");
			}
			
			//Set up producer and consumer sessions
			Trace.trace("Setting up Producer and Consumer sessions.");
			Session producerSession = producerJMSSession.getSession(JmsSession.CLIENT_TYPE.MQ, mqhostname, mqport, mqqueuemanager);
			Session consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null);
			
			//Set up topics and message
			Trace.trace("Setting up Topics and Message");
			
			
			String Subtree1 = "/LEVEL1";
			String Subtree2 = "/LEVEL1/LEVEL2";
			
			Topic mqDestination =  producerSession.createTopic(mqDestinationName);
			Topic mqDestinationSubtree1 =  producerSession.createTopic(mqDestinationName + Subtree1);
			Topic mqDestinationSubtree2 =  producerSession.createTopic(mqDestinationName + Subtree2);
			
			Topic imaDestination = consumerSession.createTopic(imadestinationName);
			Topic imaDestinationSubtree1 = consumerSession.createTopic(imadestinationName + Subtree1);
			Topic imaDestinationSubtree2 = consumerSession.createTopic(imadestinationName + Subtree2);

			Message msg = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			
			//Only used to clear retained messages on IMA
			String mqttHostName = "tcp://"+imahostname+":"+imaport;
			System.out.println(mqttHostName);
			mqttClient = new MqttClient(mqttHostName, "MQTT_Clearer");
			mqttMsg = new MqttMessage();
			mqttMsg.setQos(2);
			mqttMsg.setRetained(true);
			mqttClient.connect();
			
			//Create producer and consumer
			Trace.trace("Creating Producer and Consumer1");
			MessageProducer producer1 = producerSession.createProducer(mqDestination);
			MessageProducer producer2 = producerSession.createProducer(mqDestinationSubtree1);
			MessageProducer producer3 = producerSession.createProducer(mqDestinationSubtree2);
			
			MessageConsumer consumer1 = consumerSession.createConsumer(imaDestination);
			
			//Start the consumer session and send a message
			Trace.trace("Starting Consumer session Connection");
			consumerJMSSession.startConnection();
			Trace.trace("Sending message");
			producer1.send(msg);
			
			//Receive message and determine if it is retained. It should not be
			Message receivedMessage = null;
			Trace.trace("Receiving message with Consumer1");
			receivedMessage = consumer1.receive(5000);
			if (receivedMessage != null){
				if(Integer.valueOf(receivedMessage.getStringProperty("JMS_IBM_Retain")) == 0){
					Trace.trace("Message was received and was not retained. Pass!");
				} else {
					Trace.trace("Message was received and was retained. It should not have been. Fail!");
					RC = ReturnCodes.INCORRECT_MESSAGE;
				}
			} else {
				Trace.trace("Message was not received. Fail!");
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
			}
			consumer1.close();
			
			//Attempt to receive message as a different consumer. None should come through
			Trace.trace("Attempting to receive message with Consumer2");
			MessageConsumer consumer2 = consumerSession.createConsumer(imaDestination);
			Message receivedMessage2 = null;
			receivedMessage2 = consumer2.receive(5000);
			if (receivedMessage2 == null){
				Trace.trace("No message was received. Pass!");
			} else {
				Trace.trace("Message was received when it should not have been. Fail!");
				RC = ReturnCodes.INCORRECT_MESSAGE;
			}
			consumer2.close();
	
			//Update the rule to retain messages
			Trace.trace("Updating destination mapping rule. Setting RetainedMessages to 'All'");
			config.mqConn.updateMappingRule(testName, "RetainedMessages", "All");
			
			//Send another message
			Trace.trace("Sending another message");
			consumer1 = consumerSession.createConsumer(imaDestinationSubtree1);
			producer2.send(msg);
			
			//Receive message and determine if it is retained. It should not be
			receivedMessage = null;
			Trace.trace("Receiving next message with Consumer1");
			receivedMessage = consumer1.receive(5000);
			if (receivedMessage != null){
				if(Integer.valueOf(receivedMessage.getStringProperty("JMS_IBM_Retain")) == 0){
					Trace.trace("Message was received and was not retained. Pass!");
				} else {
					Trace.trace("Message was received and was retained. It should not have been. Fail!");
					RC = ReturnCodes.INCORRECT_MESSAGE;
				}
			} else {
				Trace.trace("Message was not received. Fail!");
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
			}
			consumer1.close();
			
			//Attempt to receive message. It should be retained
			Trace.trace("Attempting to receive another message with Consumer2");
			consumer2 = consumerSession.createConsumer(imaDestinationSubtree1);
			receivedMessage2 = null;
			receivedMessage2 = consumer2.receive(5000);
			if (receivedMessage2 != null){
				if(receivedMessage2.getIntProperty("JMS_IBM_Retain") == 1){
					Trace.trace("Message was received and was retained. Pass!");
				} else if (receivedMessage.getStringProperty("JMS_IBM_Retain") == null){
					Trace.trace("Message was received and was not retained. It should have been. Fail!");
					RC = ReturnCodes.INCORRECT_MESSAGE;
				} else {
					Trace.trace("Message was not received. Fail!");
					RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				}
			} else {
				Trace.trace("Message was not received. Fail!");
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
			}
			consumer2.close();
			
			//Clear the retained publication
			Trace.trace("Clearing retained publication");
			mqttClient.publish(imadestinationName+Subtree1, mqttMsg);
			
			//Thread.sleep(30000);
			
			//Update mapping rule to not retain messages
			Trace.trace("Updating destination mapping rule. Setting RetainedMessages to 'None'");
			config.mqConn.updateMappingRule(testName, "RetainedMessages", "None");
			
			//Send another message
			Trace.trace("Sending another message");
			consumer1 = consumerSession.createConsumer(imaDestinationSubtree2);
			producer3.send(msg);
			
			//Receive message and determine if it is retained. It should not be
			Trace.trace("Receiving next message with Consumer1");
			receivedMessage = null;
			receivedMessage = consumer1.receive(5000);
			if (receivedMessage != null){
				if(Integer.valueOf(receivedMessage.getStringProperty("JMS_IBM_Retain")) == 0){
					Trace.trace("Message was received and was not retained. Pass!");
				} else {
					Trace.trace("Message was received and was retained. It should not have been. Fail!");
					RC = ReturnCodes.INCORRECT_MESSAGE;
				}
			} else {
				Trace.trace("Message was not received. Fail!");
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
			}
			
			//Attempt to receive message. None should come through
			Trace.trace("Attempting to receive another message with Consumer2");
			consumer2 = consumerSession.createConsumer(imaDestinationSubtree2);
			receivedMessage2 = null;
			receivedMessage2 = consumer2.receive(5000);
			if (receivedMessage2 == null){
				Trace.trace("No message was received. Pass!");
			} else {
				Trace.trace("Message was received when it should not have been. Fail!");
				RC = ReturnCodes.INCORRECT_MESSAGE;
/*				config.teardown();
				System.exit(RC);*/
			}
			
			Trace.trace("Clearing retained publication");
			mqttClient.publish(imadestinationName+Subtree2, mqttMsg);
			
			if(RC != ReturnCodes.INCOMPLETE_TEST)
			RC = ReturnCodes.PASSED;
			
			Trace.trace("Closing producer");
			producer1.close();
			producer2.close();
			producer3.close();
			Trace.trace("Closing consumers");
			consumer1.close();
			consumer2.close();
			Trace.trace("Closing producer session");
			producerSession.close();
			Trace.trace("Closing consumer session");
			consumerSession.close();
			
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		//Tear down for Test 1
		try {
			mqttClient.publish(imadestinationName, mqttMsg);
			mqttClient.disconnect();
			
			if(config.mqConn.mappingRuleExists(testName)){
				Trace.trace("Disabling and deleting destination mapping rule");
				config.mqConn.updateMappingRule(testName, "Enabled", "False");
				config.mqConn.deleteMappingRule(testName);
				if(!config.mqConn.mappingRuleExists(testName)){
					Trace.trace("Destination mapping rule deleted");
				}
			}

			
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
}
