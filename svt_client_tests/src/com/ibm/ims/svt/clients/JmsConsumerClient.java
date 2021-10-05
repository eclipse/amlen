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

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.Topic;
import javax.naming.NamingException;

import com.ibm.ima.jms.ImaSubscription;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.test.cli.imaserver.ImaHAServerStatus;
import com.ibm.ima.test.cli.imaserver.ImaServer;
import com.ibm.ima.test.cli.imaserver.ImaServerStatus;
import com.ibm.mq.jms.MQConnection;

/**
 * Use this class to create a single JmsConsumerClient thread.
 *
 */
public class JmsConsumerClient implements Runnable {
	

	protected boolean durable = false;
	protected String selector = null;
	protected boolean transactedSession = false;	
	protected int ackMode = Session.AUTO_ACKNOWLEDGE;
	protected boolean recoverConnection = true;
	protected Audit trace = null;
	protected boolean retainedAware = false;
	protected int numMessageWaitToAckCommit = -1;
	
	public boolean keepRunning = true;
	protected boolean reconnecting = false;
	
	protected ConnectionFactory cf = null;
	private Connection connection = null;
	private Session clientSession = null;
	private MessageConsumer consumer = null;
	private JmsListener listener = null;
	private JmsConsumerExceptionListener exceptionListener = null; 
	
	protected String destinationName = null;
	protected JmsMessage.DESTINATION_TYPE destinationType = null;
	
	protected String subID = null;
	private String clientID=null;
	protected ArrayList<RetainedMap> retainedKey = new ArrayList<RetainedMap>();
	protected boolean afterProducer = false;
	protected boolean checkRetainedDuplicates = false;
	protected boolean sharedSubscription = false;
	protected boolean checkSharedSubDuplicates = false;
	protected boolean regressionTest = false;
	protected ArrayList<String> regressionClientIDs = null;
	protected String regressionFailMessage = "";
	protected boolean storeReceivedMsgs = false;
	
	protected boolean checkMsgInterval = false;
	protected long checkMsgIntervalVal = 30000;
	protected long lastTime = System.currentTimeMillis();
	
	protected String username = null;
	protected String password = null;
	
	protected int maxMessagesToStore = -1;
	protected int minMessagesToStore = -1;
	protected ArrayList<String> sharedSubMsgs = new ArrayList<String>();
	
	protected String clientDetails = null;
	
	protected ImaServer server = null;
	/**
	 * This method is only used for a file JNDI
	 * @param jndiLocation
	 * @param connectionFactoryName
	 * @throws NamingException
	 */
	public JmsConsumerClient(String jndiLocation, String connectionFactoryName) throws NamingException
	{
		cf = new JmsConnection(jndiLocation, connectionFactoryName).getConnectionFactory();
	}
	
	/**
	 * This method is used to create an MQ JMS client
	 * @param ipaddress
	 * @param port
	 * @param queueManager
	 * @param channel
	 * @throws NamingException
	 * @throws JMSException
	 */
	public JmsConsumerClient(String ipaddress, int port, String queueManager, String channel) throws NamingException, JMSException
	{
		cf = new JmsConnection(ipaddress, port, queueManager, channel).getConnectionFactory();
	}
	
