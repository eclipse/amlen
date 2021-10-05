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
package com.ibm.ima.mqcon.resiliency;

import java.io.File;
import java.util.ArrayList;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;

import com.ibm.ima.mqcon.utils.Audit;




public class MqttSubscribers implements Runnable{
		
		public boolean keepRunning = true;
		private MqttClient subscriberA = null;
		private int hACount = 0;
		private String clientID = null;
		private String topicName = null;
		private MqttConnectOptions connOptions = null;		
		private MqttConsumerListener listener = null;
		private int qos = 0;
		public Audit trace = null;
		private boolean reconnecting = false;
		private ArrayList<String> duplicates = new ArrayList<String>();
		private String name = null;
		private String tempLog = null;
			
		
		public MqttSubscribers(String connection, String topicName, boolean cleanSession, int qos, ConcurrentHashMap<String, String> receivedMessages, ConcurrentHashMap<String, String> expectedMessages, String auditLog, boolean checkForDuplicates) throws Exception
		{
			File audit = new File(auditLog);
			if(!audit.exists())
			{
				audit.createNewFile();
			}
			name = auditLog;
			tempLog = auditLog;
			trace = new Audit(auditLog, true);
			this.setup(connection, null, topicName, cleanSession, qos, receivedMessages, expectedMessages, checkForDuplicates);
		}
		
		public MqttSubscribers(String connectionA, String connectionB, String topicName, boolean cleanSession, int qos, ConcurrentHashMap<String, String> receivedMessages, ConcurrentHashMap<String, String> expectedMessages, String auditLog, boolean checkForDuplicates) throws Exception
		{
			File audit = new File(auditLog);
			if(!audit.exists())
			{
				audit.createNewFile();
			}
			tempLog = auditLog;
			name = auditLog;
			trace = new Audit(auditLog, true);
			this.setup(connectionA, connectionB, topicName, cleanSession, qos, receivedMessages, expectedMessages, checkForDuplicates);
		}
		
		public MqttSubscribers(String connection, String topicName, boolean cleanSession, int qos, ConcurrentHashMap<String, String> receivedMessages, ConcurrentHashMap<String, String> expectedMessages) throws Exception
		{
			this.setup(connection, null, topicName, cleanSession, qos, receivedMessages, expectedMessages, false);
			
		}
		
		public MqttSubscribers(String connectionA, String connectionB, String topicName, boolean cleanSession, int qos, ConcurrentHashMap<String, String> receivedMessages, ConcurrentHashMap<String, String> expectedMessages) throws Exception
		{
			this.setup(connectionA, connectionB, topicName, cleanSession, qos, receivedMessages, expectedMessages, false);
			
		}

		public String getName()
		{
			return name;
		}
		
		private void setup(String connectionA, String connectionB, String topicName, boolean cleanSession, int qos, ConcurrentHashMap<String, String> receivedMessages, ConcurrentHashMap<String, String> expectedMessages, boolean checkForDuplicates) throws Exception
		{
			// Set up the connections and producers etc
			if(trace.traceOn())
			{
				trace.trace("Setting up a mqtt subscriber for topic " + topicName);
			}
			String servers[] = {connectionA, connectionB};
			clientID = UUID.randomUUID().toString().substring(0, 15).replace('-', '_');
			connOptions = new MqttConnectOptions();		
			connOptions.setCleanSession(cleanSession);
			connOptions.setKeepAliveInterval(120000);
			connOptions.setServerURIs(servers);
		
			subscriberA = new MqttClient(connectionA, clientID);
			
			this.topicName = topicName;
			listener = new MqttConsumerListener(this, expectedMessages, receivedMessages, checkForDuplicates);
			
			subscriberA.setCallback(listener);
			try
			{
				subscriberA.connect(connOptions);
			}
			catch(Exception e)
			{
				if(trace.traceOn())
				{
					trace.trace("Could not connect to connectionA");
				}
				subscriberA.connect(connOptions);
			
			}
			this.qos = qos;
			subscriberA.subscribe(topicName, qos);
			
		
			if(trace.traceOn())
			{
				trace.trace("Set up complete");
			}
		}
		
		public boolean isreconnecting()
		{
			return reconnecting;
		}
		
		public synchronized void resetConnections()
		{
			
			try
			{
				reconnecting = true;
				hACount++;
				
				if(trace.traceOn())
				{
					trace.trace("Attempting to reconnect subscriber");
				}
					
				if(trace.traceOn())
				{
					trace.trace("Trying to reconnect subscriber B");
				}
				subscriberA.connect(connOptions);
				
			reconnecting = false;
			}
			catch(Exception e)
			{
				if(trace.traceOn())
				{
					trace.trace("An exception occurred in the subscriber. Exception=" + e.getMessage());
					trace.trace("Sleeping for a bit and then will try again" + e.getMessage());
				}
				try
				{
					Thread.sleep(15000);
				}
				catch(Exception es)
				{
					// do nothing
				}
				this.resetConnections();
				
			}
			
			
		}
		
		public void addDuplicate(String msg)
		{
			duplicates.add(msg);
		}
		
		public ArrayList<String> getDuplicates()
		{
			return duplicates;
		}
			

		public void run() 
		{
			
			if(trace.traceOn())
			{
				trace.trace("Subscriber starting execution");
			}
			while(keepRunning)
			{
				try
				{
					Thread.sleep(5000); //wait 5 seconds and try and reconnect
					if(subscriberA.isConnected() || this.isreconnecting())
					{
						// do nothing
					}
					else
					{
						this.resetConnections();
					}
						
				}		
				catch(Exception e)
				{
					// do nothing
				}
		
			
			}	
			if(trace.traceOn())
			{
				trace.trace("The thread was stopped so closing connections");
			}
			this.closeConnections();
		}

		public void closeConnections()
		{
			try
			{
				subscriberA.unsubscribe(topicName);
				subscriberA.disconnect();
			}
			catch(Exception e)
			{
			}
		}
}
