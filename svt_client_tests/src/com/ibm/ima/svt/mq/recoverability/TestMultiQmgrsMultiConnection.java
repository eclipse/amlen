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
 * This class is used to test mqconnectivity for multiple queue managers used in a single destination
 * mapping rule
 *
 */
public class TestMultiQmgrsMultiConnection extends TestMultipleMQQmgrsBase {
	
public static void main(String args[]) {
		
		if(args.length == 4)
		{
			imaipaddress = args[0];
			mqipaddressA = args[1];
			mqipaddressB = args[2];
			mqpasswordA = args[3]; // TODO remove once passwordless ssh has been tested
			mqpasswordB = args[3]; // TODO remove once passwordless ssh has been tested
			testAutomation = true;
		}
		else if(args.length == 5)
		{
			imaipaddress = args[0];
			mqipaddressA = args[1];
			mqipaddressB = args[2];
			mqpasswordA = args[3]; // TODO remove once passwordless ssh has been tested
			mqpasswordB = args[3]; // TODO remove once passwordless ssh has been tested
			mquser = args[4];
			testAutomation = true;
		}
		org.junit.runner.JUnitCore.main("com.ibm.ima.svt.mq.recoverability.TestMultiQmgrsMultiConnection");
		
	}
	
	public void setInitialParams()
	{
		mqtcpportA = 1437;
		mqtcpportB = 1438;
		imaport = "16311";
		qmgrA = "SVT_QMA";
		qmgrB = "SVT_QMB";
		prefix = "QmgrsMultiConnection";

	}
	
	public void setUp() throws Exception {
		
		try
		{
			new Trace("stdout", true);
			
			super.setUp();
			
			if(Trace.traceOn())
			{
				Trace.trace("Cleaning up any old mqconnectivity config");
			}
			// 	Now set up the imaserver config
			imaMqConfig.connectToServer();
				
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
			if(imaMqConfig.queueManagerConnectionExists(qmgrA))
			{
				imaMqConfig.deleteQueueManagerConnection(qmgrA);
			}
			if(imaMqConfig.queueManagerConnectionExists(qmgrB))
			{
				imaMqConfig.deleteQueueManagerConnection(qmgrB);
			}
			
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating new mqconnectivity config");
			}
			
			imaMqConfig.createQueueManagerConnection(qmgrA, qmgrA, mqipaddressA+"("+mqtcpportA+")", channelName);
			imaMqConfig.createQueueManagerConnection(qmgrB, qmgrB, mqipaddressB+"("+mqtcpportB+")", channelName);
			imaMqConfig.createMappingRule(prefix+"_out", qmgrA+","+qmgrB, "3", mqQueueIn, imaTopicOut);
			imaMqConfig.createMappingRule(prefix+"_in", qmgrA+","+qmgrB, "10", imaQueueIn, mqQueueOut);
			
			try
			{
				imaMqConfig.disconnectServer();
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
			checkCurDepth(false, false);
			
			
			mqConfigA.stopChannel(channelName, qmgrA);
		  	if(mqConfigA.isChannelActive(channelName, qmgrA))
		  	{
				throw new Exception("We were unable to stop the channel for qmgrB - exiting the test");
			}
			if(Trace.traceOn())
		    {
		    	Trace.trace("The channel was stopped for qmgrA");
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
			checkCurDepth(true, true);
			
			mqConfigB.startChannel(channelName, qmgrB);
			if(Trace.traceOn())
		    {
		    	Trace.trace("The channel was started for qmgrB");
			}
			Thread.sleep(intervalTime * 2); //wait a bit to check mqconnectivity has time to notice we are back up
			checkCurDepth(true, false);
			
			mqConfigA.startChannel(channelName, qmgrA);
			if(Trace.traceOn())
		    {
		    	Trace.trace("The channel was started for qmgrA");
			}
			Thread.sleep(intervalTime * 2); //wait a bit to check mqconnectivity has time to notice we are back up
			checkCurDepth(false, false);
			
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
			try
			{
				imaMqConfig.deleteQueueManagerConnection(qmgrA);
			}
			catch(Exception e)
			{
			// 	do nothing
			}
			try
			{
				imaMqConfig.deleteQueueManagerConnection(qmgrB);
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
