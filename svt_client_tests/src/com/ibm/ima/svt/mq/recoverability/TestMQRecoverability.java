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
 * This test will issue a number of persistent messages to and from MQ whilst performing a number
 * of outages on MQ or the imaserver. The intention is to ensure that MQ can recover successfully
 * once those outages are resumed and also that no messages get lost in the system.
 * 
 * The outages are as following:-
 * 1. Killing the imaserver process using a force stop
 * 2. Killing the mqconnectivity process using a force stop 
 * 3. Bringing the qmgr down on mq
 * 4. Stopping the mqchannel on mq
 * 5. Disabling the mappings
 * 6. Stopping and starting both mqconnectivity and imaserver
 * 
 * 
 *
 */
public class TestMQRecoverability extends TestCase {
	
	protected static String prefix = "TestMQRecoverability";
	protected static String imaipaddress = null;
	protected static String imausername = "admin";
	protected static String imapassword = "admin";
	protected static String mqipaddress = null;
	protected static String mqusername = "mquser";
	protected static String mqpassword = null;
	protected static int mqtcpport = 1435;
	protected static int mqMqttPort = 1895;
	protected static String mqlocation ="/opt/mqm/bin";
	protected static String mquser = "root"; // This needs to be a user that is not in the mqm group 
	protected static String imaport = "16310";
	protected static boolean clientTrace = false; // This can be set at runtime to trace each producer and consumer
	
	private static String qmgr = "SVTMQCON";
	private static String listenerName = "IMA_LISTENER";
	private static String channelName = "IMA_MQCON";
	private static String clientChannelName = "IMS_CLIENTS";
	
	private static String mqQueueIn = "TESTMQ_Q";
	private static String mqTopicOut = "TESTMQ_T";
	private static String imaQueueIn = prefix + "IMA_Q";
	private static String imaTopicOut = prefix + "IMA_T";
	
	private static String hub = null;
	private static String endpoint = null;
	private static String connectionPolicy = null;
	private static String messagingPolicyQ = null;
	private static String messagingPolicyT = null;
	
	private static MQConfig mqConfig = null;
	private static ImaConfig imaConfig = null;
	private static ImaMQConnectivity imaMqConfig = null;
	private static ImaServerStatus imaServer = null;
	
	private ArrayList<JmsProducerClient> jmsProducersMap1 = new ArrayList<JmsProducerClient>();
	private ArrayList<JmsProducerClient> jmsProducersMap2 = new ArrayList<JmsProducerClient>();
	private ArrayList<MqttSubscriberClient> mqttSubscribersMap1 = new ArrayList<MqttSubscriberClient>();
	private ArrayList<MqttSubscriberClient> mqttSubscribersMap2 = new ArrayList<MqttSubscriberClient>();
	private int consumerWait = 60000; // This is the time we will wait for the consumers to catch up once producers have been stopped
	private int intervalTime = 60000; // This is the time inbetween recoverability actions we will wait before performing the next action
	
	private int recoverabilityMode = 0;

	private String[] dumpFiles = null;
	private static boolean testAutomation = false;
	private static int resultCount = 0;
	private boolean firstSetup = true;
	private int methodCount = 0;
	
	/**
	 * This method is called if executing from the automation
	 * @param args
	 */
	public static void main(String args[]) {
		
		if(args.length != 3)
		{
			
			System.out.println("Please pass in the ipaddress of the ima server, mqserver and mqlocation");
		}
		else
		{
			imaipaddress = args[0];
			mqipaddress = args[1];
			mqlocation = args[2];
			testAutomation = true;
		}
		
		org.junit.runner.JUnitCore.main("com.ibm.ima.svt.mq.recoverability.TestMQRecoverability");
	}

