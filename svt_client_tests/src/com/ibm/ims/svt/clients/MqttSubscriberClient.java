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

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;

import com.ibm.ima.test.cli.imaserver.ImaHAServerStatus;
import com.ibm.ima.test.cli.imaserver.ImaServer;
import com.ibm.ima.test.cli.imaserver.ImaServerStatus;

/**
 * This class is used as an asynchronous callback for an MQTT consumer.
 * 
 *
 */
public class MqttSubscriberClient implements Runnable {
	
	protected MqttClient subscriber = null;
	private String clientID = UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	protected String topicName = null;
	protected MqttConnectOptions connOptions = new MqttConnectOptions();		
	private MqttSubscriberListener listener = null;
	protected int qos = 0;
	protected boolean reconnecting = false;
	protected MqttConnection mqttconnection = null;
	protected boolean duplicates = false;
	protected boolean recoverConnection = true;
	protected String username = null;
	protected String password = null;
	protected boolean cleanSession = true;
	protected ArrayList<RetainedMap> retainedMessages = new ArrayList<RetainedMap>();
	protected boolean retainedAware = false;
	protected boolean afterProducer = true;
	protected boolean checkRetainedDuplicates = false;
	protected String[] ipaddress = null;
	protected boolean regressionTest = false;
	protected ArrayList<String> regressionClientIDs=null;
	protected String regressionFailMessage = "";
	protected long throttleReceive = 0;
	protected boolean disableAutomaticReconnect = false;
	protected ArrayList<String> sharedSubMsgs = new ArrayList<String>();
	
	protected Audit trace = null;
	public boolean keepRunning = true;
	protected boolean disconnect = false;
	protected long diconnectTime = -1;
	protected long unsubscribeTime = -1;
	protected boolean storeReceivedMsgs = false;
	protected boolean checkMsgInterval = false;
	protected long checkMsgIntervalVal = 30000;
	protected long lastTime = System.currentTimeMillis();
	protected int maxMessagesToKeep = -1;
	protected int minMessagesToKeep = -1;
	protected String sharedPrefix = "$SharedSubscription";
	
	protected String clientDetails = null;
	protected ImaServer server = null;

	/**
	 * Used for standalone
	 * @param connection
	 * @throws Exception
	 */
	public MqttSubscriberClient(String connection) throws Exception
	{
		mqttconnection = new MqttConnection(connection);
		
	}
	
	/**
	 * Used for HA
	 * @param key
	 * @param urls
	 */
	public MqttSubscriberClient(String key, String[] urls)
	{
		mqttconnection = new MqttConnection(key);
		this.ipaddress = urls;
	}
	
	/**
	 * This must be called after the client has been configured but before the thread is started
	 * @param topicName
	 * @throws Exception
	 */
	public void setup(String topicName) throws Exception
	{
		// Set up the connections and producers etc
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Setting up a mqtt subscriber for topic " + topicName);
		}
		
		
		connOptions = new MqttConnectOptions();		
		connOptions.setCleanSession(cleanSession);
		connOptions.setKeepAliveInterval(60000);
	
		if(ipaddress != null)
		{
			// HA configuration
			connOptions.setServerURIs(ipaddress);
		}
		if(username!= null && password!=null)
		{
			connOptions.setUserName(username);
			connOptions.setPassword(password.toCharArray());
		}
		
		subscriber = mqttconnection.getClient(clientID);
		this.topicName = topicName;
		if(this.regressionTest)
		{
			// We want to correlate the messages arriving by each known client - the producer
			// must add clientID followed by sequence number into the message body
			listener = new MqttMessageListener(this, this.regressionClientIDs, this.cleanSession);
		}
		else
		{
			listener = new MqttSubscriberListener(this, duplicates, storeReceivedMsgs);
			
		}
		
