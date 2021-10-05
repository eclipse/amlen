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
package com.ibm.ima.svt.regression;

import java.io.File;
import java.util.HashMap;
import java.util.Iterator;

import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.monitor.ImaMonitoring;
import com.ibm.ima.test.cli.monitor.SubscriptionStatResult;
import com.ibm.ims.svt.clients.Audit;

/**
 * This class uses ssh commands to delete subscriptions for a particular topic from an appliance.
 * 
 * It will ask for any subscriptions to that topic and delete them. This will be repeated
 * frequently as the appliance will only return a small number of subscriptions each time.
 *
 * 
 *
 */
public class DeleteSubscriptions implements Runnable {
	
	private ImaConfig config = null;
	private ImaMonitoring monitor = null;
	private String topic = "";
	protected Audit trace = null;
	public boolean keepRunning = true;
	
	public DeleteSubscriptions(String topic, String cliaddress, boolean traceFile) throws Exception
	{
		this.topic = topic;
		config = new ImaConfig(cliaddress, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddress, "admin", "admin");
		monitor.connectToServer();
		
		if(traceFile)
		{
			File audit = new File("DeleteSubscriptions");
			if(!audit.exists())
			{
				audit.createNewFile();
			}
			trace = new Audit("DeleteSubscriptions", true);
		}
	}
	
	public DeleteSubscriptions(String topic, String cliaddressA, String cliaddressB, boolean traceFile) throws Exception
	{
		this.topic = topic;
		config = new ImaConfig(cliaddressA, cliaddressB, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddressA, cliaddressB, "admin", "admin");
		monitor.connectToServer();
		
		if(traceFile)
		{
			File audit = new File("DeleteSubscriptions");
			if(!audit.exists())
			{
				audit.createNewFile();
			}
			trace = new Audit("DeleteSubscriptions", true);
		}
	}

	public void run() {
		
		do
		{
			try
			{
				if(!monitor.isConnected())
				{
					monitor.connectToServer();
				}
				if(!config.isConnected())
				{
					config.connectToServer();
				}
				HashMap<String,SubscriptionStatResult> monitorResults = monitor.getSubStatisticsByTopic(topic);
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Found " + monitorResults.size() + " results for topic " + topic);
				}
				if(!monitorResults.isEmpty())
				{
					Iterator<String> iter = monitorResults.keySet().iterator();
					while(iter.hasNext())
					{
						SubscriptionStatResult result = monitorResults.get(iter.next());
						String sub = result.getSubName();
						String client = result.getclientId();
						try
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Attempting to delete client=" + client + ", subscription=" + sub);
							}
							config.deleteSubscription(sub, client);
						}
						catch(Exception e)
						{
							// 		do nothing
						}
					}
				}
				else
				{
					if(trace!= null && trace.traceOn())
					{
						trace.trace("No results to delete");
					}
				
				}
				Thread.sleep(10000);
			}
			catch(Exception e)
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("An exception occurred " + e.getMessage());
				}
			}
			
		}while(keepRunning);
		
		try
		{
			monitor.disconnectServer();
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.disconnectServer();
		}
		catch(Exception e)
		{
			// do nothing
		}
		
		System.out.println("Finished");
	}

}
