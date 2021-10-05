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
package com.ibm.ima.svt.mq.recoverability;


import java.util.ArrayList;

import junit.framework.TestCase;

import org.junit.Assert;

import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.config.ImaMQConnectivity;
import com.ibm.ima.test.cli.config.ImaConfig.TYPE;
import com.ibm.ima.test.cli.imaserver.ImaServerStatus;
import com.ibm.ima.test.cli.mq.MQConfig;
import com.ibm.ims.svt.clients.JmsMessage;
import com.ibm.ims.svt.clients.JmsProducerClient;
import com.ibm.ims.svt.clients.MqttSubscriberClient;
import com.ibm.ims.svt.clients.Trace;

/**
 * This class is the base for multiple queue manager connection tests.
 *
 */
public abstract class TestMultipleMQQmgrsBase extends TestCase {
	
	protected static String prefix = null;
	protected static String imaipaddress = null;
	protected static String imausername = "admin";
	protected static String imapassword = "admin";
	protected static String mqipaddressA = null;
	protected static String mqusernameA = "mquser";
	protected static String mqpasswordA = null;
	protected static String mqipaddressB = null;
	protected static String mqusernameB = "mquser";
	protected static String mqpasswordB = null;
	protected static int mqtcpportA = -1;
	protected static int mqtcpportB = -1;
	protected static String mqlocationA ="/opt/mqm/bin";
	protected static String mqlocationB ="/opt/mqm/bin";
	protected static String mquser = "root"; // This needs to be a user that is not in the mqm group
	protected static String imaport = null;
	protected static boolean clientTrace = true; // This can be set at runtime to trace each producer and consumer
	
	protected static String qmgrA = null;
	protected static String qmgrB = null;
	protected static String listenerName = "IMA_LISTENER";
	protected static String channelName = "IMA_MQCON";
	protected static String clientChannelName = "IMA_CLIENTS";
	
	protected static String mqQueueIn = "TESTMQ_QIN";
	protected static String mqQueueOut = "TESTMQ_QOUT";
	protected static String imaQueueIn = "IMA_Q";
	protected static String imaTopicOut = "IMA_T";
	
	protected static String hub = null;
	protected static String endpoint = null;
	protected static String connectionPolicy = null;
	protected static String messagingPolicyQ = null;
	protected static String messagingPolicyT = null;
	
	protected static MQConfig mqConfigA = null;
	protected static MQConfig mqConfigB = null;
	protected static ImaConfig imaConfig = null;
	protected static ImaMQConnectivity imaMqConfig = null;
	protected static ImaServerStatus imaServer = null;
	
	protected int consumerWait = 90000; // This is the time we will wait for the consumers to catch up once producers have been stopped
	protected int intervalTime = 60000; // This is the time inbetween recoverability actions we will wait before performing the next action
	protected ArrayList<JmsProducerClient> jmsProducersMap1 = new ArrayList<JmsProducerClient>();
	protected ArrayList<JmsProducerClient> jmsProducersMap2 = new ArrayList<JmsProducerClient>();
	protected ArrayList<MqttSubscriberClient> mqttSubscribers = new ArrayList<MqttSubscriberClient>();
	protected String[] dumpFiles = null;
	
	protected int qmgrADepth = -1;
	protected int qmgrBDepth = -1;
	
	protected boolean onewayonly = false;
	protected static boolean testAutomation = false;
	

	// This method must set the ports and qmgrs names
	public abstract void setInitialParams();
	
