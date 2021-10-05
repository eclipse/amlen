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

import java.util.Iterator;
import java.util.Random;
import java.util.Set;
import java.util.UUID;

import javax.jms.Connection;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.naming.NamingException;

import com.ibm.ima.jms.impl.ImaConnection;

/**
 * This class is used to contain multiple clients either with individual connections or a single connection
 *
 */
public class MultiJmsProducerClient extends JmsProducerClient implements Runnable{
	
	private int numberClients = 1;
	private boolean useSameConnection = false;
	private Connection[] connections = null;
	private Session[] sessions = null;
	private MessageProducer[] producers = null;
	private String[] clientIDs = null;
	private JmsProducerExceptionListener listener = null;
	private int messageSize = 1;

	private int sleepTime = 5000;
	private int sleepMaxTime = 350000;
	
	/**
	 * Used to create clients using a JNDI lookup
	 * @param jndiLocation
	 * @param connectionFactoryName
	 * @throws NamingException
	 */
	public MultiJmsProducerClient(String jndiLocation, String connectionFactoryName) throws NamingException
	{
		super(jndiLocation, connectionFactoryName);
	}
	
	/**
	 * Used to create MQ clients
	 * @param ipaddress
	 * @param port
	 * @param queueManager
	 * @param channel
	 * @throws NamingException
	 * @throws JMSException
	 */
	public MultiJmsProducerClient(String ipaddress, int port, String queueManager, String channel) throws  NamingException, JMSException
	{
		super(ipaddress, port, queueManager, channel);
	}
	
	/**
	 * Used to create IMA clients
	 * @param ipaddress
	 * @param port
	 * @throws NamingException
	 * @throws JMSException
	 */
	public MultiJmsProducerClient(String ipaddress, int port) throws NamingException, JMSException
	{
		super(ipaddress, port);
	}
	

	/**
	 * This is used to specify a transacted session and the number of messages to send before committing the
	 * session.
	 * This method must be called before setup(...)
	 */
	public void setTransactedSession(int intervalNumber)
	{
		this.transactedSession = true;
		this.transactedNum = intervalNumber;
	}
	
	/**
	 * This method sets the size of the message to send
	 * This method must be called before setup(...)
	 * @param size
	 */
	public void setMessageSizeKB(int size)
	{
		this.messageSize = size;
	}

	/**
	 * This method specifies the number of clients to create
	 * This method must be called before setup(...)
	 * @param clientNum
	 */
	public void numberClients(int clientNum)
	{
		this.numberClients = clientNum;
	}
	
	/**
	 * This method specifies that the clients use the same connection object
	 * This method must be called before setup(...)
	 */
	public void useSameConnection()
	{
		this.useSameConnection = true;
	}
	
