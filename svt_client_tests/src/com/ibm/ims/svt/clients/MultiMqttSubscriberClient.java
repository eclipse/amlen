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

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;

/**
 * Used to create multiple clients
 */
public class MultiMqttSubscriberClient extends MqttSubscriberClient implements Runnable {
	
	private MqttClient[] subscribers = null;
	private String[] clientIDs = null;
	private MqttSubscriberListener[] listeners = null;
	private int numberClients = 1;
	
	private boolean unsubscribeAll = false;	
	private int sleepTime = 5000;
	private int sleepMaxTime = 180000;
	
	/**
	 * Standalone
	 * @param connection
	 * @throws Exception
	 */
	public MultiMqttSubscriberClient(String connection) throws Exception
	{
		super(connection);
		
	}
	
	/**
	 * HA
	 * @param key
	 * @param urls
	 */
	public MultiMqttSubscriberClient(String key, String[] urls)
	{
		super(key, urls);
	}
	
	/**
	 * Used to specify the number of clients
	 * This method must be called before setup(...)
	 * @param clientNum
	 */
	public void numberClients(int clientNum)
	{
		this.numberClients = clientNum;
	}
	
	/**
	 * This should be called after the clients have been configured but before the thread is started
	 */
	public void setup(String topicName) throws Exception
	{
		// Set up the connections and producers etc
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Setting up " + this.numberClients + " mqtt subscribers for topic " + topicName);
		}
		
		this.topicName = topicName;
		connOptions = new MqttConnectOptions();		
		connOptions.setCleanSession(cleanSession);
		connOptions.setKeepAliveInterval(120000);	
		if(ipaddress != null)
		{
			connOptions.setServerURIs(ipaddress);
		}
		if(username!= null && password!=null)
		{
			connOptions.setUserName(username);
			connOptions.setPassword(password.toCharArray());
		}
		
		subscribers = new MqttClient[this.numberClients];
		clientIDs = new String[this.numberClients];
		listeners = new MqttSubscriberListener[this.numberClients];
		Random random = new Random();
		