	/**
	 * This meth0d is used to create an ISM JMS client
	 * @param ipaddress
	 * @param port
	 * @throws NamingException
	 * @throws JMSException
	 */
	public JmsConsumerClient(String ipaddress, int port) throws NamingException, JMSException
	{
		cf = new JmsConnection(ipaddress, port).getConnectionFactory();
	}
	
	
	/**
	 * This method should only be called once all the client configuration methods have been specified. 
	 * @param username
	 * @param password
	 * @param destinationType
	 * @param destinationName
	 */
	public void setup(String username, String password, JmsMessage.DESTINATION_TYPE destinationType, String destinationName)
	{
		int attemptSetup=5;
		int currentTry=0;
		if(clientID == null)
		{
			// Create a random client id if one has not been specified
			// yes there is a slight risk of a clash with another but generally this has proved ok
			clientID = UUID.randomUUID().toString().substring(0, 15);
		}
		try
		{
			// Set up the connections and subscriber etc
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Setting up a consumer for destination " + destinationName);
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
						
			this.destinationName = destinationName;
			this.destinationType = destinationType;
			this.username = username;
			this.password = password;
			
			if(!sharedSubscription)
			{
				connection.setClientID(clientID);
			}
			else
			{
				clientID="NOTINUSE";
			}
			
			if(trace!= null && trace.traceOn())
			{
				trace.trace("client ID=" + clientID);
			}
						
			clientSession = connection.createSession(transactedSession, ackMode);
			
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Session is transacted=" + transactedSession + " with ackMode=" + ackMode);
			}
			//TODO refactor this into another method to reduce duplicate code in resetConnections
			// create a durable client
			if(durable)
			{
				if(!sharedSubscription)
				{
					subID = UUID.randomUUID().toString().substring(0, 10);
				}
				if(destinationType.equals(JmsMessage.DESTINATION_TYPE.TOPIC))
				{
					Topic topic = clientSession.createTopic(destinationName);
					
					if(selector != null)
					{
						if(sharedSubscription)
						{
							consumer = ((ImaSubscription)(clientSession)).createSharedDurableConsumer(topic, subID, selector);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Created shared durable consumer with selector " + selector +  " and subID " + subID);
							}
						}
						else
						{
							consumer = clientSession.createDurableSubscriber(topic, subID, selector, false);
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
							consumer = ((ImaSubscription)(clientSession)).createSharedDurableConsumer(topic, subID);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Created shared durable consumer with subID " + subID);
							}
						}
						else
						{
							consumer = clientSession.createDurableSubscriber(topic, subID);
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
			else // non durable client
			{
				if(destinationType.equals(JmsMessage.DESTINATION_TYPE.TOPIC))
				{
					if(selector != null)
					{
						if(sharedSubscription)
						{
							consumer = ((ImaSubscription)(clientSession)).createSharedConsumer(clientSession.createTopic(destinationName), subID, selector);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Created shared consumer with selector " + selector);
							}
						}
						else
						{
							consumer = clientSession.createConsumer(clientSession.createTopic(destinationName), selector);
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
							consumer = ((ImaSubscription)(clientSession)).createSharedConsumer(clientSession.createTopic(destinationName), subID);
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Created shared consumer");
							}
						}
						else
						{
							consumer = clientSession.createConsumer(clientSession.createTopic(destinationName));
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
						consumer = clientSession.createConsumer(clientSession.createQueue(destinationName), selector);
						if(trace!= null && trace.traceOn())
						{
							trace.trace("Created consumer with selector " + selector);
						}
					}
					else
					{
						consumer = clientSession.createConsumer(clientSession.createQueue(destinationName));
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
			
			if(!this.regressionTest)
			{
				exceptionListener = new JmsConsumerExceptionListener(this); 
				if(transactedSession)
				{
					listener = new JmsListener(this, transactedSession, numMessageWaitToAckCommit, storeReceivedMsgs);
				}
				else if (ackMode == Session.CLIENT_ACKNOWLEDGE)
				{
					listener = new JmsListener(this, numMessageWaitToAckCommit, storeReceivedMsgs);
				}
				else
				{
					listener = new JmsListener(this, storeReceivedMsgs);
				}
				
			
				if(retainedAware)
				{
					if(trace!= null && trace.traceOn())
					{
						trace.trace("Setting the listener to be retained aware");
					}
					if(durable)
					{
						listener.checkRetainedMessage(true, retainedKey, afterProducer, checkRetainedDuplicates);
					}
					else
					{
						listener.checkRetainedMessage(false, retainedKey,  afterProducer, checkRetainedDuplicates);
					}	
				}
				connection.setExceptionListener(exceptionListener);		
			}
			else
			{
				// This will create a listener for specific producer client IDs - the producers must
				// send in messages containing a text string of client id + incremented number for this to work
				// can be used to determine that we get messages sent in sequence from the individual clients whilst
				// still have them decoupled code wise - must be used with the correct transactional/recovery behaviour in
				// the producers.
				if(destinationType.equals(JmsMessage.DESTINATION_TYPE.QUEUE))
				{
					listener = new JmsMessageListener(this, regressionClientIDs, true);
				}
				else
				{
					listener = new JmsMessageListener(this, regressionClientIDs, durable);
				}
			}
					
			if(sharedSubscription)
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Setting the listener to be check for shared subscription duplicates");
				}
				listener.checkSharedSubMsgs();
			}
			
			// Often tests can run for many days and to keep a record of all the messages we receive will just
			// blow up the memory - this can be used to ensure we only keep so many messages by throwing
			// some away when we hit the maximum
			if(this.maxMessagesToStore != -1)
			{
				listener.setMaxMessagesToKeep(this.maxMessagesToStore, this.minMessagesToStore);
			}
					
			consumer.setMessageListener(listener);
			
			StringBuffer buf = new StringBuffer();
			buf.append("Single JMS Consumer: \n");
			buf.append("Clientid:"+this.clientID + "\n");
			buf.append("Destination:"+this.destinationName + "\n");
			buf.append("Durable:"+this.durable + "\n");
			buf.append("SharedSub:"+this.sharedSubscription + " SubID:" + this.subID + "\n");
			buf.append("Selector:"+this.selector + "\n");
			buf.append("Transacted:" + this.transactedSession + " AckMod: " + this.ackMode + " Number:" + this.numMessageWaitToAckCommit + "\n");
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
				trace.trace("Finished setup for client " + clientDetails);
			}
			
		}
		catch(Exception e)
		{
			// If we didn't set up the client successfully the keep trying for up to 5 times.
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
	 * This can be used to specify a name of a file used to log all the trace messages for this particular client.
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
	 * Specify this when we expect no failover or server recovery testing to occur. This can be used when you want to fail a test
	 * if the client is disconnected for any reason
	 * 
	 * This method must be called before the setup() method.
	 */
	public void setNoConnectionRecovery()
	{
		this.recoverConnection = false;
	}
	
	/**
	 * This specifies that this client is part of a shared subscription and whether we want to
	 * check for duplicates across a shared subscription (duplicate checking can only be done in
	 * the extended MultiJmsConsumerClient class so don't use it for single JMS clients
	 * 
	 * This method must be called before the setup() method.
	 * @param subName
	 */
	public void setSharedSubscription(String subName, boolean checkDuplicates)
	{
		this.subID = subName;
		this.sharedSubscription = true;
		this.checkSharedSubDuplicates = checkDuplicates;
		
	}
	
	/**
	 * Specifies that this client is durable
	 * 
	 * This method must be called before the setup() method.
	 */
	public void setDurable()
	{
		this.durable = true;
	}
	
	/**
	 * Specifies a selector to use for message selection
	 * 
	 * This method must be called before the setup() method.
	 * @param selector
	 */
	public void setSelector(String selector)
	{
		this.selector = selector;
	}
	
	/**
	 * This method is used to differentiate messages arriving from known producers. The intention is that
	 * we can identify which message is sent from which producer from the first part of the message
	 * body containing the client ID and then check whether we get the correct stream of messages
	 * from that particular client. Only use this if you have configured the producers to send in
	 * the exact message sequence.
	 * 
	 * This method must be called before the setup() method.
	 * @param regressionClientIDs
	 */
	public void setRegressionListener(ArrayList<String> regressionClientIDs)
	{
		this.regressionTest = true;
		this.regressionClientIDs = regressionClientIDs;
	}
	
	/**
	 * This specifies whether to use a transacted session in the client.
	 * 
	 * This method must be called before the setup() method.
	 */
	public void setTransactedSession()
	{
		this.transactedSession = true;
	}
	
	/**
	 * This method specifies the acknowledgement mode and if clientAck etc then the
	 * number of messages to consume before we acknowledge a message.
	 * 
	 * This method must be called before the setup() method.
	 * @param ackMode
	 * @param ackInterval
	 */
	public void setAckMode(int ackMode, int ackInterval)
	{
		this.ackMode = ackMode;
		this.numMessageWaitToAckCommit = ackInterval;
	}
	
	/**
	 * This method specifies whether the client is retained message aware, i.e. whether
	 * it will check to see if we get a retained message when we connect depending on the 
	 * durable state. These values will be passed into the listener to use accordingly.
	 * 
	 * This method must be called before the setup() method.
	 * 
	 * @param afterProducer - this identifies whether the consumer was set up before the producer
	 * has sent a retained message so we can check we get it correctly. 
	 * @param checkRetainedDuplicates - this identifies whether we will also check for any duplicate
	 * retained messages at the same time
	 */
	public void checkRetainedMessages(boolean afterProducer, boolean checkRetainedDuplicates)
	{
		this.retainedAware = true;
		this.afterProducer = afterProducer;
		this.checkRetainedDuplicates = checkRetainedDuplicates;
	}
	
	
	/**
	 * This can be used to stop and start the consumer during a test.
	 */
	public void stopListening()
	{
		try
		{
			consumer.close();
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Consumer has stopped listening");
			}
		}
		catch(Exception e)
		{
			//do nothing
		}
	}
	
	/**
	 * This can be called to start listening again is the consumer has deliberately stopped listening.
	 */
	public void startListening()
	{
		try
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("We want consumer to start listening so going to resetConnections");
			}
			resetConnections();
		}
		catch(Exception e)
		{
			//do nothing
		}
	}
	
	/**
	 * Used by the listener class to commit the session when transacted. We have this method here
	 * so we can avoid passing the session to the listener.
	 */
	public void commitSession(int id)
	{
		try
		{
			clientSession.commit();
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
	 * Returns the client ID for this particular client.
	 * @return
	 */
	public String getClientId()
	{
		return clientID;
	}
	
	/**
	 * Used to close the client and connections down after the test has completed.
	 */
	public void closeConnections()
	{
		try
		{
			consumer.close();
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Consumer closed");
			}
		}
		catch(Exception e)
		{
			// do nothing
		}
		if(durable)
		{
			try
			{
				Thread.sleep(5000);
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Unsubscribed from durable subscription");
				}
				clientSession.unsubscribe(subID);
			}
			catch(Exception e)
			{
				// do nothing
			}
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
	 * Get the flag which is used to indicate whether this client is attempting to reconnect.
	 * @return
	 */
	public boolean isreconnecting()
	{
		return reconnecting;
	}
	
	/**
	 * This code is used to recover the client if the server if down for any reason. Some clients don't care what happens in that as
	 * soon as a problem occurs they want to attempt a reconnection. This is used for failover and recovery testing and also
	 * when we want a number of clients that try to be up and running at all times to create a 'normal messaging' load on the server.
	 */
	public synchronized void resetConnections()
	{
		// Sometimes we want to know if a client got kicked off as we may be performing tests with NO failover
		// In this case make sure the recoverConnection has been specified as false before the client setup.
		if(!recoverConnection)
		{
			if(trace!= null && trace.traceOn())
			{
				trace.trace("Client " + clientID + " has been configured not to recover the connection so exiting the thread");
			}
			
			failTest("An exception occurred when it was not expected as client is configured to be non-recoverable");
			if(trace!= null && trace.traceOn())
			{
				trace.trace("An exception occurred when it was not expected as client is configured to be non-recoverable");
			}
			keepRunning=false; // stop the client
			return;
		}
		if(connection instanceof ImaConnection)
		{
			if(isreconnecting() || (!(Boolean)((ImaConnection)connection).get("isClosed")))
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("We are already reconnecting so return");
				}
				return;
			}
		}
		else if(connection instanceof MQConnection)
		{
			if(isreconnecting())
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
		// ensure we stop all other processing by flagging up that we are reconnecing.
		reconnecting = true;
		
		do
		{
			listener.connectionLost(); // notify the listener that the connection was lost - can be used for retained message testing
			
			try
			{
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Resetting the connections after a failed attempt");
				}
				
				// 	make sure we clear any messages in history waiting for be acked or committed as the connections have been reset
				// 	this can also be used for determining retained messages following reconnection
				listener.clearMsgs(); 
				
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Closing the connections if not already done so");
				}
				try
				{
					connection.close();
				}
				catch(Exception e)
				{
				// 	do nothing
				}
				
				
				// 	OK lets attempt to create the client with the same state as it was set up
				connection = cf.createConnection();
				if(!sharedSubscription)
				{
					connection.setClientID(clientID);
				}
						
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Recreated connection for client " + clientID);
				}
				
				clientSession = connection.createSession(transactedSession, ackMode);
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Recreated session which was transacted=" + transactedSession + " with ackMode=" + ackMode);
				}
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Recreated Session");
				}
				if(durable)
				{	
					Topic topic = clientSession.createTopic(destinationName);
					
					if(selector != null)
					{
						if(sharedSubscription)
						{
							consumer = ((ImaSubscription)(clientSession)).createSharedDurableConsumer(topic, subID, selector);
						}
						else
						{
							consumer = clientSession.createDurableSubscriber(topic, subID, selector, false);
						}
						
					}
					else
					{
						if(sharedSubscription)
						{
							consumer = ((ImaSubscription)(clientSession)).createSharedDurableConsumer(topic, subID);
						}
						else
						{
							consumer = clientSession.createDurableSubscriber(topic, subID);
						}
					}
					if(trace!= null && trace.traceOn())
					{
						trace.trace("Created durable subscriber with subID=" + subID);
					}
				}
				else
				{
					if(destinationType.equals(JmsMessage.DESTINATION_TYPE.TOPIC))
					{
						
						Topic topic = clientSession.createTopic(destinationName);
						
						if(selector != null)
						{
							if(sharedSubscription)
							{
								consumer = ((ImaSubscription)(clientSession)).createSharedConsumer(topic, subID, selector);
							}
							else
							{
								consumer = clientSession.createConsumer(topic, selector);
							}
							
						}
						else
						{
							if(sharedSubscription)
							{
								consumer = ((ImaSubscription)(clientSession)).createSharedConsumer(topic, subID);
							}
							else
							{
								consumer = clientSession.createConsumer(topic);
							}
						}
					}
					else if(destinationType.equals(JmsMessage.DESTINATION_TYPE.QUEUE))
					{
						if(selector != null)
						{
							consumer = clientSession.createConsumer(clientSession.createQueue(destinationName), selector);
							
						}
						else
						{
							consumer = clientSession.createConsumer(clientSession.createQueue(destinationName));
						}
					}
				}
				if(!this.regressionTest)
				{
					connection.setExceptionListener(exceptionListener);
				}	
				consumer.setMessageListener(listener);
			
			
			
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Recreated consumer");
				}
				
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Starting connection");
				}
				connection.start();
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Started connection");
				}
				
				// We need to make sure we leave a bit of time after reconnecting before we 
				// start to check for messages being received.
				lastTime = System.currentTimeMillis() + 240000; 				
				reconnecting = false;
				
				
			}
			catch(Exception e)
			{
				// 	If we encounter a problem the server may still be down so we want to sleep before trying again.
				if(trace!= null && trace.traceOn())
				{
					trace.trace("Exception occurred when attempting to recreate the connections. Message=" + e.getMessage());
					trace.trace("Sleeping for a short time before trying again");
				}
				try
				{
					Thread.sleep(15000);
				}
				catch(Exception ex)
				{
				// 	do nothing
				}
				
				
			}
		}while(reconnecting);
			
	}
	
	

	/**
	 * The thread run method which effectively just keeps checking to see if we are connected and if not it will
	 * attempt to reconnect. This ensures we don't get 'stuck' anywhere in the actual client test code.
	 */
	public void run() {
		
		if(trace!= null && trace.traceOn())
		{
			trace.trace("Consumer starting execution");
		}
		try
		{
			connection.start();			
		
		}catch(Exception e)
		{
			// do nothing
		}
		
		while(keepRunning)
		{
			
			try
			{
				if(!reconnecting)
				{
					Thread.sleep(10000); //wait 10 seconds before performing any repeated test
					if(connection instanceof ImaConnection)
					{
						if((Boolean)((ImaConnection)connection).get("isClosed") && !reconnecting)
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Connection was closed so lets reconnect");
							}
							this.resetConnections();
						}
						if(checkMsgInterval && (lastTime < (System.currentTimeMillis() - this.checkMsgIntervalVal )))
						{
							long receivedMessage = listener.getLastMessageTime();
							
							if(receivedMessage > lastTime)
							{
								// do nothing we got a messages
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
									trace.trace("JMS Client " + this.clientID + " listening on destination  " + this.destinationName + " has been configured to check for messages every " + checkMsgIntervalVal + "milliseconds.");
									trace.trace("The last message arrived at " + receivedMessage + " but the time is " + lastTime);
								}
								StringBuffer buffer = new StringBuffer();
								buffer.append("JMS Client " + this.clientID + " listening on destination  " + this.destinationName +  " has been configured to check for messages every " + checkMsgIntervalVal + "milliseconds.");
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
							
						}
					}
					else if(connection instanceof MQConnection && !reconnecting)
					{
						// ok no idea how to establish if there is still a connection here so going to have to do something horrible
						// TODO think about what we can do that is better than this
						try
						{
							connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
						}
						catch(Exception e)		
						{
							if(trace!= null && trace.traceOn())
							{
								trace.trace("Connection was closed so lets reconnect");
							}
							this.resetConnections();
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
				if(!recoverConnection)
				{
					if(trace!= null && trace.traceOn())
					{
						trace.trace("An exception occurred whilst listening for messages " + e.getMessage());
						trace.trace("We have been configured not to recover the connection so exiting the thread");
					}
					keepRunning=false;
					failTest("Client " + clientID + " has been configured to be non-recoverable");
					return;
				}
				else
				{
					if(!reconnecting)
					{
						this.resetConnections();
					}
				}
			}
			
		}
		
		
		if(this.getFailMessage().endsWith(""))
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
	 * Return the acknowledgment mode used by this client
	 * @return
	 * @throws Exception
	 */
	public int getAckMode() throws Exception
	{
		return clientSession.getAcknowledgeMode();
		
	}
	
	/**
	 * This returns the retained messages that were obtained by this client
	 * @return
	 */
	public ArrayList<RetainedMap> getRetainedMessages()
	{
		return retainedKey;
	}
	
	/**
	 * Specifies whether this client is part of a shared subscription
	 * @return
	 */
	public boolean isSharedSubscriber()
	{
		return sharedSubscription;
	}
	
	/**
	 * Specifies whether this client is durable or not
	 * @return
	 */
	public boolean isDurable()
	{
		if(durable)
		{
			return true;
		}
		else
		{
			return false;
		}
	
	}
	
	/**
	 * Used to specify that the client checks that we continue to receive messages.
	 * Must be called before the setup(...) method.
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
	 * This is used to create a 'test' failure message which contains more useful information about why this client
	 * had a failure in the test.
	 * @param message
	 */
	protected void failTest(String message)
	{
		if(regressionFailMessage.equals(""))
		{
			regressionFailMessage = this.clientDetails + "FailMessage:";
		}
		this.regressionFailMessage =  regressionFailMessage + message + " \n ";
	}
	
	/**
	 * This method is used to notify the test code that we encountered a test failure in this client. The test code 
	 * 'polls' certain clients to see if a problem occurred and then it can take the appropriate action for that test.
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
	 * Specifies that the client stores the messages it receives.
	 * Method must be called befre setup(...)
	 */
	public void storeReceivedMsgs()
	{
		this.storeReceivedMsgs = true;
	}
	
	/**
	 * Returns an array list of the messages this class has saved.
	 * @return
	 */
	public ArrayList<String> getReceivedMsgs()
	{
		return listener.getReceivedMsgs();
	}
	
	/**
	 * Used to override the client ID.
	 * Method must be called before setip(...)
	 * @param clientID
	 */
	public void overrideClientID(String clientID)
	{
		this.clientID = clientID;
	}
	

	/**
	 * This is not used by this class
	 * @param msg
	 */
	protected void checkSharedSubMsg(String msg)
	{
		if(trace!= null && trace.traceOn())
		{
			trace.trace("The checkSharedSubMsg is not implemented for this client");
			
		}
	}
	
	
	/**
	 * This is used to reduced the amount of messages to keep for any particular test.
	 * If the test is going to be run over several days we need to limit the stored
	 * messages otherwise we will run out of memory.
	 * This method should be called before setup(...)
	 * @param maxNumber
	 * @param minNumber
	 */
	public void setMaxMessagesToKeep(int maxNumber, int minNumber)
	{
		this.maxMessagesToStore = maxNumber;
		this.minMessagesToStore = minNumber;
	}
	
	
}
