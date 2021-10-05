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

import org.junit.Assert;

import com.ibm.ims.svt.clients.Trace;

/**
 * This class is used to test mqconnectivity for a single queue manager using two queue manager
 * ip connections.
 *
 */
public class TestMultiQmgrsSingleConnection extends TestMultipleMQQmgrsBase {
	
	
	public static void main(String args[]) {
		
		if(args.length != 4)
		{
			
			System.out.println("Please pass in the ipaddress of the ima server, mqserverA, mqserverB, mqpassword");
		}
		else
		{
			imaipaddress = args[0];
			mqipaddressA = args[1];
			mqipaddressB = args[2];
			mqpasswordA = args[3]; // TODO remove once passwordless ssh has been tested
			mqpasswordB = args[3]; // TODO remove once passwordless ssh has been tested
			testAutomation = true;
		}
		org.junit.runner.JUnitCore.main("com.ibm.ima.svt.mq.recoverability.TestMultiQmgrsSingleConnection");
		
	}

	
	public void setInitialParams()
	{
		mqtcpportA = 1447;
		mqtcpportB = 1448;
		imaport = "16316";
		qmgrA = "SVT_QMC";
		qmgrB = "SVT_QMC";
		onewayonly = true;
		prefix = "QmgrsSingleConnection"; 

		
	}
	

	public void setUp() throws Exception {
		
		
		try
		{
			new Trace("stdout", true);
			
			super.setUp();
			
			if(imaMqConfig != null)
			{
				if(!imaMqConfig.isConnected())
				{
					imaMqConfig.connectToServer();
				}
				if(Trace.traceOn())
				{
					Trace.trace("Cleaing up mqconnectivity qmgr config");
				}
				if(imaMqConfig.queueManagerConnectionExists(qmgrA))
				{
					imaMqConfig.deleteQueueManagerConnection(qmgrA);
				}
				
				if(Trace.traceOn())
				{
					Trace.trace("Creating new mqconnectivity config");
				}
				
				imaMqConfig.createQueueManagerConnection(qmgrA, qmgrA, mqipaddressA+"("+mqtcpportA+"),"+mqipaddressB+"("+mqtcpportB+")", channelName);
				imaMqConfig.createMappingRule(prefix+"_out", qmgrA, "3", mqQueueIn, imaTopicOut);
				imaMqConfig.createMappingRule(prefix+"_in", qmgrA, "10", imaQueueIn, mqQueueOut);
				
				try
				{
					imaMqConfig.disconnectServer();
				}
				catch(Exception e)
				{
				// 	do nothing
				}
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


	public void testMultipleQueueManagers()
	{
		try
		{

			// first check to see what files we have on the server
			checkForStackDumps();
			
			setupClients();					
		
			Thread.sleep(intervalTime * 2);
			
			if(!mqConfigA.isConnected())
			{
				mqConfigA.connectToServer();
			}
			if(!mqConfigB.isConnected())
			{
				mqConfigB.connectToServer();
			}
			mqConfigB.stopChannel(channelName, qmgrB);
		  	if(mqConfigB.isChannelActive(channelName, qmgrB))
		  	{
				throw new Exception("We were unable to stop the channel for qmgrB - exiting the test");
			}
			if(Trace.traceOn())
		    {
		    	Trace.trace("The channel was stopped for qmgrB");
			}
			qmgrADepth = mqConfigA.getCurrentQueueDepth(mqQueueOut, qmgrA);
			qmgrBDepth = mqConfigB.getCurrentQueueDepth(mqQueueOut, qmgrB);
			Thread.sleep(intervalTime); //wait a bit to check we dont get any messages in the meantime
			checkCurDepth(false, true);
			
			mqConfigB.startChannel(channelName, qmgrB);
			if(Trace.traceOn())
		    {
		    	Trace.trace("The channel was started for qmgrB");
			}
			Thread.sleep(intervalTime * 2); //wait a bit to check mqconnectivity has time to notice we are back up
			checkCurDepth(false, true ); // as single connection mqconnectivity should still send to qmgrA only
			
			
			mqConfigA.stopChannel(channelName, qmgrA);
		  	if(mqConfigA.isChannelActive(channelName, qmgrA))
		  	{
				throw new Exception("We were unable to stop the channel for qmgrB - exiting the test");
			}
			if(Trace.traceOn())
		    {
		    	Trace.trace("The channel was stopped for qmgrA");
			}
			qmgrADepth = mqConfigA.getCurrentQueueDepth(mqQueueOut, qmgrA);
			qmgrBDepth = mqConfigB.getCurrentQueueDepth(mqQueueOut, qmgrB);
			Thread.sleep(intervalTime); //wait a bit to check we dont get any messages in the meantime
			checkCurDepth(true, false);
			
			mqConfigA.startChannel(channelName, qmgrA);
			if(Trace.traceOn())
		    {
		    	Trace.trace("The channel was started for qmgrA");
			}
			Thread.sleep(intervalTime * 2); //wait a bit to check mqconnectivity has time to notice we are back up
			checkCurDepth(true, false); // as single connection mqconnectivity should still send to qmgrA only
			
			mqConfigB.stopChannel(channelName, qmgrB);
		  	if(mqConfigB.isChannelActive(channelName, qmgrB))
		  	{
				throw new Exception("We were unable to stop the channel for qmgrB - exiting the test");
			}
			if(Trace.traceOn())
		    {
		    	Trace.trace("The channel was stopped for qmgrB");
			}
			qmgrADepth = mqConfigA.getCurrentQueueDepth(mqQueueOut, qmgrA);
			qmgrBDepth = mqConfigB.getCurrentQueueDepth(mqQueueOut, qmgrB);
			Thread.sleep(intervalTime * 2); //wait a bit to check mqconnectivity has time to notice we are back up
			checkCurDepth(false, true ); // as single connection mqconnectivity should still send to qmgrA only
					
			if(Trace.traceOn())
		    {
		    	Trace.trace("Tear down consumers and producers for recoverability test");
			}			
			teardownClients();
		   
			// Now check to see if we have any stack files produced whilst we ran this test
			checkForStackDumps();
			
			finalTearDown();
			
			System.out.println("SUCCESS");
			
			
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
	
	
	
	public void finalTearDown() throws Exception {
		
		if(Trace.traceOn())
		{
			Trace.trace("Cleaing up mqconnectivity config");
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
				imaMqConfig.deleteQueueManagerConnection(qmgrA);
			}
			catch(Exception e)
			{
			// 	do nothing
			}
			
		}
		
		try
		{
			imaMqConfig.disconnectServer();
		}
		catch(Exception e)
		{
			
		}
		TestMultipleMQQmgrsBase.tidyUp();
		
		
		
		
	}
	

}
