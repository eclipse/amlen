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

import java.util.UUID;

import javax.jms.Connection;
import javax.jms.JMSException;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.naming.NamingException;

import com.ibm.ima.jms.ImaSubscription;
import com.ibm.ima.jms.impl.ImaConnection;

/**
 * This class is used to contain multiple clients either with individual connections or a single connection
 *
 */
public class MultiJmsConsumerClient extends JmsConsumerClient implements Runnable {
	
	private int numberClients = 1;
	private Connection[] connections = null;
	private Session[] sessions = null;
	private MessageConsumer[] consumers = null;
	private JmsListener[] listeners = null;
	private JmsConsumerExceptionListener exceptionListener = null; 	
	
	private String[] clientIDs=null;
	private boolean useSameConnection = false;

	private int sleepTime = 5000;
	private int sleepMaxTime = 180000;
	
	private boolean disconnect = false;
	private long diconnectTime = -1;
	private long unsubscribeTime = -1;
	private boolean unsubscribeAll = true;
	
	/**
	 * Used to create consumers using JNDI lookup
	 * @param jndiLocation
	 * @param connectionFactoryName
	 * @throws NamingException
	 */
	public MultiJmsConsumerClient(String jndiLocation, String connectionFactoryName) throws NamingException
	{
		super(jndiLocation, connectionFactoryName);
	}
	
	/**
	 * Used to create MQ JMS consumers
	 * @param ipaddress
	 * @param port
	 * @param queueManager
	 * @param channel
	 * @throws NamingException
	 * @throws JMSException
	 */
	public MultiJmsConsumerClient(String ipaddress, int port, String queueManager, String channel) throws NamingException, JMSException
	{
		super(ipaddress, port, queueManager, channel);
	}
	
	/**
	 * Used to create IMA JMS consumers
	 * @param ipaddress
	 * @param port
	 * @throws NamingException
	 * @throws JMSException
	 */
	public MultiJmsConsumerClient(String ipaddress, int port) throws NamingException, JMSException
	{
		super(ipaddress, port);
	}
	
	/**
	 * This method should only be called once all the client configuration methods have been specified. 
	 *
	 */
	public void setup(String username, String password, JmsMessage.DESTINATION_TYPE destinationType, String destinationName)
	{
		int attemptSetup=5;
		int currentTry=0;
	
		try
		{
			// Set up the connections and subscriber etc
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Setting up " + this.numberClients + " consumers for destination " + destinationName);
			}
			
			if(this.useSameConnection)
			{
				connections = new Connection[1];
			}
			else
			{
				connections = new Connection[this.numberClients];
			}
								
			if(username != null && password != null)
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Setting up the connection with user name " + username);
				}
				
