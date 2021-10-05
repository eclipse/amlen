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
import java.util.UUID;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.config.ImaConfig.TYPE;
import com.ibm.ims.svt.clients.Audit;

/**
 * The intention of this class is to keep creating different topics all with a retained message
 * so as to create more owner records of this type in the store. As I expect this class to be
 * run alongside the usual regression stress bucket we will just keep generating these so they
 * build up over time.
 *
 */
public class CreateTopicsWithRetainedMessages implements Runnable{
	
	private String port = null; // default but each class can use a different one
	private String ipaddress = null;
	private String prefix = "";

	private String endpoint = "SVTRetainedTopicEP";
	private String connectionPolicy = "SVTRetainedTopicCP";
	private String messagingPolicy = "SVTRetainedTopicMP";
	private String messagingHub = "SVTRetainedTopicHub";
	private String clientID = UUID.randomUUID().toString().substring(0, 8);

	private int numberToCreate = 1;
	protected Audit trace = null;
	
	private ImaConfig config = null;
 	
	public CreateTopicsWithRetainedMessages(String prefix, String ipaddress, String port, String cliaddress, int number, boolean traceFile) throws Exception
	{
		this.ipaddress = ipaddress;
		this.port = port;
		this.prefix = prefix;
		config = new ImaConfig(cliaddress, "admin", "admin");
		config.connectToServer();
		this.numberToCreate = number;
		if(traceFile)
		{
			File audit = new File("CreateTopicsWithRetainedMsg");
			if(!audit.exists())
			{
				audit.createNewFile();
			}
			trace = new Audit("CreateTopicsWithRetainedMsg", true);
		}
		this.setup();
	}
	
	public CreateTopicsWithRetainedMessages(String prefix, String ipaddress, String port, String cliaddressA, String cliaddressB, int number, boolean traceFile) throws Exception
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
		this.numberToCreate = number;
		if(traceFile)
		{
			File audit = new File("CreateTopicsWithRetainedMsg");
			if(!audit.exists())
			{
				audit.createNewFile();
			}
			trace = new Audit("CreateTopicsWithRetainedMsg", true);
		}
		this.setup();
	}
	
	private void setup() throws Exception
	{		
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
		
	}
	
	private void tearDown() throws Exception
	{
		if(!config.isConnected())
		{
			config.connectToServer();
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
			System.out.println("IP=" + ipaddress);
		}
		
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Starting to create topics with prefix " + prefix + "/" + clientID + "/");
		}
		System.out.println("Starting to create topics with prefix " + prefix + "/" + clientID + "/");
		
		MqttConnectOptions options = new MqttConnectOptions();
		if(urls != null)
		{
			options.setServerURIs(urls);
		}
		options.setCleanSession(true);
		
		MqttClient client = null;
		MqttMessage msg = new MqttMessage(clientID.getBytes());
		msg.setQos(2);
		msg.setRetained(true);
		try
		{
			client = new MqttClient(ipaddress, clientID);
			client.connect(options);
		}
		catch(MqttException e)
		{
			System.out.println("Unable to create initial client " + e.getMessage());
			System.exit(0);
		}
		
		for(int i=0; i<numberToCreate; i++)
		{
			try
			{
				if(! client.isConnected())
				{
					client.connect(options);
				}
				MqttTopic producerTopic = client.getTopic(prefix+ "/" + clientID + "/"  + i);				
				producerTopic.publish(msg);
				producerTopic = null;
						
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Sent retained message to " + prefix + "/" + clientID + "/" + i);
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
