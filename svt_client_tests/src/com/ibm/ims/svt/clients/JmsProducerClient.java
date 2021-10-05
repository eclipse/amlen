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
import java.util.HashMap;
import java.util.Iterator;
import java.util.Random;
import java.util.Set;
import java.util.UUID;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.naming.NamingException;

import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.mq.jms.MQConnection;

/**
 * Use this class to create a single JmsProducerClient thread.
 *
 */
public class JmsProducerClient implements Runnable {
	
	protected JmsMessage.MESSAGE_TYPE type = JmsMessage.MESSAGE_TYPE.SERIAL;
	protected boolean transactedSession = false;
	protected int transactedNum = 0;
	protected boolean recoverConnection = true;
	protected boolean retained = false;
	protected int retainedInterval = 0;
	protected int persistence = DeliveryMode.NON_PERSISTENT;
	protected Audit trace = null;
	protected HashMap<String, String> properties = null;
	protected boolean retainedMqConnectivity = false;	
	
	protected int pauseInterval = 5000;
	protected int numberToSend = 100000;
	protected int retainedCount = 0;
	protected String username = null;
	protected String password = null;
	
	protected ConnectionFactory cf = null;	
	private Connection connection = null;
	private Session session = null;	
	private MessageProducer producer = null;
	private JmsProducerExceptionListener listener = null;
	private String clientID = UUID.randomUUID().toString().substring(0, 15);	
	
	public boolean keepRunning = true;		
	
	protected JmsMessage.DESTINATION_TYPE destinationType = null;
	protected String destinationName = null;

	protected boolean incomplete = false;
	protected String body = null;			
	protected String[] messageNotCommitted = null;
	protected boolean reconnecting = false;
	protected Message msg = null;
	protected int commitNum = 0;
	protected boolean stopOnPublishFailure = false;
	protected String failMessage = "";
	protected ArrayList<String> sentMsgs = new ArrayList<String>();
	protected boolean storeSentMsgs = false;
	protected boolean inReconnectingLoop = false;
	
	protected String clientDetails = null;

	/**
	 * This method is only used for a file JNDI
	 * @param jndiLocation
	 * @param connectionFactoryName
	 * @throws NamingException
	 */
	public JmsProducerClient(String jndiLocation, String connectionFactoryName) throws NamingException
	{
		cf = new JmsConnection(jndiLocation, connectionFactoryName).getConnectionFactory();
	}
	
	/**
	 * This method is used to create an MQ client
	 * @param ipaddress
	 * @param port
	 * @param queueManager
	 * @param channel
	 * @throws NamingException
	 * @throws JMSException
	 */
	public JmsProducerClient(String ipaddress, int port, String queueManager, String channel) throws  NamingException, JMSException
	{
		cf = new JmsConnection(ipaddress, port, queueManager, channel).getConnectionFactory();
	}
	
	/**
	 * This method is used to create a IMA client
	 * @param ipaddress
	 * @param port
	 * @throws NamingException
	 * @throws JMSException
	 */
	public JmsProducerClient(String ipaddress, int port) throws NamingException, JMSException
	{
		cf = new JmsConnection(ipaddress, port).getConnectionFactory();
	}
	