	public void initialSetup() throws Exception {
		
		new Trace("stdout", true);
		// Set up an MQ queue manager to perform the testing
		if(!testAutomation)
		{
			imaipaddress = System.getProperty("ipaddress");
			mqipaddress = System.getProperty("mqipaddress");
			mqpassword  = System.getProperty("mqpassword");
		}
		
		if(imaipaddress == null)
		{
			System.out.println("Please pass in as system parameters the 'ipaddress' of the server. For example: -Dipaddress=9.3.177.15");
			System.exit(0);
		}
		
		
		
		if(mqipaddress == null)
		{
			System.out.println("Please pass in as system parameters the 'mqipaddress' of the server. For example: -Dmqipaddress=9.3.179.191");
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
		
		temp = System.getProperty("mqusername");
		if(temp != null)
		{
			mqusername = temp;
		}
		if(mqpassword == null)
		{
			temp = System.getProperty("mqpassword");
			if(temp != null)
			{
				mqpassword = temp;
			}
		}
		temp = System.getProperty("mqLocation");
		if(temp != null)
		{
			mqlocation = temp;
		}
		temp = System.getProperty("clientTrace");
		if(temp != null)
		{
			clientTrace = new Boolean(temp);
		}
		
		// Now go and set up an MQ QMGR with channel auth on
		mqConfig = new MQConfig(mqipaddress, mqusername, mqpassword, mqlocation);
		imaConfig = new ImaConfig(imaipaddress, imausername, imapassword);
		imaMqConfig = new ImaMQConnectivity(imaipaddress, imausername, imapassword);
		imaServer = new ImaServerStatus(imaipaddress, imausername, imapassword);
		
		try
		{
			if(Trace.traceOn())
			{
				Trace.trace("Cleaning up any old configuration for MQ");
			}
			mqConfig.connectToServer();
			
			// See if QMGR exists
			if(mqConfig.qmgrExists(qmgr))
			{
				// try to delete
				mqConfig.deleteQmgr(qmgr);
			}
			if(Trace.traceOn())
			{
				Trace.trace("Creating new MQ setup - this may take some time");
			}
			// Now create the mq config objects
			mqConfig.createQmgr(qmgr);
			mqConfig.startQmgr(qmgr);
			mqConfig.createIMSChannel(channelName, mquser, qmgr);
			mqConfig.createListener(listenerName, mqtcpport, qmgr);
			mqConfig.startListener(listenerName, qmgr);
			mqConfig.configureMQChannelAuth(channelName, mquser, qmgr);
			mqConfig.createIMSChannel(clientChannelName, mquser, qmgr);
			mqConfig.configureMQChannelAuth(clientChannelName, mquser, qmgr);
			mqConfig.createQueueLocal(mqQueueIn, qmgr);
			mqConfig.createTopic(mqTopicOut, mqTopicOut, qmgr);
			mqConfig.setAuthRecordQueue(mqQueueIn, mquser, qmgr);
			mqConfig.setAuthRecordTopic(mqTopicOut, mquser, qmgr);
			mqConfig.createMqttService(mqMqttPort, qmgr);
			
			try
			{
				mqConfig.disconnectServer();
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
			
			// Now set up the imaserver config
			imaMqConfig.connectToServer();
			imaConfig.connectToServer();
			
			if(Trace.traceOn())
			{
				Trace.trace("Cleaning up any old configuration for imaserver");
			}
			if(imaMqConfig.mappingRuleExists(prefix+"_out"))
			{
				imaMqConfig.updateMappingRule(prefix+"_out", "Enabled", "False");
				imaMqConfig.deleteMappingRule(prefix+"_out");
				
			}
			if(imaMqConfig.mappingRuleExists(prefix+"_in"))
			{
				imaMqConfig.updateMappingRule(prefix+"_in", "Enabled", "False");
				imaMqConfig.deleteMappingRule(prefix+"_in");
				
			}
			if(imaMqConfig.queueManagerConnectionExists(qmgr))
			{
				imaMqConfig.deleteQueueManagerConnection(qmgr);
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
					
		
			imaMqConfig.createQueueManagerConnection(qmgr, qmgr, mqipaddress+"("+mqtcpport+")", channelName);
			imaMqConfig.createMappingRule(prefix+"_out", qmgr, "3", mqQueueIn, imaTopicOut);
			imaMqConfig.createMappingRule(prefix+"_in", qmgr, "11", imaQueueIn, mqTopicOut);
			
			try
			{
				imaMqConfig.disconnectServer();
			}
			catch(Exception e)
			{
				// do nothing
			}
			
			try
			{
				imaConfig.disconnectServer();
			}
			catch(Exception e)
			{
				// do nothing
			}
			if(Trace.traceOn())
			{
				Trace.trace("GLOBAL SETUP COMPLETE");
			}
						
		}
		catch(Exception e)
		{
			e.printStackTrace();
			throw new Exception("We were unable to setup all the required configuration to run this test so exiting");
		}
	}
	
	/**
	 * We need to make sure they everything is back to normal before starting the next test, e.g. the processes are running,
	 * the queuemanager and all listeners are running, the queues are cleared and mappings enabled etc. This is because one of the
	 * others tests might have failed and left everything in a bad way.
	 * @throws Exception
	 */
	protected void setUp() throws Exception {
	
		try
		{
			
			if(firstSetup)
			{
				firstSetup=false;
				initialSetup();
				
			}
			if(!imaServer.isConnected())
			{
				imaServer.connectToServer();
			}			
			// check that processes are up
			if(imaServer.isMQConnectivityStopped())
			{
				imaServer.startMQConnectivity();
			}
			if(imaServer.isServerStopped())
			{
				imaServer.startServer();
			}
			imaServer.disconnectServer();
			

			if(!imaMqConfig.isConnected())
			{
				imaMqConfig.connectToServer();
			}
			imaMqConfig.updateMappingRule(prefix+"_out", "Enabled", "False");
			imaMqConfig.updateMappingRule(prefix+"_in", "Enabled", "False");
			
			
			
			if(!mqConfig.isConnected())
			{
				mqConfig.connectToServer();
			}
			if(mqConfig.isQmgrRunning(qmgr))
			{
				// Stop and start the qmgr so all channels and listeners come back up
				mqConfig.stopQmgr(qmgr);
			}
			mqConfig.startQmgr(qmgr);
			if(mqConfig.qlocalExists(mqQueueIn, qmgr))
			{
				mqConfig.deleteQueue(mqQueueIn, qmgr);
			}
			mqConfig.createQueueLocal(mqQueueIn, qmgr);
			mqConfig.setAuthRecordQueue(mqQueueIn, mquser, qmgr);
			mqConfig.disconnectServer();
			
			
			if(!imaConfig.isConnected())
			{
				imaConfig.connectToServer();
			}
			if(imaConfig.queueExists(imaQueueIn))
			{
				imaConfig.deleteQueue(imaQueueIn, true);
			}
			imaConfig.createQueue(imaQueueIn, prefix, 1000000);
			imaConfig.disconnectServer();
			
			imaMqConfig.updateMappingRule(prefix+"_out", "Enabled", "True");
			imaMqConfig.updateMappingRule(prefix+"_in", "Enabled", "True");
			imaMqConfig.disconnectServer();
			
			
			if(Trace.traceOn())
			{
				Trace.trace("TEST SETUP COMPLETE");
			}
			
		}
		catch(Exception e)
		{
			e.printStackTrace();
			Assert.fail("We were unable to setup for the next test");
		}
		// We have to make sure everything is back up in case one of the previous tests failed.

		
	
	}
	
	

	public void testMQRecoverability_forceImaServerStop()
	{
		recoverabilityMode = 1;
		if(Trace.traceOn())
		{
			Trace.trace("TEST: forceImaServerStop");
		}
		startTest("forceImaServerStop");
	}
	
	
	

	public void testMQRecoverability_forceMQConnectivityStop()
	{
		recoverabilityMode = 2;
		if(Trace.traceOn())
		{
			Trace.trace("TEST: forceMQConnectivityStop");
		}
		
		startTest("forceMQConnectivityStop");
		
	}
	
		public void testMQRecoverability_stopQMGR()
	{
		recoverabilityMode = 3;
		if(Trace.traceOn())
		{
			Trace.trace("TEST: stopQMGR");
		}
		startTest("stopQMGR");
		
	}
	
	
	public void testMQRecoverability_stopChannel()
	{
		recoverabilityMode = 4;
		if(Trace.traceOn())
		{
			Trace.trace("TEST: stopChannel");
		}
		startTest("stopChannel");
		
	}
	
	

	public void testMQRecoverability_disableMappings()
	{
		recoverabilityMode = 5;
		if(Trace.traceOn())
		{
			Trace.trace("TEST: disableMappings");
		}
		startTest("disableMappings");
		
	}
	

	public void testMQRecoverability_stopStartMQAndServer()
	{
		recoverabilityMode = 6;
		if(Trace.traceOn())
		{
			Trace.trace("TEST: stopStartMQAndServer");
		}
		startTest("stopStartMQAndServer");
		
	}
	
	/**
	 * This is the overriding test that performs all the necessary recoverability
	 * @param methodName
	 */
	private void startTest(String methodName)
	{
		try
		{
			
			
			// first check to see what files we have on the server
			checkForStackDumps();
			
		
			if(Trace.traceOn())
		    {
		    	Trace.trace("Set up consumers and producers for recoverability test");
			}
			setupClients(methodName);					
		
			Thread.sleep(intervalTime * 2);
			
			switch (recoverabilityMode) {
			  case 0: 
				throw new Exception("Recoverability test not recognised");
			    
			  case 1: 
			  {
				  	if(!imaServer.isConnected())
					{
				  		imaServer.connectToServer();
					}
				  	imaServer.stopForceServer();
					if(!imaServer.isServerStopped())
					{
						throw new Exception("We were unable to stop the imaserver - exiting the test");
					}
					if(Trace.traceOn())
				    {
				    	Trace.trace("The imaserver was stopped");
					}
					
					Thread.sleep(intervalTime);
					
					imaServer.startServer(120000);
					if(!imaServer.isServerRunning())
					{
						throw new Exception("We were unable to start the imaserver - exiting the test");
					}
					if(Trace.traceOn())
				    {
				    	Trace.trace("The imaserver was restarted");
					}
					try
					{
						imaServer.disconnectServer();
					}
					catch(Exception e)
					{
						// do nothing
					}
			  }
			    break;
			  case 2: 
			  {
					if(!imaServer.isConnected())
					{
				  		imaServer.connectToServer();
					}
					imaServer.stopForceMQConnectivity();
					if(!imaServer.isMQConnectivityStopped())
					{
						throw new Exception("We were unable to stop mqconnectivity - exiting the test");
					}
					if(Trace.traceOn())
				    {
				    	Trace.trace("Mqconnectivity was stopped");
					}
					
					Thread.sleep(intervalTime);
					
					imaServer.startMQConnectivity();
					if(!imaServer.isMQConnectivityRunning())
					{
						throw new Exception("We were unable to start mqconnectivity - exiting the test");
					}
					if(Trace.traceOn())
				    {
				    	Trace.trace("mqconnectivity was restarted");
					}
					try
					{
						imaServer.disconnectServer();
					}
					catch(Exception e)
					{
						// do nothing
					}
			  }
			    break;
			  case 3: 
			  {
				    if(!mqConfig.isConnected())
				  
					{
						mqConfig.connectToServer();
					}
				  	mqConfig.stopQmgr(qmgr);
				  	if(!mqConfig.isQmgrStopped(qmgr))
				  	{
						throw new Exception("We were unable to stop the qmgr - exiting the test");
					}
					if(Trace.traceOn())
				    {
				    	Trace.trace("The qmgr was stopped");
					}
					Thread.sleep(intervalTime);
					
					mqConfig.startQmgr(qmgr);
					if(!mqConfig.isQmgrRunning(qmgr))
					{
						throw new Exception("We were unable to start the qmgr - exiting the test");
					}
					if(Trace.traceOn())
				    {
				    	Trace.trace("The qmgr was restarted");
					}
					try
					{
						mqConfig.disconnectServer();
					}
					catch(Exception e)
					{
						// do nothing
					}
			  }
			    break;
			  case 4: 
			  {
					if(!mqConfig.isConnected())
					{
						mqConfig.connectToServer();
					}
				  	mqConfig.stopChannel(channelName, qmgr);
				  	if(mqConfig.isChannelActive(channelName, qmgr))
				  	{
						throw new Exception("We were unable to stop the channel - exiting the test");
					}
					if(Trace.traceOn())
				    {
				    	Trace.trace("The channel was stopped");
					}
					Thread.sleep(intervalTime);
					
					mqConfig.startChannel(channelName, qmgr);
					if(Trace.traceOn())
				    {
				    	Trace.trace("The channel was restarted");
					}
					try
					{
						mqConfig.disconnectServer();
					}
					catch(Exception e)
					{
						// do nothing
					}
			  }
			    break;
			  case 5:
			  {
					if(!imaMqConfig.isConnected())
					{
						imaMqConfig.connectToServer();
					}
					imaMqConfig.updateMappingRule(prefix+"_out", "Enabled", "False");
					imaMqConfig.updateMappingRule(prefix+"_in", "Enabled", "False");
				  
					if(Trace.traceOn())
				    {
				    	Trace.trace("The mappings were disabled");
					}
					Thread.sleep(intervalTime);
					
					imaMqConfig.updateMappingRule(prefix+"_out", "Enabled", "True");
					imaMqConfig.updateMappingRule(prefix+"_in", "Enabled", "True");
					if(Trace.traceOn())
				    {
				    	Trace.trace("The mappings were enabled");
					}
					try
					{
						imaMqConfig.disconnectServer();
					}
					catch(Exception e)
					{
						// do nothing
					}
			  }
			    break;
			  case 6: 
			  {
				  	if(!imaServer.isConnected())
					{
				  		imaServer.connectToServer();
					}
					imaServer.stopMQConnectivity();
					if(!imaServer.isMQConnectivityStopped())
					{
						throw new Exception("We were unable to stop mqconnectivity - exiting the test");
					}
					imaServer.stopServer();
					if(!imaServer.isServerStopped())
					{
						throw new Exception("We were unable to stop server - exiting the test");
					}
					if(Trace.traceOn())
				    {
				    	Trace.trace("Server and mqconnectivity was stopped");
					}
					
					Thread.sleep(intervalTime);
					
					imaServer.startMQConnectivity();
					if(!imaServer.isMQConnectivityRunning())
					{
						throw new Exception("We were unable to start mqconnectivity - exiting the test");
					}
					imaServer.startServer(360000);
					
					if(!imaServer.isServerRunning())
					{
						throw new Exception("We were unable to start server - exiting the test");
					}
					if(Trace.traceOn())
				    {
				    	Trace.trace("Server and mqconnectivity was restarted");
					}
					try
					{
						imaServer.disconnectServer();
					}
					catch(Exception e)
					{
						// do nothing
					}
			  }
			    break;
			  default: 
			    throw new Exception("Recoverability test not recognised");
			}
			
			
			Thread.sleep(intervalTime * 2);
			if(Trace.traceOn())
		    {
		    	Trace.trace("Tear down consumers and producers for recoverability test");
			}			
			teardownClients();
		   
			// Now check to see if we have any stack files produced whilst we ran this test
			checkForStackDumps();
			if(Trace.traceOn())
			{
				Trace.trace("TEST COMPLETED");
			}
			resultCount++;
			Trace.trace("Test pass count: "+resultCount);
			if(resultCount == 6)
			{
				Trace.trace("TEST SUCCESS");
			}
			
		}
		catch (AssertionError ea)
		{
			Assert.fail(ea.getMessage());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			if(Trace.traceOn())
			{
				Trace.trace("An exception occurred running this test" + e);
			}
			Assert.fail("An exception occurred running this test" + e);
		}
		
}
	
	

	/**
	 * After each test we need to check if we are left with any outstanding mqtt clients that need to be 
	 * deleted from the imaserver
	 */
	protected void tearDown() throws Exception {
	
		methodCount++;
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
			for(int i=0; i<mqttSubscribersMap1.size(); i++)
			{
				mqttSubscribersMap1.get(i).keepRunning = false;
			}
			for(int i=0; i<jmsProducersMap1.size(); i++)
			{
				jmsProducersMap1.get(i).keepRunning = false;
			}
			// do some clearup depending on when we crashed
			for(int i=0; i<mqttSubscribersMap1.size(); i++)
			{
				
				String clientId = mqttSubscribersMap1.get(i).getClientId();
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
			for(int i=0; i<mqttSubscribersMap2.size(); i++)
			{
				mqttSubscribersMap2.get(i).keepRunning = false;
			}
			for(int i=0; i<jmsProducersMap2.size(); i++)
			{
				jmsProducersMap2.get(i).keepRunning = false;
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
		if(methodCount == 5)
		{
			finalTearDown();
		}
	
	}

	/**
	 * This performs the full tear down for the completed test, removing all mq and ima config.
	 * @throws Exception
	 */
	public void finalTearDown() throws Exception {
		
		if(Trace.traceOn())
		{
			Trace.trace("Tearing down the configuration");
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
		if(mqConfig != null)
		{
			if(!mqConfig.isConnected())
			{
				mqConfig.connectToServer();
			}
			try
			{
				mqConfig.deleteQmgr(qmgr);
			}
			catch(Exception e)
			{
				e.printStackTrace();
			}
		}	
		
		
		if(Trace.traceOn())
		{
			Trace.trace("Deleting ima config");
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
			try
			{
				imaMqConfig.deleteQueueManagerConnection(qmgr);
			}
			catch(Exception e)
			{
			// 	do nothing
			}
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
			imaMqConfig.disconnectServer();
		}
		catch(Exception e)
		{
			
		}
		try
		{
			mqConfig.disconnectServer();
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
	private void checkSentMessages() throws Exception
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
		
		for(int i=0; i<mqttSubscribersMap1.size(); i++)
		{
			
			ArrayList<String> receivedMsgs = mqttSubscribersMap1.get(i).getReceivedMsgs();
			if(Trace.traceOn())
			{
				Trace.trace("Client " + mqttSubscribersMap1.get(i).getClientId() + " received " + receivedMsgs.size() + " messages" );
			}
			if(receivedMsgs.containsAll(allMsgs))
			{
				if(Trace.traceOn())
				{
					Trace.trace("The consumer " + mqttSubscribersMap1.get(i).getClientId() + " received all the messages from the producers");
				}
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("The consumer " + mqttSubscribersMap1.get(i).getClientId() + " did not receive all the sent messages");
					
					for(int j=0; j<receivedMsgs.size(); j++)
					{
						Trace.trace("Received msg=" + receivedMsgs.get(j));
					}
					for(int j=0; j<allMsgs.size(); j++)
					{
						Trace.trace("Producer sent msg=" + allMsgs.get(j));
					}
				}
				
				Assert.fail("The consumer " + mqttSubscribersMap1.get(i).getClientId() + " did not receive all the sent messages");
			}
		}
		
		if(Trace.traceOn())
		{
			Trace.trace("Checking the messages for mapping 2");
		}
		
		allMsgs = new ArrayList<String>();
		
		for(int i=0; i<jmsProducersMap2.size(); i++)
		{
			allMsgs.addAll(jmsProducersMap2.get(i).getSentMsgs());
		}
		
		if(Trace.traceOn())
		{
			Trace.trace("The producers sent " + allMsgs.size());
		}
		for(int i=0; i<mqttSubscribersMap2.size(); i++)
		{
			
			ArrayList<String> receivedMsgs = mqttSubscribersMap2.get(i).getReceivedMsgs();
			if(Trace.traceOn())
			{
				Trace.trace("Client " + mqttSubscribersMap2.get(i).getClientId() + " received " + receivedMsgs.size() + " messages" );
			}
			if(receivedMsgs.containsAll(allMsgs))
			{
				if(Trace.traceOn())
				{
					Trace.trace("The consumer " + mqttSubscribersMap2.get(i).getClientId() + " received all the messages from the producers");
				}
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("The consumer " + mqttSubscribersMap2.get(i).getClientId() + " did not receive all the sent messages");
					
					for(int j=0; j<receivedMsgs.size(); j++)
					{
						Trace.trace("Received msg=" + receivedMsgs.get(j));
					}
					for(int j=0; j<allMsgs.size(); j++)
					{
						Trace.trace("Producer sent msg=" + allMsgs.get(j));
					}
				}
				
				Assert.fail("The consumer " + mqttSubscribersMap2.get(i).getClientId() + " did not receive all the sent messages");
			}
		}
		
		
		
	}
	
	/*
	 * This method sets up a number of clients for each recoverability test
	 */
	private void setupClients(String prefix) throws Exception
	{
		jmsProducersMap1 = new ArrayList<JmsProducerClient>();
		jmsProducersMap2 = new ArrayList<JmsProducerClient>();
		mqttSubscribersMap1 = new ArrayList<MqttSubscriberClient>();
		mqttSubscribersMap2 = new ArrayList<MqttSubscriberClient>();
		
		String mqconsumerTrace = null;
		for(int i=0; i<2; i++) 
		{
			if(clientTrace)
			{
				mqconsumerTrace = prefix + "_mqconsumer" + i;
			}
			MqttSubscriberClient sub = new MqttSubscriberClient("tcp://"+mqipaddress+":"+mqMqttPort);
			sub.setUserAndPassword(mqusername, mqpassword);
			sub.setCleanSession(false);
			sub.setQos(2);
			sub.checkDuplicates();
			sub.storeReceivedMsgs();
			sub.setTraceFile(mqconsumerTrace);
			sub.setup(mqTopicOut);
			mqttSubscribersMap2.add(sub);
			new Thread(sub).start();
			
		}
		String imaconsumerTrace = null;
		for(int i=0; i<2; i++) 
		{
			if(clientTrace)
			{
				imaconsumerTrace = prefix + "_imaconsumer" + i;
			}
			MqttSubscriberClient sub = new MqttSubscriberClient("tcp://"+imaipaddress+":"+imaport);
			sub.setCleanSession(false);
			sub.setQos(2);
			sub.checkDuplicates();
			sub.storeReceivedMsgs();
			sub.setTraceFile(imaconsumerTrace);
			sub.setup(imaTopicOut);
			mqttSubscribersMap1.add(sub);
			new Thread(sub).start();
			
		}
		
		
		for(int i=0; i<10; i++) 
		{
			JmsProducerClient prod = new JmsProducerClient(imaipaddress, new Integer(imaport));				
			if(clientTrace)
			{
				prod.setTraceFile(prefix+"_imaproducer" + i);
		
			}
			prod.sendPersistentMessages();
			prod.setMessageInterval(5000);
			prod.setNumberOfMessages(10000000);
			prod.storeSentMsgs();
			prod.setMessageType( JmsMessage.MESSAGE_TYPE.SERIAL);
			prod.setup(null, null, imaQueueIn, JmsMessage.DESTINATION_TYPE.QUEUE);
			jmsProducersMap2.add(prod);
			
		}
		
		
		for(int i=0; i<10; i++) 
		{
			JmsProducerClient prod = new JmsProducerClient(mqipaddress, new Integer(mqtcpport), qmgr, clientChannelName);				
			if(clientTrace)
			{
				prod.setTraceFile(prefix+"_mqproducer" + i);
		
			}
			prod.sendPersistentMessages();
			prod.setMessageInterval(5000);
			prod.setNumberOfMessages(10000000);
			prod.storeSentMsgs();
			prod.setMessageType( JmsMessage.MESSAGE_TYPE.SERIAL);
			prod.setup(null, null, mqQueueIn, JmsMessage.DESTINATION_TYPE.QUEUE);
			jmsProducersMap1.add(prod);
			
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
	
	/**
	 * This method tears down a number of clients for each recoverability test
	 * and calls the method to check that we did get all the expected number of 
	 * messages we sent
	 */
	private void teardownClients() throws Exception
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
			for(int i=0; i<mqttSubscribersMap1.size(); i++)
			{
			   	mqttSubscribersMap1.get(i).keepRunning = false;
			}
			for(int i=0; i<mqttSubscribersMap2.size(); i++)
			{
			   	mqttSubscribersMap2.get(i).keepRunning = false;
			}
			// Now check we got all the expected messages
			if(Trace.traceOn())
			{
				Trace.trace("Checking messages for mappings");
			}
			this.checkSentMessages();
		
			
			if(!imaConfig.isConnected())
			{
				imaConfig.connectToServer();
			}
			
			for(int i=0; i<mqttSubscribersMap1.size(); i++)
			{
				try
				{
					imaConfig.deleteMqttClient(mqttSubscribersMap1.get(i).getClientId());
				}
				catch(Exception e)
				{
				}
			}
			for(int i=0; i<mqttSubscribersMap2.size(); i++)
			{
				try
				{
					imaConfig.deleteMqttClient(mqttSubscribersMap2.get(i).getClientId());
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
	
	
	private void checkForStackDumps() throws Exception
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