	/**
	 * This method should only be called once all the client configuration methods have been specified. 
	 *
	 */
	public void setup(String username, String password, String destinationName, JmsMessage.DESTINATION_TYPE destinationType)
	{
		int attemptSetup=5;
		int currentTry=0;
		
		try
		{
			// Set up the connections and producers etc
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Setting up " + this.numberClients + " producers for destination " + destinationName);
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
				String clientID = UUID.randomUUID().toString().substring(0, 19);
				connections[i].setClientID(clientID);
				clientIDs[i] = clientID;
			}
			
			sessions = new Session[this.numberClients];
			
			if(this.useSameConnection)
			{
				for(int i=0; i<sessions.length; i++)
				{
					sessions[i] = connections[0].createSession(transactedSession, Session.AUTO_ACKNOWLEDGE);
				}
			}
			else
			{
				for(int i=0; i<sessions.length; i++)
				{
					sessions[i] = connections[i].createSession(transactedSession, Session.AUTO_ACKNOWLEDGE);
				}
			}
		
			this.destinationType = destinationType;
			this.destinationName = destinationName;
			
			producers = new MessageProducer[this.numberClients];
			if(destinationType.equals(JmsMessage.DESTINATION_TYPE.QUEUE))
			{
				for(int i=0; i<producers.length; i++)
				{
					producers[i] = sessions[i].createProducer(sessions[i].createQueue(destinationName));
					producers[i].setDeliveryMode(persistence);
				}
				
			}
			else if(destinationType.equals(JmsMessage.DESTINATION_TYPE.TOPIC))
			{
				for(int i=0; i<producers.length; i++)
				{
					producers[i] = sessions[i].createProducer(sessions[i].createTopic(destinationName));
					producers[i].setDeliveryMode(persistence);
				}
			}
			
			listener = new JmsProducerExceptionListener(this); // will just set this to the first producer
			connections[0].setExceptionListener(listener);
			
			
			StringBuffer buf = new StringBuffer();
			buf.append("Multi JMS Producer: \n");
			for(int i=0; i<clientIDs.length; i++)
			{
				buf.append("Clientid:"+clientIDs[i] + "\n");
			}			
			buf.append("Destination:"+this.destinationName + "\n");
			buf.append("Peristent:"+this.persistence + "\n");
			if(this.properties != null)
			{
				buf.append("HeaderProperties: ");
				Set<String> keys = properties.keySet();
				Iterator<String> keyItor = keys.iterator();
				while(keyItor.hasNext())
				{
					String key = keyItor.next();
					buf.append(key + ":" + properties.get(key) + " ");
				}
				buf.append("\n");
			}
			buf.append("Transacted:" + this.transactedSession +  " Number:" + this.commitNum + "\n");
			buf.append("Retained:" + this.retained + "\n");
			this.clientDetails = buf.toString();
			
			// Set up the producer
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Set up complete for " + clientDetails);	
			}
		}
		catch(Exception e)
		{
			if(currentTry < attemptSetup)
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("We encountered an exception whilst setting up the producers. We will sleep for 20 seconds and then attempt connection again");
				}
				try {
					Thread.sleep(20000);
				} catch (InterruptedException e1) {
					
				}
				currentTry++;
				this.setup(username, password, destinationName, destinationType);
				
			}
			else
			{
				throw new RuntimeException(e);
			}
		}
	}
	
	
	/**
	 * This code is used to recover if the server if down for any reason.
	 */
	public synchronized void resetConnections()
	{
	
		if(isReconnecting())
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
						trace.trace("We found a closed connected");
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
		reconnecting = true; // stop message production
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
		
		do
		{
			// lets free everything up
			for(int i=0; i<connections.length; i++)
			{
				connections[i] = null;
			}
			
			for(int i=0; i<sessions.length; i++)
			{
				sessions[i] = null;
			}
			
			for(int i=0; i<producers.length; i++)
			{
				producers[i] = null;
			}
			try
			{
				
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
					connections[i].setClientID(clientIDs[i]);
				}	
				
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Recreated connections for clients ");
				}
				
				if(this.useSameConnection)
				{
					for(int i=0; i<sessions.length; i++)
					{
						sessions[i] = connections[0].createSession(transactedSession, Session.AUTO_ACKNOWLEDGE);
					}
				}
				else
				{
					for(int i=0; i<sessions.length; i++)
					{
						sessions[i] = connections[i].createSession(transactedSession, Session.AUTO_ACKNOWLEDGE);
					}
				}
				
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Recreated Sessions");
				}
				if(destinationType.equals(JmsMessage.DESTINATION_TYPE.QUEUE))
				{
					for(int i=0; i<producers.length; i++)
					{
						producers[i] = sessions[i].createProducer(sessions[i].createQueue(destinationName));
						producers[i].setDeliveryMode(persistence);
					}
					
				}
				else if(destinationType.equals(JmsMessage.DESTINATION_TYPE.TOPIC))
				{
					for(int i=0; i<producers.length; i++)
					{
						producers[i] = sessions[i].createProducer(sessions[i].createTopic(destinationName));
						producers[i].setDeliveryMode(persistence);
					}
				}
				
				connections[0].setExceptionListener(listener);
				
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Recreated producers");
				}
				sleepTime = 0;
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
			trace.trace("Producers starting execution");
		}
		for(int i=0; i<numberToSend; i++)
		{			
			if(!keepRunning)
			{
				break;
			}
			if(i != 0)
			{
				try
				{
					if(trace!= null && trace.traceOn())
					{
						trace.trace("Pausing sending messages");
					}
					Thread.sleep(new Random().nextInt(pauseInterval)); // now go to sleep so we don't send all at once
				}
				catch(InterruptedException e)
				{
				 //	do nothing
				}
			}

			
			try
			{
				
				if(!isReconnecting())
				{
					String messageBody = createMessageKB(this.messageSize);
					Message[] messages = new Message[this.numberClients];
					for(int j=0; j<sessions.length; j++)
					{
						Message msg = null;
						if(this.useSameConnection)
						{
							msg = sessions[j].createTextMessage(messageBody + clientIDs[0] +"_j_" + i);
						}
						else
						{
							msg = sessions[j].createTextMessage(messageBody + clientIDs[j] + i);
						}
						if(properties != null)
						{
							Set<String> keys = properties.keySet();
							Iterator<String> keyItor = keys.iterator();
							while(keyItor.hasNext())
							{
								String key = keyItor.next();
								msg.setStringProperty(key, properties.get(key));
								if(trace!= null && trace.traceOn())
								{
									trace.trace("Adding property " + key + " with value " + properties.get(key) + " to message");
								}
							}
							
						}
						if(this.retained)
						{
							{
								trace.trace("Setting the message to retained");
							}
							msg.setIntProperty("JMS_IBM_Retain", 1);
						}
						messages[j] = msg;
					}
					
					
					if(transactedSession)
					{
						commitNum++;
					}
			
					if(! reconnecting)
					{	
						for(int j=0; j<producers.length; j++)
						{
							producers[j].send(messages[j]);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("producer number " + j + " sent message");
							}
							if(transactedSession)
							{
								
								if(commitNum >= transactedNum)
								{
									sessions[j].commit();
									if(trace!= null && trace.traceOn())
									{
										trace.trace("We were at the correct level for our transacted session so committing the messages");
									}
								}
								else
								{
									trace.trace("Although the session is transacted we have not reached our number to commit the session");
								}
							}
							
						}
							
						if(transactedSession)
						{	
							if(commitNum == transactedNum)
							{
								commitNum=0; //reset it but yes we may have an exception prior to this which would mean some committed
							}
						}
						
					
					}
				}
				else
				{
					// do nothing for a bit
					Thread.sleep(20000);
				}
			}
			catch(Exception e)
			{
				
				if(! reconnecting)
				{
					this.resetConnections();
				}
				else
				{
					if(trace!= null && trace.traceOn())
					{
						trace.trace("We are reconnecting so going to sleep and try again");
					}
					try
					{
						Thread.sleep(20000);
					}
					catch(Exception ef)
					{
						// 	do nothing
					}
				}
			}

		}
		if(trace!= null && trace.traceOn())
		{
			trace.trace("The thread was stopped or we finished sending all our messages so closing connections");
			
		}
	
		this.closeConnections();
	}
	
	/**
	 * close all the producers
	 */
	public void closeConnections()
	{
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Closing connections");
		}
		
		for(int i=0; i<producers.length; i++)
		{
			try
			{
				producers[i].close();
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Producer closed");
				}
			}
			catch(Exception e)
			{
			// 	do nothing
			}
		}
		for(int i=0; i<connections.length; i++)
		{
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
	 * Create a message of the specified size
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
