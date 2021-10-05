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
import java.util.ArrayList;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import com.ibm.ims.svt.clients.Audit;

/**
 * This test creates a number of clients, connects to the server, sends a message and then disconnects.
 * 
 * This is repeated until the test is stopped
 *
 */
public class CreateMultipleConnections implements Runnable {
	
	ArrayList<MqttClient> clients = new ArrayList<MqttClient>();
	MqttConnectOptions options = new MqttConnectOptions();
	MqttMessage msg = new MqttMessage("12345".getBytes());
	ArrayList<MqttTopic> topics = new ArrayList<MqttTopic>();
	public boolean keepRunning = true;
	protected Audit trace = null;
	
	public CreateMultipleConnections(String uriA, String uriB, String clientID, boolean cleanSession, int qos, String topicName, int numberClients, boolean traceFile) throws Exception
	{
		for(int i=0; i<numberClients; i++)
		{
			clients.add(new MqttClient(uriA, clientID+"_"+i, null));
			topics.add(clients.get(i).getTopic(topicName));
		}
	
		if(uriB != null)
		{
			options.setServerURIs(new String[]{uriA, uriB});
		}
		options.setCleanSession(cleanSession);
		msg.setQos(qos);
		
		if(traceFile)
		{
			File audit = new File("MultipleConnections");
			if(!audit.exists())
			{
				audit.createNewFile();
			}
			trace = new Audit("MultipleConnections", true);
		}
		
		
	}
	public void run() {
		
		do
		{
			try
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Connecting clients");
				}
				for(int i=0; i<clients.size(); i++)
				{
					clients.get(i).connect(options);
					topics.get(i).publish(msg);
					
				}
				Thread.sleep(5000);
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Disconnecting clients");
				}
				for(int i=0; i<clients.size(); i++)
				{
					if(clients.get(i).isConnected())
					{
						clients.get(i).disconnect();
					}
				}
				Thread.sleep(5000);
				
			}
			catch(Exception me)
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("An exception occurred " + me.getMessage());
				}
				
				try {
					Thread.sleep(10000);
				} catch (InterruptedException e) {
					// do nothing
					
				}
			}
		}
		while(keepRunning);
	}

}
