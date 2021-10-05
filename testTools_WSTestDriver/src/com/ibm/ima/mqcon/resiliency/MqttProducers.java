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
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import com.ibm.ima.mqcon.utils.Audit;



public class MqttProducers implements Runnable{
	
	public boolean keepRunning = true;
	private ConcurrentHashMap<String, String> messageKey = null;
	private ConcurrentHashMap<MqttDeliveryToken, String> messageIDKey = new ConcurrentHashMap<MqttDeliveryToken, String>();
	private int numberMsgs = 0;
	private MqttClient producerA = null;
	private int hACount = 0;
	private String clientID = null;
	private String messagebody = null;
	private MqttTopic topicA = null;
	private int qos = 0;
	private MqttConnectOptions options = new MqttConnectOptions();
	private MqttProducerListener listener = null;
	private boolean cleanSession = false;
	private boolean reconnecting = false;
	public Audit trace = null;
	private boolean incomplete = false;
	private Integer msgID = -1;
	private String tempLog = null;
	
	
	public MqttProducers(String auditLog, int number, String connection, String topicName, int qos, boolean cleanSession, ConcurrentHashMap<String, String> messageKey) throws Exception
	{
		File audit = new File(auditLog);
		if(!audit.exists())
		{
			audit.createNewFile();
		}
		tempLog = auditLog;
		trace = new Audit(auditLog, true);
		this.setup(number, connection, null, topicName, qos, cleanSession, messageKey);
	}
	
	public MqttProducers(String auditLog, int number, String connectionA, String connectionB, String topicName, int qos, boolean cleanSession, ConcurrentHashMap<String, String> messageKey) throws Exception
	{
		File audit = new File(auditLog);
		if(!audit.exists())
		{
			audit.createNewFile();
		}
		tempLog = auditLog;
		trace = new Audit(auditLog, true);
		this.setup(number, connectionA, connectionB, topicName, qos, cleanSession, messageKey);
	}
	
	public MqttProducers(int number, String connection, String topicName, int qos, boolean cleanSession, ConcurrentHashMap<String, String> messageKey) throws Exception
	{
		
		this.setup(number, connection, null, topicName, qos, cleanSession, messageKey);
		
	}
	
	public MqttProducers(int number, String connectionA, String connectionB, String topicName, int qos, boolean cleanSession, ConcurrentHashMap<String, String> messageKey) throws Exception
	{
		
		this.setup(number, connectionA, connectionB, topicName, qos, cleanSession, messageKey);
		
	}
	
	public boolean isreconnecting()
	{
		return reconnecting;
	}
	
	private void setup(int number, String connectionA, String connectionB, String topicName, int qos, boolean cleanSession, ConcurrentHashMap<String, String> messageKey) throws Exception
	{
		// Set up the connections and producers etc
		if(trace.traceOn())
		{
			trace.trace("Setting up a mqtt provider for topic " + topicName);
		}
		String servers[] = {connectionA, connectionB};
		clientID = UUID.randomUUID().toString().substring(0, 15).replace('-', '_');
		
		producerA = new MqttClient(connectionA ,clientID);
		this.qos = qos;
		numberMsgs = number;
		topicA = producerA.getTopic(topicName);
		this.cleanSession = cleanSession;
		options.setCleanSession(cleanSession);
		options.setKeepAliveInterval(120000);
		options.setServerURIs(servers);
		
		
		try
		{
			producerA.connect(options);
		}
		catch(Exception e)
		{
			if(trace.traceOn())
			{
				trace.trace("Could not connect to connectionA - trying connectionB first");
			}
			producerA.connect(options);
			
		}
		
		listener = new MqttProducerListener(this, messageKey, messageIDKey);
		producerA.setCallback(listener);
		this.messageKey = messageKey;
		
				
		// Set up the producer
		if(trace.traceOn())
		{
			trace.trace("Set up complete");
		}
	}