				for(int i=0; i<connections.length; i++)
				{
					connections[i] = cf.createConnection(username, password);
				}
			}
			else
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Setting up the connection");
				}
				for(int i=0; i<connections.length; i++)
				{
					connections[i] = cf.createConnection();
				}
					
			}
			
			clientIDs = new String[this.numberClients];
			for(int i=0; i<connections.length; i++)
			{
				String clientID = UUID.randomUUID().toString().substring(0, 18);
				
				if(sharedSubscription && !this.useSameConnection)
				{
					clientID = "NOT IN USE";
				}
				else
				{
					connections[i].setClientID(clientID);
				}
					
				clientIDs[i] = clientID;
			}
				
			sessions = new Session[this.numberClients];
			
			if(this.useSameConnection)
			{
				for(int i=0; i<sessions.length; i++)
				{
					sessions[i] = connections[0].createSession(transactedSession, ackMode);
				}
			}
			else
			{
				for(int i=0; i<sessions.length; i++)
				{
					sessions[i] = connections[i].createSession(transactedSession, ackMode);
				}
			}
			this.destinationName = destinationName;
			this.destinationType = destinationType;
			this.username = username;
			this.password = password;
			
					
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Session is transacted=" + transactedSession + " with ackMode=" + ackMode);
			}
			
			consumers = new MessageConsumer[this.numberClients];
			
			if(durable)
			{
				if(!sharedSubscription)
				{
					subID = UUID.randomUUID().toString().substring(0, 10);
				}
			}
			
			for(int i=0; i<consumers.length; i++)
			{
				if(durable)
				{
					if(destinationType.equals(JmsMessage.DESTINATION_TYPE.TOPIC))
					{
						
						if(selector != null)
						{
								if(sharedSubscription)
								{
									consumers[i] = ((ImaSubscription)(sessions[i])).createSharedDurableConsumer(sessions[i].createTopic(destinationName), subID, selector);
									if(trace!= null && trace.traceOn())
									{
										trace.trace("Created shared durable consumer with selector " + selector +  " and subID " + subID);
									}
								}
								else
								{
									consumers[i] = sessions[i].createDurableSubscriber(sessions[i].createTopic(destinationName), subID, selector, false);
									if(trace!= null && trace.traceOn())
									{
										trace.trace("Created durable consumer with selector " + selector +  " and subID " + subID);
									}
								}
						}
						else
						{
							if(sharedSubscription)
							{
								consumers[i] = ((ImaSubscription)(sessions[i])).createSharedDurableConsumer(sessions[i].createTopic(destinationName), subID);
								if(trace!= null && trace.traceOn())
								{
									trace.trace("Created shared durable consumer with subID " + subID);
								}
							}
							else
							{
								consumers[i] = sessions[i].createDurableSubscriber(sessions[i].createTopic(destinationName), subID);
								if(trace!= null && trace.traceOn())
								{
									trace.trace("Created durable consumer with subID " + subID);
								}
							}
						}
						
					}
					else if(destinationType.equals(JmsMessage.DESTINATION_TYPE.QUEUE))
					{
						if(trace!= null && trace.traceOn())
						{
							trace.trace("Cannot create a durable subscriber for a queue");
						}
						System.exit(0);
					}
				}
				else
				{
					if(destinationType.equals(JmsMessage.DESTINATION_TYPE.TOPIC))
					{
						if(selector != null)
						{
							if(sharedSubscription)
							{
								consumers[i] = ((ImaSubscription)(sessions[i])).createSharedConsumer(sessions[i].createTopic(destinationName), subID, selector);
								if(trace!= null && trace.traceOn())
								{
									trace.trace("Created shared consumer with selector " + selector);
								}
							}
							else
							{
								consumers[i] = sessions[i].createConsumer(sessions[i].createTopic(destinationName), selector);
								if(trace!= null && trace.traceOn())
								{
									trace.trace("Created consumer with selector " + selector);
								}
							}
						}
						else
						{
							if(sharedSubscription)
							{
								consumers[i] = ((ImaSubscription)(sessions[i])).createSharedConsumer(sessions[i].createTopic(destinationName), subID);
								if(trace!= null && trace.traceOn())
								{
									trace.trace("Created shared consumer");
								}
							}
							else
							{
								consumers[i] = sessions[i].createConsumer(sessions[i].createTopic(destinationName));
								if(trace!= null && trace.traceOn())
								{
									trace.trace("Created consumer");
								}
							}
						}
					}
					else if(destinationType.equals(JmsMessage.DESTINATION_TYPE.QUEUE))
					{
						if(selector != null)
						{
							consumers[i] = sessions[i].createConsumer(sessions[i].createQueue(destinationName), selector);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Created consumer with selector " + selector);
							}
						}
						else
						{
							consumers[i] = sessions[i].createConsumer(sessions[i].createQueue(destinationName));
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Created consumer");
							}
						}
					}
					if(trace!= null && trace.traceOn())
					{
						trace.trace("Created non-durable subscriber");
					}
				}
			}
			
			exceptionListener = new JmsConsumerExceptionListener(this); 
			connections[0].setExceptionListener(exceptionListener);
			listeners = new JmsListener[this.numberClients];
			
			// create the listeners
			this.createListeners();
			
			StringBuffer buf = new StringBuffer();
			buf.append("Multi JMS Consumer: \n");
			for(int i=0; i<clientIDs.length; i++)
			{
				buf.append("Clientid:"+ clientIDs[i]+ "\n");
			}
			buf.append("Destination:"+this.destinationName + "\n");
			buf.append("Durable:"+this.durable + "\n");
			buf.append("SingleConnection:"+ this.useSameConnection + "\n");
			buf.append("SharedSub:"+this.sharedSubscription + " SubID:" + this.subID + "\n");
			buf.append("Selector:"+this.selector + "\n");
			buf.append("Transacted:" + this.transactedSession + " AckMod: " + this.ackMode + " Number:" + this.numMessageWaitToAckCommit + "\n");
			if(this.disconnect)
			{
				buf.append("DisconnectTime:" + this.diconnectTime+ "\n");
			}
			if(this.unsubscribeTime != -1)
			{
				buf.append("UnSubscribeTime:" + this.unsubscribeTime + " UnSubscribeAll:" + this.unsubscribeAll + "\n");
			}
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
			
			this.clientDetails = buf.toString();
			
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Set up complete for " + clientDetails);
			}
			
		}
		catch(Exception e)
		{
			e.printStackTrace();
			if(currentTry < attemptSetup)
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("We encountered an exception whilst setting up the consumer. " + e.getMessage());
					trace.trace("We will sleep for 20 seconds and then attempt connection again");
				}
				try {
					Thread.sleep(20000);
				} catch (InterruptedException e1) {
					
					// do nothing
				}
				currentTry++;
				this.setup(username, password, destinationType, destinationName);
				
			}
			else
			{
				throw new RuntimeException(e);
			}
		}
		
		
	}
	
	/**
	 * This method creates the correc listener for each transaction method
	 * @throws JMSException
	 */
	private void createListeners() throws JMSException
	{
		if(transactedSession)
		{
			for(int j=0; j<consumers.length; j++)
			{
				JmsListener listener = new JmsListener(this, transactedSession, numMessageWaitToAckCommit, storeReceivedMsgs);
				listener.setID(j);
				consumers[j].setMessageListener(listener);
				listeners[j] = listener;
			}
		}
		else if(ackMode == Session.CLIENT_ACKNOWLEDGE)
		{
			for(int j=0; j<consumers.length; j++)
			{
				JmsListener listener = new JmsListener(this, numMessageWaitToAckCommit, storeReceivedMsgs);
				listener.setID(j);
				consumers[j].setMessageListener(listener);
				listeners[j] = listener;
			}
		}
		else
		{
			for(int j=0; j<consumers.length; j++)
			{
				JmsListener listener = new JmsListener(this, storeReceivedMsgs);
				listener.setID(j);
				consumers[j].setMessageListener(listener);
				listeners[j] = listener;
			}
		}
		
		if(this.sharedSubscription && this.checkSharedSubDuplicates)
		{
			for(int j=0; j<listeners.length; j++)
			{
				listeners[j].checkSharedSubMsgs();
			}
				
		}
	}
	
	/**
	 * Used to specify that all the clients use the same connection object
	 * This method must be called before setup(...)
	 */
	public void useSameConnection()
	{
		this.useSameConnection = true;
	}
	
	/**
	 * Used to specify the number of clients to create
	 * This method must be called before setup(...)
	 * @param clientNum
	 */
	public void numberClients(int clientNum)
	{
		this.numberClients = clientNum;
	}
	
	/**
	 * Used to unsubscribe and close all the clients 
	 */
	public void closeConnections()
	{
		for(int i=0; i<consumers.length; i++)
		{
			if(durable)
			{
				try
				{
					Thread.sleep(5000);
					if(trace!= null && trace.traceOn())
					{
						trace.trace("Unsubscribed from durable subscription");
					}
					sessions[i].unsubscribe(subID);
				}
				catch(Exception e)
				{
				// 	do nothing
				}
			}
			try
			{
				consumers[i].close();
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Consumer closed");
				}
			}
			catch(Exception e)
			{
			// 	do nothing
			}
			try
			{
				connections[i].close();
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Connection closed");
				}
			}
			catch(Exception e)
			{
			// 	do nothing
			}
			
		}
	}
	


	/**
	 * This code is used to recover the clients if the server if down for any reason.
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
		for(int i=0; i<connections.length; i++)
		{
			if(connections[i] instanceof ImaConnection)
			{
				if((Boolean)((ImaConnection)connections[i]).get("isClosed"))
				{
					if(trace!= null && trace.traceOn())
					{
						trace.trace("We found a closed connection");
					}
					allConnected = false;;
				}
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
		if(!keepRunning)
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("We have stopped the thread so exiting resetConnections");
			}
			return;
		}
		reconnecting = true;
		
		
		do
		{

			// lets free everything up
			for(int i=0; i<consumers.length; i++)
			{
				consumers[i] = null;
			}
			for(int i=0; i<connections.length; i++)
			{
				connections[i] = null;
			}
			
			for(int i=0; i<sessions.length; i++)
			{
				sessions[i] = null;
			}
			
			for(int i=0; i<listeners.length; i++)
			{
				listeners[i] = null;
			}
		
			try
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Resetting the connections after a failed attempt");
				}
				
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Closing all connections");
				}
				
				for(int i=0; i<connections.length; i++)
				{
					try
					{
						connections[i].close();
					}
					catch(Exception e)
					{
						// 	do nothing
					}
				}
				
				if(username != null && password != null)
				{
					if(trace!= null && trace.traceOn())
					{
						trace.trace("Setting up the connection with user name " + username);
					}
					
					for(int i=0; i<connections.length; i++)
					{
						connections[i] = cf.createConnection(username, password);
					}
				}
				else
				{
					if(trace!= null && trace.traceOn())
					{
						trace.trace("Setting up the connection");
					}
					for(int i=0; i<connections.length; i++)
					{
						connections[i] = cf.createConnection();
					}
					
				}
				for(int i=0; i<connections.length; i++)
				{
					
					if(sharedSubscription && !this.useSameConnection)
					{
						
					}
					else
					{
						connections[i].setClientID(clientIDs[i]);
					}
					
				}
				
						
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Recreated connections");
				}
				
				if(this.useSameConnection)
				{
					for(int i=0; i<sessions.length; i++)
					{
						sessions[i] = connections[0].createSession(transactedSession, ackMode);
					}
				}
				else
				{
					for(int i=0; i<sessions.length; i++)
					{
						sessions[i] = connections[i].createSession(transactedSession, ackMode);
					}
				}
				
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Recreated sessions which was transacted=" + transactedSession + " with ackMode=" + ackMode);
				}
				
				for(int i=0; i<consumers.length; i++)
				{
					if(durable)
					{
						if(destinationType.equals(JmsMessage.DESTINATION_TYPE.TOPIC))
						{
							
							if(selector != null)
							{
									if(sharedSubscription)
									{
										consumers[i] = ((ImaSubscription)(sessions[i])).createSharedDurableConsumer(sessions[i].createTopic(destinationName), subID, selector);
										if(trace!= null && trace.traceOn())
										{
											trace.trace("Created shared durable consumer with selector " + selector +  " and subID " + subID);
										}
									}
									else
									{
										consumers[i] = sessions[i].createDurableSubscriber(sessions[i].createTopic(destinationName), subID, selector, false);
										if(trace!= null && trace.traceOn())
										{
											trace.trace("Created durable consumer with selector " + selector +  " and subID " + subID);
										}
									}
							}
							else
							{
								if(sharedSubscription)
								{
									consumers[i] = ((ImaSubscription)(sessions[i])).createSharedDurableConsumer(sessions[i].createTopic(destinationName), subID);
									if(trace!= null && trace.traceOn())
									{
										trace.trace("Created shared durable consumer with subID " + subID);
									}
								}
								else
								{
									consumers[i] = sessions[i].createDurableSubscriber(sessions[i].createTopic(destinationName), subID);
									if(trace!= null && trace.traceOn())
									{
										trace.trace("Created durable consumer with subID " + subID);
									}
								}
							}
							
						}
						else if(destinationType.equals(JmsMessage.DESTINATION_TYPE.QUEUE))
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Cannot create a durable subscriber for a queue");
							}
							System.exit(0);
						}
					}
					else
					{
						if(destinationType.equals(JmsMessage.DESTINATION_TYPE.TOPIC))
						{
							if(selector != null)
							{		
								if(sharedSubscription)
								{	
									consumers[i] = ((ImaSubscription)(sessions[i])).createSharedConsumer(sessions[i].createTopic(destinationName), subID, selector);
									if(trace!= null && trace.traceOn())
									{
										trace.trace("Created shared consumer with selector " + selector);
									}
								}
								else
								{
									consumers[i] = sessions[i].createConsumer(sessions[i].createTopic(destinationName), selector);
									if(trace!= null && trace.traceOn())
									{
										trace.trace("Created consumer with selector " + selector);
									}
								}
							}
							else
							{
								if(sharedSubscription)
								{
									consumers[i] = ((ImaSubscription)(sessions[i])).createSharedConsumer(sessions[i].createTopic(destinationName), subID);
									if(trace!= null && trace.traceOn())
									{
										trace.trace("Created shared consumer");
									}	
								}	
								else
								{
									consumers[i] = sessions[i].createConsumer(sessions[i].createTopic(destinationName));
									if(trace!= null && trace.traceOn())
									{
										trace.trace("Created consumer");
									}
								}
							}	
						}
						else if(destinationType.equals(JmsMessage.DESTINATION_TYPE.QUEUE))
						{
							if(selector != null)
							{
								consumers[i] = sessions[i].createConsumer(sessions[i].createQueue(destinationName), selector);
								if(trace!= null && trace.traceOn())
								{
									trace.trace("Created consumer with selector " + selector);
								}
							}
							else
							{
								consumers[i] = sessions[i].createConsumer(sessions[i].createQueue(destinationName));
								if(trace!= null && trace.traceOn())
								{
									trace.trace("Created consumer");
								}
							}
						}
						if(trace!= null && trace.traceOn())
						{
							trace.trace("Created non-durable subscriber");
						}
					}	
				}	
				
				this.createListeners();
				
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Recreated consumers");
				}
				
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Starting connections");
				}
				
				for(int j=0; j<connections.length; j++)
				{
					connections[j].start();
				}
				
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Started connections");
				}
				lastTime = System.currentTimeMillis() + 240000;
				reconnecting = false;
				
				
			}
			catch(Exception e)
			{	
				if(trace!= null && trace.traceOn())
				{	
					trace.trace("Exception occurred when attempting to recreate the connections. Message=" + e.getMessage());
					trace.trace("Sleeping for a short time before trying again");
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
				catch(Exception ex)
				{
				// 	do nothing
				}
				
				
			}
		}while(reconnecting);
		
	}
	
	
	public void run() {
		
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Consumer starting execution");
		}
		for(int j=0; j<connections.length; j++)
		{
			try
			{
				connections[j].start();
			}
			catch(Exception e)
			{
				// do nothing
			}		
		}
		
		while(keepRunning)
		{
			
			try
			{
				if(!reconnecting)
				{
					if(this.disconnect)
					{
						Thread.sleep(this.diconnectTime *2); //wait before keep performing the repeated checks
					}
					else if(this.unsubscribeTime != -1)
					{
						Thread.sleep(this.unsubscribeTime *2);
					}
					else
					{
						Thread.sleep(20000); //wait 20 seconds check if we are all connected
					}
					boolean allConnected = true;
					for(int i=0; i<connections.length; i++)
					{
						if(connections[i] instanceof ImaConnection)
						{
							if((Boolean)((ImaConnection)connections[i]).get("isClosed"))
							{
								if(trace!= null && trace.traceOn())
								{
									trace.trace("We found a closed connection whilst running");
								}
								allConnected = false;
							}
						}
										
					}
					if(allConnected)
					{
						if(trace!= null && trace.traceOn())
						{
							trace.trace("We are all connected");
						}
						if(this.disconnect)
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("The subscriber was configured to disconnect for up to " + this.diconnectTime + " milliseconds");
								trace.trace("We will disable the reconnection attempts during this time");
							}
							reconnecting = true; // stop any reconnection attempts if the server comes and goes in the meantime
							for(int i=0; i<connections.length; i++)
							{
								try
								{
									connections[i].close();
								}
								catch(Exception e)
								{
									// 	do nothing
								}
								do
								{
									Thread.sleep(2000);			
								}while(! (Boolean)((ImaConnection)connections[i]).get("isClosed"));
							}
							if(trace!= null && trace.traceOn())
							{
								trace.trace("All connections have been closed so sleeping for the configured disconnect time");
							}
							Thread.sleep(this.diconnectTime);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Attempting to reconnect the subscriber after the disconnect");
							}
							reconnecting = false;
							resetConnections();
						}
						else if((this.unsubscribeTime != -1) && (! this.useSameConnection) && durable)
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("The subscriber was configured to unsubscribe for up to " + this.unsubscribeTime + " milliseconds");
								trace.trace("If the server goes down in this time we will automatically resubscribe");
							}
							for(int i=0; i<sessions.length; i++)
							{
								if(! unsubscribeAll)
								{
									if(i%2 == 0)
									{
										if(trace!= null && trace.traceOn())
										{
											trace.trace("Unsubscribing client " + clientIDs[i]);
										}
										connections[i].stop();
										consumers[i].close();
										sessions[i].unsubscribe(this.subID);
									}
									else
									{
										if(trace!= null && trace.traceOn())
										{
											trace.trace("Client " + clientIDs[i] +  " will remain subscribed");
										}
									}
								}
								else
								{
									if(trace!= null && trace.traceOn())
									{
										trace.trace("Unsubscribing client " + clientIDs[i]);
									}
									connections[i].stop();
									consumers[i].close();
									sessions[i].unsubscribe(this.subID);								
								}
								
							}	
							Thread.sleep(this.unsubscribeTime);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Attempting to resubscribe to the topic");
							}
							for(int i=0; i<sessions.length; i++)
							{
								if(! unsubscribeAll)
								{
									if(i%2 == 0)
									{
										if(trace!= null && trace.traceOn())
										{
											trace.trace("Resubscribing client " + clientIDs[i]);
										}
										this.resubscribe(sessions[i], i);
									}
								}
								else
								{
									if(trace!= null && trace.traceOn())
									{
										trace.trace("Resubscribing client " + clientIDs[i]);;
									}
									this.resubscribe(sessions[i], i);
								}
								
							}
							
							
						}
						else
						{
							if(checkMsgInterval && (lastTime < (System.currentTimeMillis() - this.checkMsgIntervalVal )))
							{
								for(int k=0; k<listeners.length; k++)
								{
									long receivedMessage = listeners[k].getLastMessageTime();
									if(receivedMessage > lastTime)
									{
										// 	do nothing we got a messages
										if(trace!= null && trace.traceOn())
										{
											trace.trace("Checked message interval was ok");
											
										}
									}
									else
									{
										if(trace!= null && trace.traceOn())
										{
											trace.trace("Client " + clientIDs[k] + " has been configured to check for messages every " + checkMsgIntervalVal + "milliseconds.");
											trace.trace("The last message arrived at " + receivedMessage + " but the time is " + lastTime);
										}
										StringBuffer buffer = new StringBuffer();
										buffer.append("Client " + clientIDs[k] + " has been configured to check for messages every " + checkMsgIntervalVal + "milliseconds.");
										buffer.append("The last message arrived at " + receivedMessage + " but the time is " + lastTime);
										
										
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
									lastTime = System.currentTimeMillis();
									
									
								}
								
							}	
							if(trace!= null && trace.traceOn())
							{
								trace.trace("We are already connected so sleep");
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
						else
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("We are not connected but already registered reconnecting so sleep");
							}
						}
					}
					
				}
				else
				{
					Thread.sleep(20000);	
				}
						
			}						
			catch(Exception e)
			{
				if(!reconnecting)
				{
					if(trace!= null && trace.traceOn())
					{
						trace.trace("Calling reset connections after an exception " + e.getMessage());
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
	 * Used to specify whether the clients will be frequently disconnected and reconnected during testing.
	 * This method must be called before setup(...)
	 * @param time
	 */
	public void disconnect(long time)
	{
		this.disconnect = true;
		this.diconnectTime = time;
	}
	
	/**
	 * Used to specify whether the clients will frequently unsubscribe and resubscribe. 
	 * This method must be called before setup(...)
	 * @param time
	 * @param all - if false we will just unsubscribe a few of the clients
	 */
	public void unsubscribe(long time, boolean all)
	{
		if(time > 0)
		{
			unsubscribeTime = time;
		}
		unsubscribeAll = all;
		
	}
	
	/**
	 * This method is used to resubscribe the specified client
	 * @param session
	 * @param i - indicates which connection/session to use
	 * @throws JMSException
	 */
	protected void resubscribe(Session session, int i) throws JMSException
	{
		connections[i].stop();
		
		if(durable)
		{
			if(destinationType.equals(JmsMessage.DESTINATION_TYPE.TOPIC))
			{
				
				if(selector != null)
				{
						if(sharedSubscription)
						{
							consumers[i] = ((ImaSubscription)(sessions[i])).createSharedDurableConsumer(sessions[i].createTopic(destinationName), subID, selector);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Recreated shared durable consumer with selector " + selector +  " and subID " + subID);
							}
						}
						else
						{
							consumers[i] = sessions[i].createDurableSubscriber(sessions[i].createTopic(destinationName), subID, selector, false);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Recreated durable consumer with selector " + selector +  " and subID " + subID);
							}
						}
				}
				else
				{
					if(sharedSubscription)
					{
						consumers[i] = ((ImaSubscription)(sessions[i])).createSharedDurableConsumer(sessions[i].createTopic(destinationName), subID);
						if(trace!= null && trace.traceOn())
						{
							trace.trace("Recreated shared durable consumer with subID " + subID);
						}
					}
					else
					{
						consumers[i] = sessions[i].createDurableSubscriber(sessions[i].createTopic(destinationName), subID);
						if(trace!= null && trace.traceOn())
						{
							trace.trace("Recreated durable consumer with subID " + subID);
						}
					}
				}
				
			}
		}
		else
		{
			if(destinationType.equals(JmsMessage.DESTINATION_TYPE.TOPIC))
			{
				if(selector != null)
				{
					if(sharedSubscription)
					{
						consumers[i] = ((ImaSubscription)(sessions[i])).createSharedConsumer(sessions[i].createTopic(destinationName), subID, selector);
						if(trace!= null && trace.traceOn())
						{
							trace.trace("Recreated shared consumer with selector " + selector);
						}
					}
					else
					{
						consumers[i] = sessions[i].createConsumer(sessions[i].createTopic(destinationName), selector);
						if(trace!= null && trace.traceOn())
						{
							trace.trace("Recreated consumer with selector " + selector);
						}
					}
				}
				else
				{
					if(sharedSubscription)
					{
						consumers[i] = ((ImaSubscription)(sessions[i])).createSharedConsumer(sessions[i].createTopic(destinationName), subID);
						if(trace!= null && trace.traceOn())
						{
							trace.trace("Recreated shared consumer");
						}
					}
					else
					{
						consumers[i] = sessions[i].createConsumer(sessions[i].createTopic(destinationName));
						if(trace!= null && trace.traceOn())
						{
							trace.trace("Recreated consumer");
						}
					}
				}
			}
			else if(destinationType.equals(JmsMessage.DESTINATION_TYPE.QUEUE))
			{
				if(selector != null)
				{
					consumers[i] = sessions[i].createConsumer(sessions[i].createQueue(destinationName), selector);
					if(trace!= null && trace.traceOn())
					{
						trace.trace("Recreated consumer with selector " + selector);
					}
				}
				else
				{
					consumers[i] = sessions[i].createConsumer(sessions[i].createQueue(destinationName));
					if(trace!= null && trace.traceOn())
					{
						trace.trace("Recreated consumer");
					}
				}
			}
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Recreated non-durable subscriber");
			}
		}
		
		consumers[i].setMessageListener(listeners[i]);
		connections[i].start();
	}
	

	/**
	 * Used by the listener class to commit the session when transacted. We have this method here
	 * so we can avoid passing the session to the listener.
	 */
	public void commitSession(int id)
	{
		try
		{
			sessions[id].commit();
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Session committed");
			}
		}
		catch(Exception e)
		{
			resetConnections();
		}
	}
	
	
	/**
	 * This will be slow so use with care  - used to check that we do not receive any duplicate messages in a shared sub
	 */
	protected void checkSharedSubMsg(String msg)
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
				// The problem is with this test running for a very long time we need to make sure we don't build up a massive
				// arraylist of messages - we need some way of removing the old ones
				if(this.maxMessagesToStore != -1 && sharedSubMsgs.size() > this.maxMessagesToStore)
				{
					if(this.minMessagesToStore != -1)
					{
						if(trace!= null && trace.traceOn())
						{
							trace.trace("Clearing the messages stored down to " + (this.minMessagesToStore-1));
						}
						sharedSubMsgs.subList(0, this.maxMessagesToStore - this.minMessagesToStore).clear(); // remove all but the last 100
					}
					else
					{
						if(trace!= null && trace.traceOn())
						{
							trace.trace("Clearing the messages stored down to 99 messages");
						}
						sharedSubMsgs.subList(0, this.maxMessagesToStore - 100).clear(); // remove all but the last 100
					}
				}
			}
		}
	}
	
}
