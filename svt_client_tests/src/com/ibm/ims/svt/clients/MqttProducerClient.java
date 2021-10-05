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

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttToken;
import org.eclipse.paho.client.mqttv3.MqttTopic;

/**
 * Used to create a single MqttProducerClient thread.
 *
 */
public class MqttProducerClient implements Runnable {

	protected boolean cleanSession = true;
	protected int qos = 0;
	protected boolean recoverConnection = true;
	protected int numberMsgs = 1000000;
	protected int messageInterval = 5000;
	protected boolean retainedMessages = false;
	protected int retainedInterval = 0;
	protected ConcurrentHashMap<Integer, String> messageIDKey = new ConcurrentHashMap<Integer, String>();
	protected int retainedNumber = 0;
	protected Integer msgID = -1;
	protected boolean reconnecting = false;
	protected String messagebody = null;
	
	protected String username = null;
	protected String password = null;	
	protected MqttTopic topic = null;
	protected MqttConnection mqttconnection =null;
	private MqttClient producer = null;
	private String clientID = UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	protected MqttConnectOptions options = new MqttConnectOptions();
	private MqttProducerListener listener = null;
	protected String[] ipaddress = null;
	
	protected Audit trace = null;
	public boolean keepRunning = true;
	protected String failMessage = "";
	protected boolean storeSentMsgs = false;
	
	protected String clientDetails = null;
	
	
	/**
	 * Used for standalone clients
	 * @param connection
	 * @throws Exception
	 */
	public MqttProducerClient(String connection) throws Exception
	{
		mqttconnection = new MqttConnection(connection);
		
	}
	
	/**
	 * Used for HA clients
	 * @param key
	 * @param urls
	 */
	public MqttProducerClient(String key, String[] urls)
	{
		mqttconnection = new MqttConnection(key);
		this.ipaddress = urls;
	}
	
	/**
	 * This method should be the final one called after the client has been configured and before calling start
	 * on the thread.
	 * @param topicName
	 * @throws Exception
	 */
	public void setUpClient(String topicName) throws Exception
	{
		// Set up the connections and producers etc
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Setting up a mqtt provider for topic " + topicName);
		}
				
		producer = mqttconnection.getClient(clientID);
	
		topic = producer.getTopic(topicName);
		options.setCleanSession(cleanSession);
		
		// Need to make sure we configure for HA if required
		if(ipaddress != null)
		{
			options.setServerURIs(ipaddress);
		}
		
		
		if(username != null && password != null)
		{
			options.setUserName(username);
			options.setPassword(password.toCharArray());
		}
		
		listener = new MqttProducerListener(this, messageIDKey, storeSentMsgs);		
		producer.setCallback(listener);
		producer.connect(options);
		
		StringBuffer buf = new StringBuffer();
		buf.append("Single MQTT producer: \n");
		buf.append("Clientid:"+this.clientID + "\n");
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
	 * Can be used to specify a client ID otherwise this class will create one.
	 * This method must be called before setUpClient(...)
	 * @param overrideClientID
	 */
	public void setClientID(String overrideClientID)
	{
		this.clientID = overrideClientID;
	}
	
	
	/**
	 * Set the username and password if this is required on the connection
	 * This method must be called before setUpClient(...)
	 * @param username
	 * @param password
	 */
	public void setUserAndPassword(String username, String password)
	{
		this.username = username;
		this.password = password;
	}
	
	/**
	 * This will set the clean session of the mqtt producer.
	 * If this method is not called the value used will be true.
	 * This method must be called before setUpClient(...)
	 * @param cleanSession
	 */
	public void setCleanSession(boolean cleanSession)
	{
		this.cleanSession = cleanSession;
	}
	
	/**
	 * This will set the qos of the messages sent by the producer.
	 * If this method is not called the value used will be 0
	 * This method must be called before setUpClient(...)
	 * @param qos
	 */
	public void setQos(int qos)
	{
		this.qos = qos;
	}
	