		if(this.maxMessagesToKeep != -1)
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("The listener will store maximum of " + maxMessagesToKeep + " messages and keep a minimum of " + minMessagesToKeep);
			}
			listener.setMaxMessagesToKeep(maxMessagesToKeep, minMessagesToKeep);
		}

		if(retainedAware)
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("The subscriber will check for retained messages");
			}
			listener.checkRetainedMessages(retainedMessages, cleanSession, afterProducer, checkRetainedDuplicates);
		}
		
		if(this.throttleReceive != 0)
		{
			// Can be used to slow the message delivery down to make this a 'slow client'
			listener.throttleReceive(throttleReceive);
		}
		subscriber.setCallback(listener);
		subscriber.connect(connOptions);
		subscriber.subscribe(topicName, qos);
		if(this.disconnect)
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("The client has been configured to frequently disconnect for approx " + this.diconnectTime + " milliseconds");
			}
		}
		if(this.unsubscribeTime != -1)
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("The client has been configured to frequently unsubscribe for approx " + this.unsubscribeTime + " milliseconds");
			}
		}
		
		StringBuffer buf = new StringBuffer();
		buf.append("Single MQTT Consumer: \n");
		buf.append("Clientid:"+this.clientID + "\n");
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
	 * This method is used to specify the list of publisher IDs.
	 * The method must be called before setup(...)
	 * @param regressionClientIDs
	 */
	public void setRegressionListener(ArrayList<String> regressionClientIDs)
	{
		this.regressionTest = true;
		this.regressionClientIDs = regressionClientIDs;
	}
	
	/**
	 * Returns the list of retained messages if we have been configured to check and store retained messages
	 * @return
	 */
	public ArrayList<RetainedMap> getRetainedMessages()
	{
		return retainedMessages;
	}
	
	/**
	 * This method configures the subscriber to check for retained messages. If this is
	 * not called then we will not check for retained messages
	 * @afterProducer - this indicates whether we connected after the producer started i.e.
	 * do we expect to see our first retained messages marked as retained
	 * The method must be called before setup(...)
	 */
	public void checkRetainedMessages(boolean afterProducer, boolean checkRetainedDuplicates)
	{
		this.retainedAware = true;
		this.afterProducer = afterProducer;
		this.checkRetainedDuplicates = checkRetainedDuplicates;
	}
	
	
	/**
	 * This method enables the client to check for duplicate messages. Should
	 * only be used for QOS2 messages
	 * The method must be called before setup(...)
	 */
	public void checkDuplicates()
	{
		duplicates = true;
	}
	
	/**
	 * This method configures the client to check that we received a message every x milliseconds.
	 * The method must be called before setup(...)
	 * @param interval
	 */
	public void checkMsgInterval(long interval)
	{
		this.checkMsgInterval = true;
		this.checkMsgIntervalVal = interval;
	}
	
	/**
	 * Used to specify that the client checks that we continue to receive messages.
	 * Must be called before the setup(...) method.
	 * We use the ipaddress for the server to check if this is a planned outage - for example
	 * if the server is being stressed by millions of unacked messages an imaserver stop can 
	 * take several minutes during which time no messages will be received.
	 * @param interval
	 */
	public void checkMsgInterval(long interval, String cliaddressA, String cliaddressB)
	{
		this.checkMsgInterval = true;
		this.checkMsgIntervalVal = interval;
		
		if(cliaddressB != null)
		{
			server = new ImaHAServerStatus(cliaddressA, cliaddressB, "admin", "admin");
		}
		else
		{
			server = new ImaServerStatus(cliaddressA, "admin", "admin");
		}
	}
	
	/**
	 * Set the username and password if this is required on the connection
	 * The method must be called before setup(...)
	 * @param username
	 * @param password
	 */
	public void setUserAndPassword(String username, String password)
	{
		this.username = username;
		this.password = password;
	}
	
	/**
	 * This will set the clean session of the mqtt client.
	 * If this method is not called the value used will be true.
	 * The method must be called before setup(...)
	 * @param cleanSession
	 */
	public void setCleanSession(boolean cleanSession)
	{
		this.cleanSession = cleanSession;
	}
	
	/**
	 * This will set the qos for the client.
	 * If this method is not called the value used will be 0
	 * The method must be called before setup(...)
	 * @param qos
	 */
	public void setQos(int qos)
	{
		this.qos = qos;
	}
	
	
	/**
	 * This method will configure the subscriber to fail if the connection to the system
	 * is broken. If you do not call this method then we will automatically reconnect
	 * to the server.
	 * The method must be called before setup(...)
	 */
	public void disableRecoverSession()
	{
		recoverConnection = false;
	}
	
	/**
	 * Returns the client ID	
	 * @return
	 */
	public String getClientId()
	{
		return clientID;
	}
	
	
	/**
	 * This method will use the file name to log the trace information
	 * in this particular client. If this method is not called then no
	 * trace information will be logged
	 * The method must be called before setup(...)
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
	 * Used to determine if the client is attempting to reconnect
	 * @return
	 */
	public boolean isreconnecting()
	{
		return reconnecting;
	}
	
	/**
	 * Used to attempt to reconnect the client
	 */
	public synchronized void resetConnections()
	{
		
		if(!recoverConnection)
		{
			System.out.println("A problem occurred and we are not set to reconnect so exiting");
			System.exit(0);
		}
		if(isreconnecting() || ((subscriber != null && subscriber.isConnected())))
		{
			if(isreconnecting())
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("We are in the resetConnections and already reconnecting so return");
				}
			}
			else
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("We are in the resetConnections and the subscriber is still connected so return");
				}
			}
			return;
		}
		if(disableAutomaticReconnect)
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("We have disabled automaticReconnection");
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
					trace.trace("Attempting to reconnect subscriber");
				}
				subscriber.setCallback(listener);
				subscriber.connect(connOptions);
				
				if(retainedAware && !cleanSession)
				{
					// 	do not subscribe otherwise we get two retained messages
				}
				else
				{
					subscriber.subscribe(topicName, qos);
				}
				
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Subscriber successfully reconnected");
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
					Thread.sleep(10000);
				}
				catch(Exception es)
				{
					// 	do nothing
				}				
			}
		}while(!subscriber.isConnected());
		
	}
	
	/**
	 * Close the subscriber
	 */
	public void closeConnections()
	{
		try
		{
			if(subscriber != null && subscriber.isConnected())
			{
				subscriber.unsubscribe(topicName);
				subscriber.disconnect();
			}
		}
		catch(Exception e)
		{
			// do nothing
		}

	}
	
	/**
	 * Returns true if connected
	 * @return
	 */
	public boolean isConnected()
	{
		if(subscriber != null && subscriber.isConnected())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	
	public void run() {
		
		
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Subscriber starting execution");
		}
		while(keepRunning)
		{
			try
			{
				if(!this.reconnecting)
				{
					if(this.disconnect) // make sure we leave time between disconnects
					{
						Thread.sleep(this.diconnectTime *2); 
					}
					else if(this.unsubscribeTime != -1)
					{
						Thread.sleep(this.unsubscribeTime *2);
					}
					else
					{
						Thread.sleep(5000);
					}
					if((subscriber != null && subscriber.isConnected()))
					{
						if(checkMsgInterval && (lastTime < (System.currentTimeMillis() - this.checkMsgIntervalVal )))
						{
							// Check we received a message within the specified time
							long receivedMessage = listener.getLastMessageTime();
							
							if(receivedMessage > lastTime)
							{
								// 	do nothing we got a messages
								lastTime = System.currentTimeMillis();
								if(trace!= null && trace.traceOn())
								{
									trace.trace("Checked message interval was ok");
									
								}
							}
							else
							{
								if(trace!= null && trace.traceOn())
								{
									trace.trace("Client " + this.clientID + " has been configured to check for messages every " + checkMsgIntervalVal + "milliseconds.");
									trace.trace("The last message arrived at " + receivedMessage + " but the time is " + lastTime);
								}
								StringBuffer buffer = new StringBuffer();
								buffer.append("MQTT Client " + this.clientID + " has been configured to check for messages every " + checkMsgIntervalVal + "milliseconds.");
								buffer.append("The last message arrived at " + receivedMessage + " but the time is " + lastTime);
								
								if(server == null)
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
						if(this.disconnect)
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("The subscriber was configured to disconnect for up to " + this.diconnectTime + " milliseconds");
								trace.trace("We will disable the reconnection attempts during this time");
							}
							reconnecting = true; // stop any reconnection attempts if the server comes and goes in the meantime
							subscriber.disconnect(120000);
							do
							{
								Thread.sleep(5000);			
							}while(subscriber.isConnected());
							
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
							subscriber.unsubscribe(this.topicName);
							Thread.sleep(this.unsubscribeTime);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Attempting to resubscribe to the topic");
							}
							subscriber.subscribe(this.topicName);
							lastTime = System.currentTimeMillis(); 
						}
					}
				}
									
			}		
			catch(Exception e)
			{
				if(!recoverConnection)
				{
					System.out.println("A problem occurred and we are not set to reconnect");
					e.printStackTrace();
					System.exit(0);
				}
			}
	
		
		}	
		if(trace!= null && trace.traceOn())
		{
			trace.trace("The thread was stopped so closing connections");
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
				trace.trace("Will leave the consumer up for debugging as we have noted a failure message");
			}
		}
		
		
	}
	
	/**
	 * Returns true if non durable or false if durable
	 * @return
	 */
	public boolean getCleanSession()
	{
		return cleanSession;
	}
	
	/**
	 * Used to set a failure message for this particular client
	 * @param message
	 */
	protected void failTest(String message)
	{
		keepRunning = false;
		if(regressionFailMessage.equals(""))
		{
			regressionFailMessage = this.clientDetails + "FailMessage:";
		}
		this.regressionFailMessage =  regressionFailMessage + message + " : ";
	}
	
	/**
	 * Used to return the failure message - the calling class can then determine what action to do
	 * @return
	 */
	public String getFailMessage()
	{
		return regressionFailMessage;
	}
	
	/**
	 * This can be used to clean this clients failure message
	 */
	public void clearFailMessage()
	{
		regressionFailMessage = "";
	}
	
	/**
	 * Configure the client to frequently disconnect for up to the time specified.
	 * The method must be called before setup(...)
	 * @param time
	 */
	public void disconnect(long time)
	{
		this.disconnect = true;
		this.diconnectTime = time;
	}
	
	
	/**
	 * Returns the received messages if configured to store them	
	 * @return
	 */
	public ArrayList<String> getReceivedMsgs()
	{
		return listener.getReceivedMsgs();
	}
	
	/**
	 * Clears the received messages
	 */
	public void clearReceivedMsgs()
	{
		listener.clearReceivedMsgs();
	}
	
	/**
	 * Configures the client to store messages
	 * The method must be called before setup(...)
	 */
	public void storeReceivedMsgs()
	{
		storeReceivedMsgs = true;
	}
	
	/**
	 * Override the client ID
	 * The method must be called before setup(...)
	 * @param clientID
	 */
	public void overrideClientID(String clientID)
	{
		this.clientID = clientID;
	}
	
	/**
	 * Configures the client to slow the delivery of messages down by the specified time
	 * The method must be called before setup(...)
	 * @param time
	 */
	public void throttleMessages(long time)
	{
		this.throttleReceive = time;
	}
	
	/**
	 * Returns the qos used by this client
	 * @return
	 */
	public int getQos()
	{
		return this.qos;
	}
	
	
	/**
	 * This is used to reduced the amount of messages to keep for any particular test.
	 * If the test is going to be run over several days we need to limit the stored
	 * messages otherwise we will run out of memory.
	 * This should be called before setup(...)
	 * @param maxNumber
	 * @param minNumber
	 */
	public void setMaxMessagesToKeep(int maxNumber, int minNumber)
	{
		this.maxMessagesToKeep = maxNumber;
		this.minMessagesToKeep = minNumber;
	}
	
	/**
	 * Disables automatic reconnection
	 */
	public void disableAutomaticReconnect()
	{
		disableAutomaticReconnect = true;
	}
	
	/**
	 * Configures the client to frequently unsubscribe for at least the specified time.
	 * The method must be called before setup(...)
	 * @param time
	 */
	public void unsubscribe(long time)
	{
		if(time > 0)
		{
			this.unsubscribeTime = time;
		}
	}
	
	
	/**
	 * Not implemented for single consumers threads
	 * @param msg
	 */
	public void checkSharedSubMsg(String msg)
	{
		if(trace!= null && trace.traceOn())
		{
			trace.trace("The checkSharedSubMsg is not implemented for this client");
			
		}
	}
	
	
}
