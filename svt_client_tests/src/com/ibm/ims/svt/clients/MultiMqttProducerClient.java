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
package com.ibm.ims.svt.clients;

import java.util.Random;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;

/**
 * This class is used to contain multiple clients 
 *
 */
public class MultiMqttProducerClient extends MqttProducerClient implements Runnable {

	private int messageSize = 1;
	private MqttTopic[] topics = null;
	private MqttClient[] producers = null;
	private String[] clientIDs = null;
	private MqttProducerListener listener = null;
	private int numberClients = 1;
	private boolean sendSimpleTextMessage = false;

	private int sleepTime = 5000;
	private int sleepMaxTime = 180000;
	
	/**
	 * Standalone
	 * @param connection
	 * @throws Exception
	 */
	public MultiMqttProducerClient(String connection) throws Exception
	{
		super(connection);
		
	}
	
	/**
	 * HA
	 * @param key
	 * @param urls
	 */
	public MultiMqttProducerClient(String key, String[] urls)
	{
		super(key, urls);
	}
	
	/**
	 * Must be called after the clients have been configured but before the thread started
	 * @param topicName
	 * @param messageKey
	 * @throws Exception
	 */
	public void setUpClient(String topicName, ConcurrentHashMap<String, String> messageKey) throws Exception
	{
		// Set up the connections and producers etc
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Setting up " + this.numberClients + " mqtt providers for topic " + topicName);
		}
		
		options.setCleanSession(cleanSession);
		options.setKeepAliveInterval(120000);
		
		if(ipaddress != null)
		{
			options.setServerURIs(ipaddress);
		}
		
		
		if(username != null && password != null)
		{
			options.setUserName(username);
			options.setPassword(password.toCharArray());
		}
		
		producers = new MqttClient[this.numberClients];
		clientIDs = new String[this.numberClients];
		topics = new MqttTopic[this.numberClients];
		Random random = new Random();
		for(int i=0; i<numberClients; i++)
		{
			int randomNumber = random.nextInt(20 - 15) + 15;
			int randomNumber2 = random.nextInt(99 - 1) + 1;
			String clientID = UUID.randomUUID().toString().substring(0, randomNumber).replace('-', '_')+ "_" + randomNumber2 + i;
			clientIDs[i] = clientID;
			producers[i] = mqttconnection.getClient(clientID);
			topics[i] = producers[i].getTopic(topicName);
		}
		
		
		listener = new MqttProducerListener(this, null, false);
		producers[0].setCallback(listener);
		
		for(int i=0; i<producers.length; i++)
		{
			producers[i].connect(options);
		}
		
		
		StringBuffer buf = new StringBuffer();
		buf.append("Multi MQTT producer: \n");
		for(int i=0; i<clientIDs.length; i++)
		{
			buf.append("Clientid:"+clientIDs[i] + "\n");
		}
		buf.append("Destination:"+topicName + "\n");
		buf.append("CleanSession:"+this.cleanSession + "\n");
		buf.append("Qos:" + this.qos + "\n");
		buf.append("Retained:" + this.retainedMessages + "\n");
		
