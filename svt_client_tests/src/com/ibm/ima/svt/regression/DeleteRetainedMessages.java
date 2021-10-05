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

import com.ibm.ims.svt.clients.Audit;

/**
 * This class sends empty bodied retained MQTT messages to topics
 * which will remove any retained messages from that topic.
 * 
 *
 */
public class DeleteRetainedMessages implements Runnable {
	
	private String port = null; // default but each class can use a different one
	private String ipaddress = null;
	private String clientID = UUID.randomUUID().toString().substring(0, 7);
	protected Audit trace = null;
	public boolean keepRunning = true;
	private String prefix = null;
	private int min = 1;
	private int max = 1;
	
	public DeleteRetainedMessages(String ipaddress, String port, boolean traceFile, String prefix, int min, int max) throws Exception
	{
		this.ipaddress = ipaddress;
		this.port = port;
		this.prefix = prefix;
		this.min = min;
		this.max = max;
		if(traceFile)
		{
			File audit = new File("DeleteRetainedMessages");
			if(!audit.exists())
			{
				audit.createNewFile();
			}
			trace = new Audit("DeleteRetainedMessages", true);
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
			trace.trace("Starting client to delete retained messages from "+ prefix);
		}
		System.out.println("Starting client to delete retained messages from " + prefix);
		
		MqttConnectOptions options = new MqttConnectOptions();
		if(urls != null)
		{
			options.setServerURIs(urls);
		}
		options.setCleanSession(true);
		
		MqttClient client = null;
		MqttMessage msg = new MqttMessage();
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
		
		
			
		for(int i=min; i<=max; i++)
		{
			try
			{
				if(! client.isConnected())
				{
					client.connect(options);
				}
				MqttTopic producerTopic = client.getTopic(prefix + i);				
				producerTopic.publish(msg);
				producerTopic = null;	
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Sent empty retained message to " + prefix + i);
				}
			}
			catch(Exception e)
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("An exception occurred " + e.getMessage());
				}
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
			client.close();
		}
		catch(MqttException me)
		{
			// do nothing
		}
		
	}

}