	/**
	 * This method should only be called once all the client configuration methods have been specified. 
	 * @param username
	 * @param password
	 * @param destinationName
	 * @param destinationType
	 */
	public void setup(String username, String password, String destinationName, JmsMessage.DESTINATION_TYPE destinationType)
	{
		int attemptSetup=5;
		int currentTry=0;
		
		try
		{
			// Set up the connections and subscriber etc
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Setting up a producer for destination " + destinationName +  " with clientID=" + clientID);
			}
								
			if(username != null && password != null)
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Setting up the connection with user name " + username);
				}	
				try
				{
					connection = cf.createConnection(username, password);	
				}
				catch(Exception e)
				{
					// try again just for the HA pair
					connection = cf.createConnection(username, password);	
				}
			}
			else
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Setting up the connection");
				}
				try
				{
					connection = cf.createConnection();
				}
				catch(Exception e)
				{
					// try again just for the HA pair
					connection = cf.createConnection();
				}
			}
			connection.setClientID(clientID);
			
			session = connection.createSession(transactedSession, Session.AUTO_ACKNOWLEDGE);
			this.destinationType = destinationType;
			this.destinationName = destinationName;
			
			if(transactedSession)
			{
				messageNotCommitted = new String[transactedNum];
			}
			
			if(destinationType.equals(JmsMessage.DESTINATION_TYPE.QUEUE))
			{
				producer = session.createProducer(session.createQueue(destinationName));
			}
			else if(destinationType.equals(JmsMessage.DESTINATION_TYPE.TOPIC))
			{
				producer = session.createProducer(session.createTopic(destinationName));
			}
			
			producer.setDeliveryMode(persistence);
			listener = new JmsProducerExceptionListener(this);
			connection.setExceptionListener(listener);
			
			StringBuffer buf = new StringBuffer();
			buf.append("Single JMS Producer: \n");
			buf.append("Clientid:"+this.clientID + "\n");
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
				trace.trace("Set up complete for  " + this.clientDetails);
			}
		}
		catch(Exception e)
		{
			if(currentTry < attemptSetup)
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("We encountered an exception whilst setting up the producer. We will sleep for 20 seconds and then attempt connection again");
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
	 * This method specifies that the producer should send retained messages.
	 * This method must be called before the setup() method.
	 * @param intervalNumber
	 */
	public void sendRetainedMessages(int intervalNumber)
	{
		this.retained = true;
		this.retainedInterval = intervalNumber;
	}
	
	/**
	 * This method specifies that the producer will use a transacted
	 * session and commit after the specified number of messages have
	 * been sent to the server.
	 * 
	 * This method must be called before the setup() method.
	 * @param intervalNumber
	 */
	public void setTransactedSession(int intervalNumber)
	{
		this.transactedSession = true;
		this.transactedNum = intervalNumber;
	}
	
	/**
	 * This method specifies that the producer should fail if the connection
	 * dies. This should only be used if we have no failover or recovery testing.
	 * This method must be called before the setup() method.
	 */
	public void setNoConnectionRecovery()
	{
		this.recoverConnection = false;
	}
	
	/**
	 * This method specifies a filename to create which will be used to 
	 * trace this client
	 * This method must be called before the setup() method.
	 * @param fileName
	 * @throws IOException
	 */
	public void setTraceFile(String fileName) throws IOException
	{
		File audit = new File(fileName);
		if(!audit.exists())
		{
			audit.createNewFile();
		}
		
		this.trace = new Audit(fileName, true);
		
	}
	
	/**
	 * This method specifies that the producer will send in persistent messages
	 * This method must be called before the setup() method.
	 */
	public void sendPersistentMessages()
	{
		this.persistence = DeliveryMode.PERSISTENT;
	}
	
	/**
	 * This method specifies which type of JMS message to send
	 * This method must be called before the setup() method.
	 * @param messageType
	 */
	public void setMessageType(JmsMessage.MESSAGE_TYPE messageType)
	{
		this.type = messageType;
	}
	
	/**
	 * This method specifies that the producer will generate JMS User properties
	 * for each message.
	 * This method must be called before the setup() method.
	 * @param headerProperties
	 */
	public void setHeaderProperties(HashMap<String, String> headerProperties)
	{
		this.properties = headerProperties;
	}
	
	/**
	 * This method specifies the number of messages to send to the server.
	 * This method must be called before the setup() method.
	 * @param number
	 */
	public void setNumberOfMessages(int number)
	{
		this.numberToSend = number;
	}
	
	/**
	 * This method specifies the minimum pause interval between each message that is sent.
	 * This method must be called before the setup() method.
	 * @param number
	 */
	public void setMessageInterval(int number)
	{
		this.pauseInterval = number;
	}
	/**
	 * This method is used to indicate whether we are attempting a reconnection. We may get a notification that something 
	 * happened if we get a problem from the exception listener or the listener. We only want to start the process of attempting
	 * reconnection if this is the first time we encountered a problem.
	 * @return
	 */
	public boolean isReconnecting()
	{
		return reconnecting;
	}
	
	/**
	 * This method overrides the default client ID
	 * This method must be called before the setup() method.
	 * @param overrideClientID
	 */
	public void setClientID(String overrideClientID)
	{
		this.clientID = overrideClientID;
	}
	
	/**
	 * This method is used to indicate that we should fail if we encounter
	 * a problem publishing a message if the error is the destination is full (used for discard message testing)
	 * This method must be called before the setup() method.
	 */
	public void stopOnPublishFailure()
	{
		stopOnPublishFailure = true;
	}
	
	/**
	 * This code is used to recover if the server if down for any reason.
	 */
	public synchronized void resetConnections()
	{
		if(!recoverConnection)
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Client " + clientID + " has not to recover the connection so exiting the thread");
			}
			setFailMessage("An exception occurred when it was not expected as client is configured to be non-recoverable");
			keepRunning=false;
			return;
			
		}
		
		if(connection instanceof ImaConnection)
		{
			if((isReconnecting() && ! inReconnectingLoop) || (!(Boolean)((ImaConnection)connection).get("isClosed")))
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("We are already reconnecting or are connected so return");
				}
				return;
			}
		}
		else if(connection instanceof MQConnection)
		{
			if(isReconnecting())
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("We are already reconnecting so return");
				}
				return;
			}
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
		inReconnectingLoop = true;
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Resetting the connections after a failed attempt");
		}
		
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Closing all connections");
		}
		try
		{
			connection.close();
		}
		catch(Exception e)
		{
			// do nothing
		}
	
		if(transactedSession)
		{
			// 	remove all keys
			for(int j=0; j<messageNotCommitted.length; j++)
			{
				String removeBody = messageNotCommitted[j];
				if(trace!= null && trace.traceOn())
				{
					trace.trace(removeBody + " was not sent successfully and is being removed from awaiting transacted list");
				}
			}
			commitNum = 0;	
		}
		
		do
		{
			try
			{
				
				connection = cf.createConnection();	
				connection.setClientID(clientID);	
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Recreated connection for client " + clientID);
				}
				session = connection.createSession(transactedSession, Session.AUTO_ACKNOWLEDGE);
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Recreated Session");
				}
				if(destinationType.equals(JmsMessage.DESTINATION_TYPE.TOPIC))
				{
					producer = session.createProducer(session.createTopic(destinationName));	
				}
				else if(destinationType.equals(JmsMessage.DESTINATION_TYPE.QUEUE))
				{
					producer = session.createProducer(session.createQueue(destinationName));	
				}
				producer.setDeliveryMode(persistence);
				connection.setExceptionListener(listener);
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Recreated producer");
				}
				
				reconnecting = false;
				inReconnectingLoop = false;
				
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
					Thread.sleep(5000);
				}
				catch(Exception ex)
				{
					// 	do nothing
				}
				
				
			}
		}while(reconnecting);
		
	}

	/**
	 * The thread run method which is used to send messages to the server
	 */
	public void run()
	{

		if(trace!= null && trace.traceOn())
		{
			trace.trace("Producer starting execution");
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
					
					if((retained && (i!= 0 && i%retainedInterval == 0)))
					{
						producer.setDeliveryMode(DeliveryMode.PERSISTENT); // we will only send in persistent retained messages
						if(trace!= null && trace.traceOn())
						{
							trace.trace("The retained count is at the required level so sending a retained message");
						}
						body = retainedCount + ".RETAINED." + UUID.randomUUID().toString().substring(0, 15) + "." + clientID;
						retainedCount++;
						msg = session.createTextMessage(body);
						if(trace!= null && trace.traceOn())
						{
							trace.trace("We are sending sending the following retained message " + body);
						} 
						msg.setIntProperty("JMS_IBM_Retain", 1);
	
						if(properties != null)
						{
							Set<String> keys = properties.keySet();
							Iterator<String> keyItor = keys.iterator();
							while(keyItor.hasNext())
							{
								String key = keyItor.next();
								msg.setStringProperty(key, properties.get(key));
								if(trace.traceOn())
								{
									trace.trace("Adding property " + key + " with value " + properties.get(key) + " to message");
								}
							}
						}
					}
					else
					{
						if(producer.getDeliveryMode() != this.persistence) // go back to default persistence
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Changing producer delivery mode back " + persistence);
							}
							producer.setDeliveryMode(persistence);
						}
						if(type.equals(JmsMessage.MESSAGE_TYPE.SERIAL))
						{
							msg = session.createTextMessage(clientID + ":" + i);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Created a serial message");
							}
						}
						else if(type.equals(JmsMessage.MESSAGE_TYPE.LARGE) && i==0)
						{
							msg = JmsMessage.createMessage(session, type);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Created a large message");
							}
						}
						else if (type.equals(JmsMessage.MESSAGE_TYPE.LARGE) )
						{
							// 	do nothing and use the previously created one as it large and takes up lots of memory
							
							if(trace!= null &&trace.traceOn())
							{
								trace.trace("Using previously created large message");
							}
						}
						else
						{
							msg = JmsMessage.createMessage(session, type);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Created a message of type " + type);
							}
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
					
						body = ((TextMessage)msg).getText();
						if(trace!= null && trace.traceOn())
						{
							trace.trace("Generating a new message with id=" + body + " and persistence " + persistence);
						}
						incomplete = true;	
						
					}
					
					
					if(transactedSession)
					{
						messageNotCommitted[(commitNum)]=body;
						commitNum++;
					}		
					producer.send(msg);
								
					if(transactedSession)
					{
						
						if(commitNum == transactedNum)
						{
							session.commit();
							incomplete = false;
							if(trace!= null && trace.traceOn())
							{
								trace.trace("We were at the correct level for our transacted session so committing all messages");
							}
							if(storeSentMsgs)
							{
								sentMsgs.add(body);
								for(int j=0; j<messageNotCommitted.length; j++)
								{
									String oldMsg = messageNotCommitted[j];
									sentMsgs.add(oldMsg);							
								}
							}
							commitNum=0; //reset it
							messageNotCommitted = new String[transactedNum];
						}		
						else if(commitNum < transactedNum)
						{
							if(trace!= null && trace.traceOn())
							{
								
								trace.trace("Although the session is transacted we have not reached our number to commit the session");
								trace.trace("commitNum " + commitNum);
						
							}
							
						}

						commitNum=0; //reset it
						messageNotCommitted = new String[transactedNum];
					}
					else
					{
						incomplete = false;
						if(storeSentMsgs)
						{
							sentMsgs.add(body);
						}
					}
			
					
					
					if(trace!= null && trace.traceOn())
					{
						trace.trace(body + " message sent successfully");
					}
				}
				else
				{
					if(trace!= null && trace.traceOn())
					{
						trace.trace(body + " was not sent successfully and is being removed from the count");
					}
					
					if(msg instanceof TextMessage)
					{
						if(((TextMessage) msg).getText().contains("RETAINED"))
						{
							retainedCount--;
						}
					}
					
					i--;
					if(! reconnecting)
					{
						if(trace!= null && trace.traceOn())
						{
							trace.trace("A failure occurred so attempting reconnecting");
						}
						this.resetConnections();
					}
				}
			}
			catch(Exception e)
			{
				i--;
				if(stopOnPublishFailure)
				{
					try {
						Thread.sleep(5000);
					} catch (InterruptedException e1) {
						// do nothing
					}
					if(trace!= null && trace.traceOn())
					{
						trace.trace("Check to see if the producer is still connected and if so throw fatal exception");
					}
					if(connection instanceof ImaConnection)
					{
						
						if(e.toString().contains("CWLNC0218"))
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("The producer was rejected during the publish and we are configured to stop on publish failure");
							}
							setFailMessage("The producer " + clientID + " for topic " + destinationName + " was rejected during the publish and we are configured to stop on publish failure");
							keepRunning = false;
							
						}
						else
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("The producer was rejected for a reason other than the destination was full so we will continue");
							}
						}
					
					}
				}
				if(!recoverConnection)
				{
					if(trace!= null && trace.traceOn())
					{
						trace.trace("Client " + clientID + " has been configured not to recover the connection so exiting the thread");
					}
					keepRunning=false;
					setFailMessage("An exception occurred when it was not expected as client is configured to be non-recoverable");
					return;
				}
				if(incomplete)
				{

					if(body.contains("RETAINED"))
					{
						retainedCount--;
					}
					
					if(transactedSession)
					{
						
						if(trace!= null && trace.traceOn())
						{
							trace.trace("An exception occurred whilst trying to send a message. Exception="+ e);
							trace.trace(body + " was not sent successfully and is being removed from any list");
						}
								
						//	remove all keys
						for(int j=0; j<messageNotCommitted.length; j++)
						{
							String removeBody = messageNotCommitted[j];
							if(trace!= null && trace.traceOn())
							{
								trace.trace(removeBody + " was not sent successfully and is being removed from any lists");
							}
							if(removeBody != null)
							{
								if(removeBody.contains("RETAINED"))
								{
									retainedCount--;
								}
									
							}
							
						}
					}
				}
				if(! reconnecting)
				{
					this.resetConnections();
				}
			}

		}
		if(trace!= null && trace.traceOn())
		{
			trace.trace("The thread was stopped or we finished sending all our messages so closing connections");
			
		}
		if(incomplete)
		{
			if(body.contains("RETAINED"))
			{
				retainedCount--;
			}
			
			if(trace!= null && trace.traceOn())
			{
				trace.trace(body + " was not sent successfully and is being removed from any lists");
			}
		}
		if(transactedSession)
		{
			//	remove all keys
			for(int j=0; j<messageNotCommitted.length; j++)
			{
				String removeBody = messageNotCommitted[j];
				if(trace!= null && trace.traceOn())
				{
					trace.trace(removeBody + " was not sent successfully and is being removed from any lists");
				}
				if(removeBody != null)
				{
					if(removeBody.contains("RETAINED"))
					{
						retainedCount--;
					}
					
				}
					
			}
		}
		this.closeConnections();
	
	}
	
	/**
	 * Returns the client ID for this producer,
	 * @return
	 */
	public String getClientID()
	{
		return this.clientID;
	}
	
	private void setFailMessage(String message)
	{
		if(failMessage.equals(""))
		{
			failMessage = this.clientDetails + "FailMessage:";
		}
		this.failMessage =  failMessage + message + " \n ";
	}
	
	/**
	 * This method is used to notify the test code that we encountered a test failure in this client. The test code 
	 * 'polls' certain clients to see if a problem occurred and then it can take the appropriate action for that test.
	 * @return
	 */
	public String getFailMessage()
	{
		return failMessage;
	}
		
	/**
	 * Used to close all the connections and client
	 */
	public void closeConnections()
	{
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Closing connections");
		}
		try
		{
			producer.close();
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Producer closed");
			}
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			connection.close();
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Connection closed");
			}
		}
		catch(Exception e)
		{
			// do nothing
		}
	
	}
	
	/**
	 * Specifies that the producer should store the messages it sends.
	 * Method must be called before setup(...)
	 */
	public void storeSentMsgs()
	{
		this.storeSentMsgs = true;
	}
	
	
	/**
	 * Returns the messages stored by the producer
	 * @return
	 */
	public ArrayList<String> getSentMsgs()
	{
		return sentMsgs;
	}

}