		for(int i=0; i<numberClients; i++)
		{
			int randomNumber = random.nextInt(20 - 16) + 16;
			String clientID = UUID.randomUUID().toString().substring(0, randomNumber).replace('-', '_')+i;
			clientIDs[i] = clientID;
			subscribers[i] = mqttconnection.getClient(clientID);
			MqttSubscriberListener listener = new MqttSubscriberListener(this, duplicates, false);
			if(this.topicName.startsWith(sharedPrefix))
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Listener configured to check duplicates for shared sub");
				}
				listener.checkSharedSubMsgs();
			}
			subscribers[i].setCallback(listener);
			listeners[i] = listener;
			
		}
	
		
		
		for(int i=0; i<subscribers.length; i++)
		{
			subscribers[i].connect(connOptions);
			subscribers[i].subscribe(topicName, qos);
			
		}
		
		
		StringBuffer buf = new StringBuffer();
		buf.append("Multi MQTT Consumer: \n");
		for(int i=0; i<clientIDs.length; i++)
		{
			buf.append("Clientid:"+clientIDs[i] + "\n");
		}
		buf.append("Destination:"+this.topicName + "\n");
		buf.append("CleanSession:"+this.cleanSession + "\n");
		if(this.topicName.startsWith(this.sharedPrefix))
		{
			buf.append("SharedSub:true \n");
		}
		buf.append("Qos:" + this.qos + "\n");
		buf.append("Retained:" + this.retainedAware + "\n");
		if(this.regressionClientIDs != null)
		{
			buf.append("Listening for producers: ");
			for(int i=0; i< regressionClientIDs.size(); i++)
			{
				buf.append(regressionClientIDs.get(i) + ":");
			}
			buf.append("\n");
		}
		buf.append("CheckMsgInterval:" + this.checkMsgInterval + " CheckIntVal:" + this.checkMsgIntervalVal + "\n");
		if(this.disconnect)
		{
			buf.append("DisconnectTime:" + this.diconnectTime+ "\n");
		}
		if(this.unsubscribeTime != 1)
		{
			buf.append("UnSubscribeTime:" + this.unsubscribeTime + "\n");
		}
		if(this.throttleReceive != 0)
		{
			buf.append("ThrottleMessages:" +  this.throttleReceive + "\n");
		}
		
		this.clientDetails = buf.toString();
		
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Set up complete for " + clientDetails);
		}
	}
	

	/**
	 * Used to reconnect all the clients
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
		
		if(isAllConnected())
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("We are already connected so return");
			}
			return;
		}
		reconnecting = true;
		
		do
		{
			try
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Attempting to reconnect subscribers");
				}
				
				for(int i=0; i<subscribers.length; i++)
				{
					if(! subscribers[i].isConnected())
					{
						if(trace!= null && trace.traceOn())
						{
							trace.trace("Attempting to reconnect client " + subscribers[i].getClientId() +  " with topic " + topicName + " with clean session " + connOptions.isCleanSession());
							
						}
						subscribers[i].setCallback(listeners[i]);
						subscribers[i].connect(connOptions);
						subscribers[i].subscribe(topicName, qos);
					}
				}
				
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Subscribers reconnected");
				}
				
				
				// 	If checking messages received we need to make sure we give enough time before checking starts again
				lastTime = System.currentTimeMillis() + 240000;
				reconnecting = false;
				
				
			}
			catch(Exception e)
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("An exception occurred in the subscriber. Exception=" + e.getMessage());
					trace.trace("Sleeping for a bit and then will try again" + e.getMessage());
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
				}
				catch(Exception es)
				{
					// 	do nothing
				}
			}
		
		}while(! isAllConnected());
	}
	
	/**
	 * Close all the connections
	 */
	public void closeConnections()
	{
		for(int i=0; i<subscribers.length; i++)
		{
			try
			{
				subscribers[i].unsubscribe(topicName);
				subscribers[i].disconnect();
				
			}
			catch(Exception e)
			{
			// 	do nothing
			}
		}
	}
	
	
	
	public void run() {
		
		
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Subscribers starting execution");
		}
		lastTime = System.currentTimeMillis();
		while(keepRunning)
		{
			try
			{
				
				
				if(!this.isreconnecting())
				{
					if(this.disconnect)
					{
						Thread.sleep(this.diconnectTime *2); // make sure we leave enough time between disconnects
					}
					else if(this.unsubscribeTime != -1)
					{
						Thread.sleep(this.unsubscribeTime *2); // make sure we leave enough time between unsubscribes
					}
					else
					{
						Thread.sleep(5000); //wait 5 seconds and try and reconnect
					}
					
					if(isAllConnected())
					{
						if(trace!= null && trace.traceOn())
						{
							trace.trace("We are already connected");
						}
						if(this.disconnect)
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("The subscriber was configured to disconnect for up to " + this.diconnectTime + " milliseconds");
								trace.trace("We will disable the reconnection attempts during this time");
							}
							reconnecting = true; // stop any reconnection attempts if the server comes and goes in the meantime
							for(int i=0; i<subscribers.length; i++)
							{
								subscribers[i].disconnect(120000);
								do
								{
									Thread.sleep(2000);			
								}while(subscribers[i].isConnected());
							}							
							Thread.sleep(this.diconnectTime);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Attempting to reconnect the subscriber after the disconnect");
							}
							reconnecting = false;
							resetConnections();
						}
						else if(this.unsubscribeTime != -1)
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("The subscriber was configured to unsubscribe for up to " + this.unsubscribeTime + " milliseconds");
								trace.trace("If the server goes down in this time we will automatically resubscribe");
							}
							for(int i=0; i<subscribers.length; i++)
							{
								if(! unsubscribeAll)
								{
									if(i%2 == 0)
									{
										if(trace!= null && trace.traceOn())
										{
											trace.trace("Unsubscribing client " + subscribers[i].getClientId());
										}
										subscribers[i].unsubscribe(this.topicName);
									}
									else
									{
										if(trace!= null && trace.traceOn())
										{
											trace.trace("Client " + subscribers[i].getClientId() +  " will remain subscribed");
										}
									}
								}
								else
								{
									if(trace!= null && trace.traceOn())
									{
										trace.trace("Unsubscribing client " + subscribers[i].getClientId());
									}
									subscribers[i].unsubscribe(this.topicName);									
								}
								
							}	
							Thread.sleep(this.unsubscribeTime);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Attempting to resubscribe to the topic");
							}
							for(int i=0; i<subscribers.length; i++)
							{
								if(! unsubscribeAll)
								{
									if(i%2 == 0)
									{
										if(trace!= null && trace.traceOn())
										{
											trace.trace("Resubscribing client " + subscribers[i].getClientId());
										}
										subscribers[i].subscribe(this.topicName);
									}
								}
								else
								{
									if(trace!= null && trace.traceOn())
									{
										trace.trace("Resubscribing client " + subscribers[i].getClientId());
									}
									subscribers[i].subscribe(this.topicName);
								}
								
							}	
						
						}
						else
						{
							if(checkMsgInterval && (lastTime < (System.currentTimeMillis() - this.checkMsgIntervalVal)))
							{
								for(int k=0; k<listeners.length; k++)
								{
									long receivedMessage = listeners[k].getLastMessageTime();
										
									if(receivedMessage > lastTime)
									{
										// 	do nothing we got a messages
										if(trace!= null && trace.traceOn())
										{
											trace.trace("Checked message interval was ok for listener for client " + clientIDs[k]) ;
												
										}
									}
									else
									{
										if(trace!= null && trace.traceOn())
										{
											trace.trace("We have been configured to check for messages every " + checkMsgIntervalVal + "milliseconds.");
											trace.trace("The last message for the client " + clientIDs[k] + " listener arrived at " + receivedMessage + " but the time is " + lastTime);
										}
										StringBuffer buffer = new StringBuffer();
										buffer.append("We have been configured to check for messages every " + checkMsgIntervalVal + "milliseconds.");
										buffer.append("The last message for the client " + clientIDs[k] + " listener arrived at " + receivedMessage + " but the time is " + lastTime);
										

										if(server== null)
										{
											keepRunning = false;
										}
										else
										{
											server.connectToServer();
											if(server.isStatusUnknown() || server.isStatusFailover())
											{
												if(trace!= null && trace.traceOn())
												{
													trace.trace(buffer.toString());
													trace.trace("We encountered the issue above but we were configured to continue on unknown server staue during failover");
												}
											}
											else
											{
												if(trace!= null && trace.traceOn())
												{
													trace.trace("The server status was not in an unknown failover state therefore we are failing the test");
												}
												failTest(buffer.toString());
												keepRunning = false;
											}
											server.disconnectServer();
										}
										
									}
									
								}
								lastTime = System.currentTimeMillis();
							}
						}
					}
					else
					{
						if(!reconnecting)
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Calling reset connections");
							}
							this.resetConnections();
						}
					}
				}
				
					
			}		
			catch(Exception e)
			{
				if(!reconnecting)
				{
					if(trace!= null && trace.traceOn())
					{
						trace.trace("An exceception occurred: " + e.getMessage());
						trace.trace("Calling reset connections");
					}
					this.resetConnections();
				}
			}
	
		
		}
		if(this.getFailMessage().equals(""))
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("The thread was stopped with no issues reported in fail messaeg so closing connections");
			}
			this.closeConnections();
		}
		else
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Will leave the consumers up for debugging as we have noted a failure message");
			}
		}
		
	}
	
	/**
	 * Used to determine if all the clients are still connected
	 * @return
	 */
	public boolean isAllConnected()
	{
		boolean allConnected = true;
		for(int i=0; i<subscribers.length; i++)
		{
			if(!subscribers[i].isConnected())
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("We found a closed connected");
				}
				allConnected = false;;
			
			}
		
		}
		return allConnected;
	}
	
	/**
	 * Returns true is non durable or false is durable
	 */
	public boolean getCleanSession()
	{
		return cleanSession;
	}

	/**
	 * Configures the clients to frequently disconnect by the specified time.
	 * This method must be called before setup(...)
	 */
	public void disconnect(long time)
	{
		this.disconnect = true;
		this.diconnectTime = time;
	}
	
	/**
	 * Configures the clients to frequently unsubscribe by the specified time
	 * This method must be called before setup(...)
	 * @param time
	 * @param all - if false only some of the clients will unsubscribe
	 */
	public void unsubscribe(long time, boolean all)
	{
		if(time > 0)
		{
			this.unsubscribeTime = time;
		}
		unsubscribeAll = all;
		
	}
	
	/**
	 * This method is used for shared subscriptions. It will check that 
	 * we do not receive any duplicate messages across shared subscribers.
	 * @param msg
	 */
	public void checkSharedSubMsg(String msg)
	{
		if(this.qos == 2)
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Checking for duplicates for a shared subscription");
			}
			synchronized(sharedSubMsgs)
			{
				if(sharedSubMsgs.contains(msg))
				{
					if(trace!= null && trace.traceOn())
					{
						trace.trace("We got a duplicate message for a shared subscription " + msg);
					}
					failTest("We got a duplicate message for a shared subscription " + msg);
				}
				else
				{
					sharedSubMsgs.add(msg);
					if(trace!= null && trace.traceOn())
					{
						trace.trace("Passed the duplicate test");
					}
					// 	The problem is with this test running for a very long time we need to make sure we don't build up a massive
					// 	arraylist of messages - we need some way of removing the old ones
					if(this.maxMessagesToKeep != -1 && sharedSubMsgs.size() > this.maxMessagesToKeep)
					{
						if(this.minMessagesToKeep != -1)
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Clearing the messages stored down to " + (minMessagesToKeep-1));
							}
							sharedSubMsgs.subList(0, maxMessagesToKeep - minMessagesToKeep).clear(); // remove all but the last 100
						}
						else
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Clearing the messages stored down to 99 messages");
							}
							sharedSubMsgs.subList(0, maxMessagesToKeep - 100).clear(); // remove all but the last 100
						}
					}
					
				}
			}
			
		}
		else
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Bypassing the duplicate flag as we are qos " + this.qos);
			}
		}
	}
	
}
	