	/**
	 * This method sets the number of messages the producer will send.
	 * If this method is not called then 1000000 will be sent.
	 * This method must be called before setUpClient(...)
	 * @param number
	 */
	public void numberMessages(int number)
	{
		this.numberMsgs = number;
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
	 * This method will configure the producer to send retained messages
	 * to the server. If this is not called then no retained messages will be
	 * sent. The interval number determines when a retained message is sent, for
	 * example 0 means send every messages as retained, 10 means sent a retained
	 * message every 10th message. 
	 * This method must be called before setUpClient(...)
	 * @param interval
	 */
	public void setRetainedMessages(int interval)
	{
		this.retainedMessages = true;
		this.retainedInterval = interval;
	}
	
	

	/**
	 * This method will use the file name to log the trace information
	 * in this particular client. If this method is not called then no
	 * trace information will be logged
	 * This method must be called before setUpClient(...)
	 * @param fileName
	 */
	public void setTraceFile(String fileName) throws IOException
	{
		if(fileName != null)
		{
			File audit = new File(fileName);
			if(!audit.exists())
			{
				audit.createNewFile();
			}
			trace = new Audit(fileName, true);
		}
	}
	
	
	
	/**
	 * This method will configure the producer to fail if the connection to the system
	 * is broken. If you do not call this method then we will automatically reconnect
	 * to the server.
	 * This method must be called before setUpClient(...)
	 */
	public void disableRecoverSession()
	{
		recoverConnection = false;
	}
	
	/**
	 * Used to close the producer
	 */
	public void closeConnections()
	{
		try
		{
			
			if(producer != null && producer.isConnected())
			{
				producer.disconnect();
			}
			
		}catch(Exception e)
		{
			
		}
		
	}
	
	/**
	 * Used to indicate whether we are attempting a reconnection of the client
	 * @return
	 */
	public boolean isreconnecting()
	{
		return reconnecting;
	}
	
	/**
	 * Returns true if the producer is connected
	 * @return
	 */
	public boolean isConnected()
	{
		if(producer != null && producer.isConnected())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	
	/**
	 * This method is used to reconnect the client after a failure
	 */
	public synchronized void resetConnections()
	{
		if(!recoverConnection)
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Client " + clientID + " has been configured not to recover the connection so exiting the thread");
			}
			keepRunning=false;
			setFailMessage("An problem occurred when it was not expected as client is configured to be non-recoverable");
			return;
			
		}
		if(isreconnecting())
		{
			if(isreconnecting())
			{
				if(trace!= null && trace.traceOn())
				{		
					trace.trace("We are in the reset connections so return");
				}
			}
			return;
		}
		
		reconnecting = true;
		
		if(trace!= null && trace.traceOn())
		{
			trace.trace("In reset connections");
			trace.trace("Trying to connect again with cleanSession=" + cleanSession);
		}
				
		try
		{
			producer.connect(options);
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Producer was successfully reconnected");
			}
			
			reconnecting = false;
			if(cleanSession)
			{
				if(trace!= null && trace.traceOn())
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
			reconnecting = false;
			if(trace!= null && trace.traceOn())
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
	
	
	public void run() {
		
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Producer starting execution");
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
			
 				if(!isreconnecting() && ((producer != null && producer.isConnected())))
 				{
 					try
 					{
 						if((retainedMessages && (i!= 0 && i%retainedInterval == 0)))
 						{
 							// Create a retained message
 							messagebody = null;
 							if(i!= 0 && i%retainedInterval == 0)
 							{
 								if(trace!= null && trace.traceOn())
 								{
 									trace.trace("The retained count is at the required level so creating a retained message");
 								}
 								messagebody = retainedNumber + ".RETAINED." + UUID.randomUUID().toString().substring(0, 15) + "." + clientID;
 								MqttMessage message = new MqttMessage(messagebody.getBytes());	
 								message.setQos(2);
 								message.setRetained(true);
 								MqttToken token = topic.publish(message);	 	
 								if(messageIDKey!= null && storeSentMsgs)
 								{
 									msgID = token.getMessageId();
 									messageIDKey.put(token.getMessageId(), messagebody);
 								}
 								if(trace!= null && trace.traceOn())
 								{	
 									trace.trace("Producer published retained message " + messagebody +  "with ID " + msgID);
 								}
 							}
 							
 						}
 						else
 						{
 							// Send a non retained message
 							if(producer.isConnected() )
 							{
 								messagebody = clientID + ":" + (i);
 								MqttMessage message1 = new MqttMessage(messagebody.getBytes());			
 								message1.setQos(qos);
 								if(trace!= null && trace.traceOn())
 								{
 									trace.trace("Generating a new message with id=" + messagebody + " and qos " + qos);
 								}
 								
 								MqttToken token = topic.publish(message1);	
 								if(messageIDKey!= null && storeSentMsgs)
 								{
 									msgID = token.getMessageId();
 									messageIDKey.put(token.getMessageId(), messagebody);
 								}
 								if(trace!= null && trace.traceOn())
 								{	
 									trace.trace("Producer published message " + messagebody +  "with ID " + msgID);
 								}
 							}
 							
 						}
 					}
 					catch(Exception e)
 					{
 						i--; // decrement the message count
 						if(trace!= null && trace.traceOn())
 						{
 							trace.trace("An exception occurred " + e.getMessage());	
 						}
 						if(! this.recoverConnection)
 						{
 							System.out.println("A problem occurred and we are not set to reconnect");
 							e.printStackTrace();
 							System.exit(0);
 						}
 					}
 				}
 				else
 				{
 					i--;
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
	 * Return the client ID for this producer
	 * @return
	 */
	public String getClientID()
	{
		return clientID;
	}
	
	protected void setFailMessage(String message)
	{
		if(failMessage.equals(""))
		{
			failMessage = this.clientDetails + "FailMessage:";
		}
		this.failMessage =  failMessage + message + " : ";
	}
	
	/**
	 * Get any failure message - calling test can then decide what to do i.e. stop global test etc?
	 * @return
	 */
	public String getFailMessage()
	{
		return failMessage;
	}
	
	/**
	 * Used to configure this client to store the sent messages.
	 * This method must be called before setUpClient(...)
	 */
	public void storeSentMsgs()
	{
		this.storeSentMsgs = true;
	}
	
	/**
	 * Used to get the messages sent by this producer.
	 * @return
	 */
	public ArrayList<String> getSentMsgs()
	{
		return listener.getSentMsgs();
	}

}