	public void run() {
		

		if(trace.traceOn())
		{
			trace.trace("Producer starting execution");
		}
		
 		for(int i=0; i<numberMsgs; i++)
		{
 			if(! keepRunning)
 			{
 				break;
 			}
			if(i != 0)
			{
				try
				{
					if(trace.traceOn())
					{
						trace.trace("Pausing sending messages");
					}
					Thread.sleep(500); // now go to sleep so we don't send all at once
				}
				catch(InterruptedException e)
				{
				// 	do nothing
				}
			}
			
			if(! isreconnecting())
			{
				try
				{
					messagebody = UUID.randomUUID().toString().substring(0, 30);
					MqttMessage message1 = new MqttMessage(messagebody.getBytes());			
					message1.setQos(qos);
					if(trace.traceOn())
					{
						trace.trace("Generating a new message with id=" + messagebody + " and qos " + qos);
					}
					messageKey.put(messagebody, "Awaiting Delivery");
					incomplete = true;
						
					if(trace.traceOn())
					{
						trace.trace(messagebody + " added to the messageKey to await delivery");
					}
					if(producerA.isConnected())
					{
						MqttDeliveryToken token = topicA.publish(message1);
						messageIDKey.put(token, messagebody);
						incomplete = false;
						if(trace.traceOn())
						{
							trace.trace("Token " + token + " with messageId " + token.getMessageId() + " was associated with the delivery for message " + messagebody);
						}
						if(!cleanSession)
						{
							msgID = token.getMessageId();
						}
					}
					else
					{
						{
							messageKey.remove(messagebody);
							if(trace.traceOn())
							{
								trace.trace("We never sent the message " + messagebody + " so removing it from the key");
							}
							if(! isreconnecting())
							{
								this.resetConnections();
							}
						}
					}
						
				}
				catch(Exception e)
				{
					if(trace.traceOn())
					{
						trace.trace("An exception occurred " + e.getMessage());
						
					}
					if(incomplete)
					{
						if(trace.traceOn())
						{
							trace.trace("We never sent the message " + messagebody + " so removing it from the key");
						}
						if(messagebody != null)
						{
							messageKey.remove(messagebody);
						}
					}
					if(! isreconnecting())
					{
						if(trace.traceOn())
						{
							trace.trace("Calling reset connections");
							this.resetConnections();
						}
					}
				}
			}
			else
			{
				if(trace.traceOn())
				{
					trace.trace("We are reconnecting so going to sleep and try again");
				}
				try
				{
					Thread.sleep(10000);
				}
				catch(Exception e)
				{
					// do nothing
				}
			}
			
			
		}
 		if(trace.traceOn())
		{
			trace.trace("The thread was stopped or we finished sending all our messages so closing connections");
		}
 		if(incomplete)
		{
			if(trace.traceOn())
			{
				trace.trace("We never sent the message " + messagebody + " so removing it from the key");
			}
			if(messagebody != null)
			{
				messageKey.remove(messagebody);
			}
		}
		this.closeConnections();
		
	}
	
	public synchronized void resetConnections()
	{
		reconnecting = true;
		hACount++;
		
		if(trace.traceOn())
		{
			trace.trace("In reset connections");
		}
		if(incomplete)
		{
			if(trace.traceOn())
			{
				trace.trace("We never sent the message " + messagebody + " so removing it from the key");
			}
			if(messagebody != null)
			{
				messageKey.remove(messagebody);
			}
		}
		if(trace.traceOn())
		{
			trace.trace("Trying to connect again with cleanSession=" + cleanSession);
		}
				
		try
		{
			
			if(producerA.isConnected())
			{
				if(trace.traceOn())
				{
					trace.trace("We may still be disconnecting so lets just sleep before we try again");
				}
				Thread.sleep(5000);
			}
				producerA.connect(options);
			reconnecting = false;
			if(cleanSession)
			{
				if(trace.traceOn())
				{
					trace.trace("We need to remove any expectation that the messageID is going to be completed if not already done so");
				}
				if(msgID != null)
				{
					messageIDKey.remove(msgID);
				}
			}
			
				
		}
		catch(Exception e)
		{
			if(trace.traceOn())
			{
				trace.trace("An exception occurred when attempting to reconnect. Will sleep and try again");
			}
			try
			{
				Thread.sleep(10000);
				this.resetConnections();
			}
			catch(Exception ef)
			{
				// do nothing
			}
		}
			
	}
	
	public void closeConnections()
	{
		messageKey = null;
		try
		{
			
			if(producerA.isConnected())
			{
				producerA.disconnect();
			}
			
		}catch(Exception e)
		{
			
		}
		
		
	}

}
