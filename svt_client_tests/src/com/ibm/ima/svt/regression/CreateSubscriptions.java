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
import java.util.UUID;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;

import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.config.ImaConfig.TYPE;
import com.ibm.ima.test.cli.monitor.ImaMonitoring;
import com.ibm.ima.test.cli.monitor.SubscriptionStatResult;
import com.ibm.ims.svt.clients.Audit;

/**
 * The intention of this class is to keep creating different durable and non durable subscriptions
 * so that we create owner records of this type in the store plus we keep connecting and disconnecting
 * non durable subscriptions to throw some other stuff into the mix. 
 * 
 * As I expect this class to be run alongside the usual regression stress bucket we will just keep generating these so they
 * build up over time.
 *
 */
public class CreateSubscriptions implements Runnable {
	
	private String port = null; // default but each class can use a different one
	private String ipaddress = null;
	private String prefix = "";
	private String topic = "TOPIC";

	private String endpoint = "SVTDSubscriptionsEP";
	private String connectionPolicy = "SVTSubscriptionsCP";
	private String messagingPolicy = "SVTSubscriptionsMP";
	private String messagingHub = "SVTSubscriptionsHub";
	private String clientID = UUID.randomUUID().toString().substring(0, 9);
	private boolean tearDownClients = false;
	
	private int numberToCreate = 1;
	
	private ImaConfig config = null;
	private ImaMonitoring monitor = null;
	protected Audit trace = null;
 	
	public CreateSubscriptions(String prefix, String ipaddress, String port, String cliaddress, int number, boolean tearDown, boolean traceFile) throws Exception
	{
		this.ipaddress = ipaddress;
		this.port = port;
		this.prefix = prefix;
		config = new ImaConfig(cliaddress, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddress, "admin", "admin");
		monitor.connectToServer();
		this.numberToCreate = number;
		this.tearDownClients = tearDown;
		if(traceFile)
		{
			File audit = new File("CreateSubscriptions");
			if(!audit.exists())
			{
				audit.createNewFile();
			}
			trace = new Audit("CreateSubscriptions", true);
		}
		this.setup();
	}
	
	public CreateSubscriptions(String prefix, String ipaddress, String port, String cliaddressA, String cliaddressB, int number, boolean tearDown, boolean traceFile) throws Exception
	{
		/*
		 * ipaddress may contain '+' as delimiter or ','. JMS api's use either <space> or comma for delimiter. Therefore,
		 * if '+' is used the convert to comma. This change is due to automation which cannot handle ',' as delimiter
		 */
		if(ipaddress.contains("+"))
		{	
			/*
			 * convert '+' to ','
			 */
			this.ipaddress = ipaddress.trim().replace('+',',');
		} else {
			/*
			 * Assume ',' is being used
			 */
			this.ipaddress = ipaddress;
		}
		this.port = port;
		this.prefix = prefix;
		config = new ImaConfig(cliaddressA, cliaddressB, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddressA, cliaddressB, "admin", "admin");
		monitor.connectToServer();
		this.numberToCreate = number;
		this.tearDownClients = tearDown;
		if(traceFile)
		{
			File audit = new File("CreateSubscriptions");
			if(!audit.exists())
			{
				audit.createNewFile();
			}
			trace = new Audit("CreateSubscriptions", true);
		}
		this.setup();
	}
	
	private void setup() throws Exception
	{
		topic = prefix + topic;
		
		if(tearDownClients)
		{
			HashMap<String,SubscriptionStatResult> monitorResults = monitor.getSubStatisticsByTopic(topic);
			
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
						config.deleteSubscription(sub, client);
					}
					catch(Exception e)
					{
						// 	do nothing
					}
				}
			}
		}
		
		if(config.endpointExists(endpoint))
		{
			config.deleteEndpoint(endpoint);
		}
		if(config.messagingPolicyExists(messagingPolicy))
		{
			config.deleteMessagingPolicy(messagingPolicy);
		}
		if(config.connectionPolicyExists(connectionPolicy))
		{
			config.deleteConnectionPolicy(connectionPolicy);
		}
		if(config.messageHubExists(messagingHub))
		{
			config.deleteMessageHub(messagingHub);
		}
		
		if(! config.messageHubExists(messagingHub))
		{
			config.createHub(messagingHub, "Used by Hursley SVT");
		}
		if(! config.connectionPolicyExists(connectionPolicy))
		{
			config.createConnectionPolicy(connectionPolicy, connectionPolicy, TYPE.JMSMQTT);
		}
		if(! config.messagingPolicyExists(connectionPolicy))
		{
			config.createMessagingPolicy(messagingPolicy, messagingPolicy, "*", TYPE.Topic, TYPE.JMSMQTT);
			config.updateMessagingPolicy(messagingPolicy, "MaxMessages", "20000000");
		}
		if(! config.endpointExists(endpoint))
		{
			config.createEndpoint(endpoint, endpoint, port, TYPE.JMSMQTT, connectionPolicy, messagingPolicy, messagingHub);
		}	
		
		try
		{
			config.disconnectServer();
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			monitor.disconnectServer();
		}
		catch(Exception e)
		{
			// do nothing
		}
	}
	
	private void tearDown() throws Exception
	{
		if(!config.isConnected())
		{
			config.connectToServer();
		}
		if(!monitor.isConnected())
		{
			monitor.connectToServer();
		}
		
	
		if(tearDownClients)
		{
			HashMap<String,SubscriptionStatResult> monitorResults = monitor.getSubStatisticsByTopic(topic);
		
		
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
						config.deleteSubscription(sub, client);
					}
					catch(Exception e)
					{
					// 		do nothing
					}
				}
			}
		}		
		try
		{
			config.deleteEndpoint(endpoint);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteConnectionPolicy(connectionPolicy);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessageHub(messagingHub);
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
		try
		{
			monitor.disconnectServer();
		}
		catch(Exception e)
		{
			// do nothing
		}
	}


	public void run() {
		
		String[] urls = null;
		if(ipaddress.contains(",") || ipaddress.contains("+"))
		{	
			/*
			 * split either on "," "|" or "+"
			 */
			String[] temp = ipaddress.trim().split(",|\\||\\+", -1);
			urls = new String[temp.length];
			for(int i=0; i<temp.length; i++)
			{
				urls[i] = "tcp://" + temp[i] + ":" + port;
			}
			ipaddress = urls[0];
			
		}
		else
		{
			ipaddress = "tcp://" + ipaddress + ":" + port;
			
		}
		
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Starting to create subscrptions with prefix " + clientID);
		}
		System.out.println("Starting to create subscrptions with prefix " + clientID);
		
		MqttConnectOptions options = new MqttConnectOptions();
		if(urls != null)
		{
			options.setServerURIs(urls);
		}
		
		for(int i=0; i<numberToCreate; i++)
		{
			try
			{
				MqttClient client = new MqttClient(ipaddress, clientID+i);
				if(i%2 ==0)
				{
					options.setCleanSession(true);
				}
				else
				{
					options.setCleanSession(false);
				}
				client.connect(options);
				client.subscribe(topic);
				client.disconnect();
				
				client = null;
				
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Created and disconnect " + i);
				}
				
				Thread.sleep(500);
				
			}
			catch(Exception e)
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("An exception occurred " + e.getMessage());
				}
			
				// 	the server might be down so wait and keep trying
				i--;
				try {
					Thread.sleep(5000);
				} catch (InterruptedException e1) {
					// do nothing
				}
			}
		}
		
		System.out.println("Finished");
		try
		{
			this.tearDown();
		}
		catch(Exception e)
		{
			// do nothing
		}
	}	

}