	protected void setUp() throws Exception {
		
		new Trace("stdout", true);
		setInitialParams();
		
		
		if(!testAutomation)
		{
			imaipaddress = System.getProperty("ipaddress");
			mqipaddressA = System.getProperty("mqipaddressA");
			mqpasswordA  = System.getProperty("mqpasswordA");
			mqipaddressB = System.getProperty("mqipaddressB");
			
		}
		
		// Set up an MQ queue manager to perform the testing
		
		
		if(imaipaddress == null)
		{
			System.out.println("Please pass in as system parameters the 'ipaddress' of the server. For example: -Dipaddress=9.3.177.15");
			System.exit(0);
		}
		
		
		if(mqipaddressA == null)
		{
			System.out.println("Please pass in as system parameters the 'mqipaddressA' of the server. For example: -DmqipaddressA=9.3.179.191");
			System.exit(0);
		}
		
		
		
		if(mqpasswordA == null)
		{
			System.out.println("Please pass in as system parameters the 'mqpasswordA' of the server. For example: -DmqpasswordA=password");
			System.exit(0);
		}
		
		
		
		if(mqipaddressB == null)
		{
			System.out.println("Please pass in as system parameters the 'mqipaddressB' of the server. For example: -DmqipaddressB=9.3.179.191");
			System.exit(0);
		}
				
		if(mqpasswordB == null)
		{
			System.out.println("Please pass in as system parameters the 'mqpasswordB' of the server. For example: -DmqpasswordB=password");
			System.exit(0);
		}
		
		// This allows someone to configure the test differently at runtime		
		String temp = System.getProperty("username");
		if(temp != null)
		{
			imausername = temp;
		}
		temp = System.getProperty("password");
		if(temp != null)
		{
			imapassword = temp;
		}
		
		temp = System.getProperty("mqusernameA");
		if(temp != null)
		{
			mqusernameA = temp;
		}
		temp = System.getProperty("mqpasswordA");
		if(temp != null)
		{
			mqpasswordB = temp;
		}
		temp = System.getProperty("mqusernameB");
		if(temp != null)
		{
			mqusernameB = temp;
		}
		temp = System.getProperty("mqpasswordB");
		if(temp != null)
		{
			mqpasswordB= temp;
		}
		temp = System.getProperty("mqLocationA");
		if(temp != null)
		{
			mqlocationA = temp;
		}
		temp = System.getProperty("mqLocationB");
		if(temp != null)
		{
			mqlocationB = temp;
		}
		temp = System.getProperty("clientTrace");
		if(temp != null)
		{
			clientTrace = new Boolean(temp);
		}
		
		// Now go and set up an MQ QMGR with channel auth on
			
		mqConfigA = new MQConfig(mqipaddressA, mqusernameA, mqpasswordA, mqlocationA);
		mqConfigB = new MQConfig(mqipaddressB, mqusernameB, mqpasswordB, mqlocationB);
		imaConfig = new ImaConfig(imaipaddress, imausername, imapassword);
		imaMqConfig = new ImaMQConnectivity(imaipaddress, imausername, imapassword);
		imaServer = new ImaServerStatus(imaipaddress, imausername, imapassword);
		
		try
		{
			if(Trace.traceOn())
			{
				Trace.trace("Cleaning up any old configuration for MQ");
			}
			mqConfigA.connectToServer();
			mqConfigB.connectToServer();
			
			// See if QMGR exists
			if(mqConfigA.qmgrExists(qmgrA))
			{
				// try to delete
				mqConfigA.deleteQmgr(qmgrA);
			}
			// See if QMGR exists
			if(mqConfigB.qmgrExists(qmgrB))
			{
				// try to delete
				mqConfigB.deleteQmgr(qmgrB);
			}
			if(Trace.traceOn())
			{
				Trace.trace("Creating new MQ setup - this may take some time");
			}
			// Now create the mq config objects
			mqConfigA.createQmgr(qmgrA);
			mqConfigA.startQmgr(qmgrA);
			mqConfigA.createIMSChannel(channelName, mquser, qmgrA);
			mqConfigA.createListener(listenerName, mqtcpportA, qmgrA);
			mqConfigA.startListener(listenerName, qmgrA);
			mqConfigA.configureMQChannelAuth(channelName, mquser, qmgrA);
			mqConfigA.createIMSChannel(clientChannelName, mquser, qmgrA);
			mqConfigA.configureMQChannelAuth(clientChannelName, mquser, qmgrA);
			mqConfigA.createQueueLocal(mqQueueIn, qmgrA);
			mqConfigA.createQueueLocal(mqQueueOut, qmgrA);
			mqConfigA.setAuthRecordQueue(mqQueueIn, mquser, qmgrA);
			mqConfigA.setAuthRecordQueue(mqQueueOut, mquser, qmgrA);
			
			mqConfigB.createQmgr(qmgrB);
			mqConfigB.startQmgr(qmgrB);
			mqConfigB.createIMSChannel(channelName, mquser, qmgrB);
			mqConfigB.createListener(listenerName, mqtcpportB, qmgrB);
			mqConfigB.startListener(listenerName, qmgrB);
			mqConfigB.configureMQChannelAuth(channelName, mquser, qmgrB);
			mqConfigB.createIMSChannel(clientChannelName, mquser, qmgrB);
			mqConfigB.configureMQChannelAuth(clientChannelName, mquser, qmgrB);
			mqConfigB.createQueueLocal(mqQueueIn, qmgrB);
			mqConfigB.createQueueLocal(mqQueueOut, qmgrB);
			mqConfigB.setAuthRecordQueue(mqQueueIn, mquser, qmgrB);
			mqConfigB.setAuthRecordQueue(mqQueueOut, mquser, qmgrB);
			
			try
			{
				mqConfigA.disconnectServer();
			}
			catch(Exception e)
			{
				// do nothing
			}
			try
			{
				mqConfigB.disconnectServer();
			}
			catch(Exception e)
			{
				// do nothing
			}
			
			
			hub = prefix + "Hub";
			messagingPolicyQ = prefix + "MPQ";
			messagingPolicyT = prefix + "MPT";
			connectionPolicy = prefix + "CP";
			endpoint = prefix + "EP";
			imaQueueIn = prefix + "IMA_Q";
			
			// Now set up the imaserver config
			
			if(Trace.traceOn())
			{
				Trace.trace("Cleaing up mqconnectivity mapping config");
			}
			if(imaMqConfig != null)
			{
				if(!imaMqConfig.isConnected())
				{
					imaMqConfig.connectToServer();
				}
			
				try
				{
					imaMqConfig.updateMappingRule(prefix+"_out", "Enabled", "False");
					imaMqConfig.deleteMappingRule(prefix+"_out");
					
				}
				catch(Exception e)
				{
				// 	do nothing
				}
				try
				{
					imaMqConfig.updateMappingRule(prefix+"_in", "Enabled", "False");
					imaMqConfig.deleteMappingRule(prefix+"_in");
				
				}
				catch(Exception e)
				{
				// do nothing
				}
			}
			
			imaConfig.connectToServer();
			
			if(Trace.traceOn())
			{
				Trace.trace("Cleaning up any old ima config");
			}
			
			if(imaConfig.queueExists(imaQueueIn))
			{
				imaConfig.deleteQueue(imaQueueIn, true);
			}
			if(imaConfig.endpointExists(endpoint))
			{
				imaConfig.updateEndpoint(endpoint, "Enabled", "False");
				imaConfig.deleteEndpoint(endpoint);
			}
			if(imaConfig.connectionPolicyExists(connectionPolicy))
			{
				imaConfig.deleteConnectionPolicy(connectionPolicy);
			}
			if(imaConfig.messagingPolicyExists(messagingPolicyQ))
			{
				imaConfig.deleteMessagingPolicy(messagingPolicyQ);
			}
			if(imaConfig.messagingPolicyExists(messagingPolicyT))
			{
				imaConfig.deleteMessagingPolicy(messagingPolicyT);
			}
			if(imaConfig.messageHubExists(hub))
			{
				imaConfig.deleteMessageHub(hub);
			}
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating new imaserver configuration");
			}
			imaConfig.createQueue(imaQueueIn, prefix, 1000000);		
			imaConfig.createHub(hub, prefix);
			imaConfig.createConnectionPolicy(connectionPolicy, prefix,TYPE.JMSMQTT);
			imaConfig.createMessagingPolicy(messagingPolicyQ, prefix, "*", TYPE.Queue, TYPE.JMS);
			imaConfig.createMessagingPolicy(messagingPolicyT, prefix, "*", TYPE.Topic, TYPE.MQTT);
			imaConfig.createEndpoint(endpoint, prefix, imaport, TYPE.JMSMQTT, connectionPolicy, messagingPolicyQ+","+messagingPolicyT, hub);

			try
			{
				imaConfig.disconnectServer();
			}
			catch(Exception e)
			{
				// do nothing
			}
			
						
		}
		catch(Exception e)
		{
			e.printStackTrace();
			throw new RuntimeException("We were unable to setup all the required configuration to run this test so exiting");
		}
	}
	
	
	
	/*
	 * This method sets up a number of clients for each recoverability test
	 */
	protected void setupClients() throws Exception
	{
		if(Trace.traceOn())
		{
			Trace.trace("Setting up the clients");
		}
		mqttSubscribers = new ArrayList<MqttSubscriberClient>();
			
		
		jmsProducersMap1 = new ArrayList<JmsProducerClient>();
		jmsProducersMap2 = new ArrayList<JmsProducerClient>();
		
		for(int i=0; i<10; i++) 
		{
			JmsProducerClient prod = new JmsProducerClient(imaipaddress, new Integer(imaport));				
			if(clientTrace)
			{
				prod.setTraceFile("imaproducer" + i);
		
			}
			prod.sendPersistentMessages();
			prod.setMessageInterval(2000);
			prod.setNumberOfMessages(10000000);
			prod.setMessageType( JmsMessage.MESSAGE_TYPE.SERIAL);
			prod.storeSentMsgs();
			prod.setup(null, null, imaQueueIn, JmsMessage.DESTINATION_TYPE.QUEUE);
			jmsProducersMap2.add(prod);
		}
	
		if(!onewayonly)
		{
			String imaconsumerTrace = null;
			for(int i=0; i<2; i++) 
			{
				if(clientTrace)
				{
					imaconsumerTrace = "imaconsumer" + i;
				}
				MqttSubscriberClient sub = new MqttSubscriberClient("tcp://"+imaipaddress+":"+imaport);
				sub.setCleanSession(false);
				sub.setQos(2);
				sub.checkDuplicates();
				sub.setTraceFile(imaconsumerTrace);
				sub.storeReceivedMsgs();
				sub.setup(imaTopicOut);
				mqttSubscribers.add(sub);
				new Thread(sub).start();
				
			}
		
			for(int i=0; i<5; i++) 
			{
				JmsProducerClient prod = new JmsProducerClient(mqipaddressA, new Integer(mqtcpportA), qmgrA, clientChannelName);				
				if(clientTrace)
				{
					prod.setTraceFile("mqproducerA" + i);
			
				}
				prod.sendPersistentMessages();
				
				prod.setMessageInterval(5000);
				prod.setNumberOfMessages(10000000);
				prod.setMessageType( JmsMessage.MESSAGE_TYPE.SERIAL);
				prod.storeSentMsgs();
				prod.setup(mquser, "rubbish", mqQueueIn, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducersMap1.add(prod);
				
			}
			for(int i=0; i<5; i++) 
			{
				JmsProducerClient prod = new JmsProducerClient(mqipaddressB, new Integer(mqtcpportB), qmgrB, clientChannelName);				
				if(clientTrace)
				{
					prod.setTraceFile("mqproducerB" + i);
			
				}
				prod.sendPersistentMessages();
				
				prod.setMessageInterval(5000);
				prod.setNumberOfMessages(10000000);
				prod.setMessageType( JmsMessage.MESSAGE_TYPE.SERIAL);
				prod.storeSentMsgs();
				prod.setup(mquser, "rubbish", mqQueueIn, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducersMap2.add(prod);
				
			
			}
		}
		
		
		for(int i=0; i<jmsProducersMap1.size(); i++)
		{
			new Thread(jmsProducersMap1.get(i)).start();
		}
		for(int i=0; i<jmsProducersMap2.size(); i++)
		{
			new Thread(jmsProducersMap2.get(i)).start();
		}
		if(Trace.traceOn())
		{
			Trace.trace("Producers have started");
		}
	}
	
	
	protected void checkCurDepth(boolean qmgrADown, boolean qmgrBDown) throws Exception
	{
		int qmgrADepth_new = -1;
		int qmgrBDepth_new = -1;
		if(Trace.traceOn())
		{
			Trace.trace("Checking curdepth");
		}
		//Get the curdepth for qmgrA and qmgrB
		if(!qmgrADown && qmgrBDown)
		{
			qmgrADepth_new = mqConfigA.getCurrentQueueDepth(mqQueueOut, qmgrA);
			qmgrBDepth_new = mqConfigB.getCurrentQueueDepth(mqQueueOut, qmgrB);

			if(Trace.traceOn())
			{
				Trace.trace("Old qmgrA curdepth=" + qmgrADepth + ". New qmgrA curdepth=" + qmgrADepth_new);
				Trace.trace("Old qmgrB curdepth=" + qmgrBDepth + ". New qmgrB curdepth=" + qmgrBDepth_new);
			}
			Assert.assertTrue("Old depth for qmgrA was " + qmgrADepth + " and the new depth was " + qmgrADepth_new, qmgrADepth_new > qmgrADepth);
			Assert.assertTrue("Old depth for qmgrB was " + qmgrBDepth + " and the new depth was " + qmgrBDepth_new, qmgrBDepth_new == qmgrBDepth);
		}
		else if(!qmgrADown && !qmgrBDown)
		{
			qmgrADepth_new = mqConfigA.getCurrentQueueDepth(mqQueueOut, qmgrA);
			qmgrBDepth_new = mqConfigB.getCurrentQueueDepth(mqQueueOut, qmgrB);
			if(Trace.traceOn())
			{
				Trace.trace("Old qmgrA curdepth=" + qmgrADepth + ". New qmgrA curdepth=" + qmgrADepth_new);
				Trace.trace("Old qmgrB curdepth=" + qmgrBDepth + ". New qmgrB curdepth=" + qmgrBDepth_new);
			}
			Assert.assertTrue("Old depth for qmgrA was " + qmgrADepth + " and the new depth was " + qmgrADepth_new, qmgrADepth_new > qmgrADepth);
			Assert.assertTrue("Old depth for qmgrB was " + qmgrBDepth + " and the new depth was " + qmgrBDepth_new, qmgrBDepth_new > qmgrBDepth);
		}
		else if(qmgrADown && !qmgrBDown)
		{
			qmgrADepth_new = mqConfigA.getCurrentQueueDepth(mqQueueOut, qmgrA);
			qmgrBDepth_new = mqConfigB.getCurrentQueueDepth(mqQueueOut, qmgrB);
			if(Trace.traceOn())
			{
				Trace.trace("Old qmgrA curdepth=" + qmgrADepth + ". New qmgrA curdepth=" + qmgrADepth_new);
				Trace.trace("Old qmgrB curdepth=" + qmgrBDepth + ". New qmgrB curdepth=" + qmgrBDepth_new);
			}
			Assert.assertTrue("Old depth for qmgrA was " + qmgrADepth + " and the new depth was " + qmgrADepth_new, qmgrADepth_new == qmgrADepth);
			Assert.assertTrue("Old depth for qmgrB was " + qmgrBDepth + " and the new depth was " + qmgrBDepth_new, qmgrBDepth_new > qmgrBDepth);
		}
		else if(qmgrADown && qmgrBDown)
		{
			qmgrADepth_new = mqConfigA.getCurrentQueueDepth(mqQueueOut, qmgrA);
			qmgrBDepth_new = mqConfigB.getCurrentQueueDepth(mqQueueOut, qmgrB);
			if(Trace.traceOn())
			{
				Trace.trace("Old qmgrA curdepth=" + qmgrADepth + ". New qmgrA curdepth=" + qmgrADepth_new);
				Trace.trace("Old qmgrB curdepth=" + qmgrBDepth + ". New qmgrB curdepth=" + qmgrBDepth_new);
			}
			Assert.assertTrue("Old depth for qmgrA was " + qmgrADepth + " and the new depth was " + qmgrADepth_new, qmgrADepth_new == qmgrADepth);
			Assert.assertTrue("Old depth for qmgrB was " + qmgrBDepth + " and the new depth was " + qmgrBDepth_new, qmgrBDepth_new == qmgrBDepth);
		}
		
		
		qmgrADepth = qmgrADepth_new;
		qmgrBDepth = qmgrBDepth_new;
		
	}
	
	/**
	 * This method tears down a number of clients for each recoverability test
	 * and calls the method to check that we did get all the expected number of 
	 * messages we sent
	 */
	protected void teardownClients() throws Exception
	{
		 if(Trace.traceOn())
		    {
		    	Trace.trace("We have finished so closing all producers");
			}
		    for(int i=0; i<jmsProducersMap1.size(); i++)
			{
		    	jmsProducersMap1.get(i).keepRunning = false;				
		    }
		    for(int i=0; i<jmsProducersMap2.size(); i++)
			{
		    	jmsProducersMap2.get(i).keepRunning = false;				
		    }
			   
			if(Trace.traceOn())
			{
				Trace.trace("We will sleep for " + consumerWait + " milliseconds to give all the consumers a chance to catch up");
			}
			try 
			{
			   	Thread.sleep(consumerWait);   
			} 
			catch(Exception e)
			{
			  	// do nothing
			}
			for(int i=0; i<mqttSubscribers.size(); i++)
			{
			   	mqttSubscribers.get(i).keepRunning = false;
			}
			// Now check we got all the expected messages
			if(!onewayonly)
			{
				if(Trace.traceOn())
				{
					Trace.trace("Checking messages for mapping 1: MQ queue to IMA topic");
				}
				this.checkSentMessages();
			}
			
			qmgrBDepth = mqConfigB.getCurrentQueueDepth(mqQueueOut, qmgrB);
			qmgrADepth = mqConfigA.getCurrentQueueDepth(mqQueueOut, qmgrA);
			ArrayList<String> allMsgs = new ArrayList<String>();
			
			for(int i=0; i<jmsProducersMap2.size(); i++)
			{
				allMsgs.addAll(jmsProducersMap2.get(i).getSentMsgs());
			}
			if(Trace.traceOn())
			{
				Trace.trace("New qmgrA curdepth=" + qmgrADepth);
				Trace.trace("New qmgrB curdepth=" + qmgrBDepth);
				Trace.trace("Sent messages="+allMsgs.size());
			}
			Assert.assertTrue("The depth for qmgrA was " + qmgrADepth + ". The depth for qmgrB was " + qmgrBDepth + ". In total they should add up to the mapsentSize which was " + allMsgs.size(), allMsgs.size() == (qmgrADepth + qmgrBDepth));
			
			if(!imaConfig.isConnected())
			{
				imaConfig.connectToServer();
			}
			
			for(int i=0; i<mqttSubscribers.size(); i++)
			{
				try
				{
					imaConfig.deleteMqttClient(mqttSubscribers.get(i).getClientId());
				}
				catch(Exception e)
				{
				}
			}
			
			try
			{
				imaConfig.disconnectServer();
			}
			catch(Exception e)
			{
				// do nothing
			}
			
	}
	
	/**
	 * After each test we need to check if we are left with any outstanding mqtt clients that need to be 
	 * deleted from the imaserver
	 */
	protected void tearDown() throws Exception {
	
		if(imaConfig != null)
		{
			
			if(!imaConfig.isConnected())
			{
				try
				{
					imaConfig.connectToServer();
				}
				catch(Exception e)
				{
					// do nothing
				}
			}
			for(int i=0; i<mqttSubscribers.size(); i++)
			{
				mqttSubscribers.get(i).keepRunning = false;
			}
			for(int i=0; i<jmsProducersMap1.size(); i++)
			{
				jmsProducersMap1.get(i).keepRunning = false;
			}
			for(int i=0; i<jmsProducersMap2.size(); i++)
			{
				jmsProducersMap2.get(i).keepRunning = false;
			}
			// do some clearup depending on when we crashed
			for(int i=0; i<mqttSubscribers.size(); i++)
			{
				
				String clientId = mqttSubscribers.get(i).getClientId();
				try
				{
					imaConfig.deleteSubscription(imaTopicOut, clientId);
				}
				catch(Exception ex)
				{
				//	do nothing
				}
				try
				{
					imaConfig.deleteMqttClient(clientId);
				}
				catch(Exception ex)
				{
					
				}
			}
			try
			{
				imaConfig.disconnectServer();
			}
			catch(Exception e)
			{
				// do nothing
			}
		}
	
	}

	/**
	 * This performs the full tear down for the completed test, removing all mq and ima config.
	 * @throws Exception
	 */
	public static void tidyUp() throws Exception {
		
		if(Trace.traceOn())
		{
			Trace.trace("Tearing down the mq configuration");
		}
		try
		{
			if(imaServer != null)
			{
				imaServer.disconnectServer();
			}
		}
		catch(Exception e)
		{
			
		}
		
		if(Trace.traceOn())
		{
			Trace.trace("Deleting mq config");
		}
		if(mqConfigA != null)
		{
			if(!mqConfigA.isConnected())
			{
				mqConfigA.connectToServer();
			}
			try
			{
				mqConfigA.stopQmgr(qmgrA);
			}
			catch(Exception e)
			{
				
			}
			try
			{
				mqConfigA.deleteQmgr(qmgrA);
			}
			catch(Exception e)
			{
				
			}
		}
		if(mqConfigB != null)
		{
			if(!mqConfigB.isConnected())
			{
				mqConfigB.connectToServer();
			}
			try
			{
				mqConfigB.stopQmgr(qmgrB);
			}
			catch(Exception e)
			{
				
			}
			try
			{
				mqConfigB.deleteQmgr(qmgrB);
			}
			catch(Exception e)
			{
				
			}
		}	
		
		
		if(Trace.traceOn())
		{
			Trace.trace("Deleting ima config");
		}
		
		
		if(imaConfig != null)
		{
			if(!imaConfig.isConnected())
			{
				imaConfig.connectToServer();
			}
			try
			{	imaConfig.updateEndpoint(endpoint, "Enabled", "False");
				imaConfig.deleteEndpoint(endpoint);
			}
			catch(Exception e)
			{
			
			}
			try
			{
				imaConfig.deleteConnectionPolicy(connectionPolicy);
			}
			catch(Exception e)
			{
			
			}
			try
			{
				imaConfig.deleteMessagingPolicy(messagingPolicyQ);
			}
			catch(Exception e)
			{
				
			}
			try
			{
				imaConfig.deleteMessagingPolicy(messagingPolicyT);
			}
			catch(Exception e)
			{
				
			}
			try
			{
				imaConfig.deleteMessageHub(hub);
			}
			catch(Exception e)
			{
				
			}
			try
			{
				imaConfig.deleteQueue(imaQueueIn, true);
			}
			catch(Exception e)
			{
			// 	do nothing
			}
							
		}
		
	
		try
		{
			imaConfig.disconnectServer();
		}
		catch(Exception e)
		{
			
		}
		try
		{
			mqConfigA.disconnectServer();
		}
		catch(Exception e)
		{
			
		}
		try
		{
			mqConfigB.disconnectServer();
		}
		catch(Exception e)
		{
			
		}
		
		
	}

	
	
	
	/*
	 * This method will determine if the messages we sent were actually received by the consumers
	 * @param sent
	 * @param received
	 * @throws Exception
	 */
	protected void checkSentMessages() throws Exception
	{
		if(Trace.traceOn())
		{
			Trace.trace("Checking the messages for mapping 1");
		}
		
		ArrayList<String> allMsgs = new ArrayList<String>();
		
		for(int i=0; i<jmsProducersMap1.size(); i++)
		{
			allMsgs.addAll(jmsProducersMap1.get(i).getSentMsgs());
		}
		
		if(Trace.traceOn())
		{
			Trace.trace("The producers sent " + allMsgs.size());
		}
		
		for(int i=0; i<mqttSubscribers.size(); i++)
		{
			
			ArrayList<String> receivedMsgs = mqttSubscribers.get(i).getReceivedMsgs();
			if(Trace.traceOn())
			{
				Trace.trace("Client " + mqttSubscribers.get(i).getClientId() + " received " + receivedMsgs.size() + " messages" );
			}
			if(receivedMsgs.containsAll(allMsgs))
			{
				if(Trace.traceOn())
				{
					Trace.trace("The consumer " + mqttSubscribers.get(i).getClientId() + " received all the messages from the producers");
				}
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("The consumer " + mqttSubscribers.get(i).getClientId() + " did not receive all the sent messages");
					
					for(int j=0; j<receivedMsgs.size(); j++)
					{
						Trace.trace("Received msg=" + receivedMsgs.get(j));
					}
					for(int j=0; j<allMsgs.size(); j++)
					{
						Trace.trace("Producer sent msg=" + allMsgs.get(j));
					}
				}
				
				Assert.fail("The consumer " + mqttSubscribers.get(i).getClientId() + " did not receive all the sent messages");
			}
		}
	
	}
	
	
	protected void checkForStackDumps() throws Exception
	{
		if(!imaServer.isConnected())
		{
			
	  		imaServer.connectToServer();
		}
		if(dumpFiles == null)
		{
			if(Trace.traceOn())
			{
				Trace.trace("Getting the stack files on the system for future reference");
			}
			dumpFiles = imaServer.getFileNames();
			if(Trace.traceOn())
			{
				Trace.trace("The files on the current system are:");
			}
			for(int i=0; i<dumpFiles.length; i++)
			{
				if(Trace.traceOn())
				{
					Trace.trace(dumpFiles[i]);
				}
			}
		}
		else
		{
			String[] newFiles = imaServer.getFileNames();
			if(Trace.traceOn())
			{
				Trace.trace("The files on the current system are:");
			}
			for(int i=0; i<newFiles.length; i++)
			{
				if(Trace.traceOn())
				{
					Trace.trace(newFiles[i]);
				}
			}
			
			Assert.assertTrue("Checking the number of stack files on the system has not changed", dumpFiles.length == newFiles.length);
			
			// ok now log the newFiles in case this class wants to be called again
			dumpFiles = newFiles;
			
		}
		
	}
	
	
}