		this.clientDetails = buf.toString();
		// Set up the producer
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Set up complete for " + clientDetails);
		}
		
	}
	
	

	
	/**
	 * This method sets the minimum time interval between each message
	 * being sent. If this is not called then 5000 will be used
	 * This method must be called before setUpClient(...)
	 * @param interval
	 */
	public void messageInterval(int interval)
	{
		this.messageInterval = interval;
	}
	

	/**
	 * Used to close all the producers
	 */
	public void closeConnections()
	{
		for(int i=0; i<producers.length; i++)
		{
			try
			{
				
				producers[i].disconnect();
				
				
			}catch(Exception e)
			{
				
			}
		}
		
	}
	

	/**
	 * Used to reconnect all the producers
	 */
	public synchronized void resetConnections()
	{
		
		if(isreconnecting())
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("We are already reconnecting so return");
			}
			return;
		}
		
		boolean allConnected = true;
		for(int i=0; i<producers.length; i++)
		{
			if(!producers[i].isConnected())
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("We found a closed connected");
				}
				allConnected = false;;
			}
			
		}
		if(allConnected)
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("We are already connected so return");
			}
			return;
		}
		
		reconnecting = true;
		
		if(trace!= null && trace.traceOn())
		{
			trace.trace("In reset connections");
		}
		
		
		for(int i=0; i<producers.length; i++)
		{
			try
			{
				producers[i].disconnect(); // make sure they are all disconnected
			}
			catch(Exception e)
			{
				// do nothing
			}
		}
			
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Trying to connect again with cleanSession=" + cleanSession);
		}
		
		
		try
		{
			for(int i=0; i<producers.length; i++)
			{
				producers[i].connect(options);
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Producer was successfully reconnected");
				}
				
			}
					
			reconnecting = false;
				
				
		}
		catch(Exception e)
		{
			
			reconnecting = false;
			if(trace!= null && trace.traceOn())
			{
				trace.trace("An exception occurred when attempting to reconnect. Will sleep and try again");
			}
			try
			{
				this.sleepTime = sleepTime + 5000;
				if(sleepTime > this.sleepMaxTime)
				{
					Thread.sleep(sleepMaxTime);
				}
				else
				{
					Thread.sleep(sleepTime);
				}
				this.resetConnections();
			}
			catch(Exception ef)
			{
				// do nothing
			}
		}
			
	}
	
	
	
	
	
	public void run() {
		
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Producers starting execution");
		}
		
		
 		for(int i=0; i<numberMsgs; i++)
		{
 			if(keepRunning)
 			{
 				
 			
 				if(i != 0)
 				{
 					try
 					{
 						if(trace!= null && trace.traceOn())
 						{
 							trace.trace("Pausing sending messages");
 						}
 						Thread.sleep(messageInterval); // now go to sleep so we don't send all at once
 					}
 					catch(InterruptedException e)
 					{
 						// 	do nothing
 					}
 				}
 				
			
 				if(!isreconnecting())
 				{
 					String messageBody = null; 
 	 				
 	 				if(sendSimpleTextMessage)
 	 				{
 	 					messageBody = Integer.toString(i);
 	 				}
 	 				else
 	 				{
 	 					messageBody = createMessageKB(this.messageSize);
 	 				}
 					try
 					{
 						for(int j=0; j<topics.length; j++)
 						{
 							MqttMessage message = new MqttMessage((messageBody + clientIDs[j] + j).getBytes());
 							message.setQos(this.qos);
 							if(this.retainedMessages)
 							{
 								message.setRetained(true);
 							}
 							topics[j].publish(message);
 							if(trace!= null && trace.traceOn())
 							{
 								trace.trace("Client " + clientIDs[j] + " sent message " + j);
 							}
 						}
 								
 					}
 					catch(Exception e)
 					{
 						
 						if(trace!= null && trace.traceOn())
 						{
 							trace.trace("An exception occurred " + e.getMessage());	
 						}
 					}
 				}
 				else
 				{
 					if(trace!= null && trace.traceOn())
 					{
 						trace.trace("We are reconnecting so going to sleep and try again");
 					}
 					try
 					{
 						Thread.sleep(10000);
 					}
 					catch(Exception e)
 					{
					// 	do nothing
 					}
 				}
			
 			}
 			else
 			{
 				if(trace!= null && trace.traceOn())
 				{
 					trace.trace("Keep running was set to be false");
 				}
 				break;
 			}
		}
 		if(trace!= null && trace.traceOn())
		{
			trace.trace("The thread was stopped or we finished sending all our messages so closing connections");
		}
 	
		this.closeConnections();
		
	}
	
	/**
	 * Configures the size of message to send
	 * This method must be called before setUpClient(...)
	 * @param size
	 */
	public void setMessageSizeKB(int size)
	{
		this.messageSize = size;
	}
	
	/**
	 * Configures the producer to send a small text message
	 * This method must be called before setUpClient(...)
	 */
	public void sendSimpleTextMessage()
	{
		this.sendSimpleTextMessage = true;
	}
	
	/**
	 * Specified the number of clients to create
	 * @param clientNum
	 */
	public void numberClients(int clientNum)
	{
		this.numberClients = clientNum;
	}

	/**
	 * Used to create a message of a specified size
	 * @param size
	 * @return
	 */
	public static String createMessageKB(int size)
	{
		
		Random randomGenerator = new Random();
		StringBuilder messageBody = new StringBuilder();
		long messageSizeBytes = size * 1024;
		for(int x = 0;x<messageSizeBytes;x++){
			int randomInt = randomGenerator.nextInt(10);
			messageBody.append(randomInt);
		}
		return messageBody.toString();
	}
}
