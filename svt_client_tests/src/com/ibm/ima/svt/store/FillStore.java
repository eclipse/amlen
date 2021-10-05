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
package com.ibm.ima.svt.store;

import java.io.File;
import java.util.Random;
import java.util.UUID;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.config.ImaConfig.TYPE;
import com.ibm.ima.test.cli.monitor.ImaMonitoring;
import com.ibm.ims.svt.clients.Audit;

/**
 * This test is designed to send a mix of large and small messages
 * sizes to the appliance to fill up the generations. Once the memory is at the specified
 * amount it will delete the durable subscription containing the larger messages to free up the memory, 
 * resubscribe and then continue.
 *
 */
public class FillStore implements Runnable {
	
	private int nonPersistedMessageSize = 256;
	private int persistedMessageSize = 3;
	private int freeMemoryLimit = 30;
	private int numberMessages = 4000;
	
	private ImaConfig config = null;
	private ImaMonitoring monitor = null;
	
	private MessageProducer producerA = null;
	private MessageProducer producerB = null;
	
	private String clientId_A = "ABCDE";	
	private String clientAsubId = "abcde";
	private String topicA = "UnsavedMessages";
	private String clientId_B = "FGJHIJ";
	private String clientBsubId = "fghij";
	private int topicBCorrelationCount = 0;
	private String topicB = "SavedMessages";
	private int topicACorrelationCount = 0;

	
	private Audit trace = null;
	
	private String jmsipaddress = null;
	private String cliaddressA = null;
	private String cliaddressB = null;
	private int port = 16444;
	
	private ConnectionFactory cfClientA = null;
	private ConnectionFactory cfClientB = null;
	private ConnectionFactory cfProducerA = null;
	private ConnectionFactory cfProducerB = null;
	private Connection connClientA = null;
	private Connection connClientB = null;
	private Connection connProducerA = null;
	private Connection connProducerB = null;
	private Session sessClientA = null;
	private Session sessClientB = null;
	private Session sessionPA =null;
	private Session sessionPB = null;
	private Topic topicClientA = null;
	private Topic topicClientB = null;

	private MessageConsumer consumerA = null;
	private MessageConsumer consumerB = null;
	
	private int messageCount = 0;
	private String prefix=null;
	
	private long timeout = 5000000;
	
	public boolean keepRunning = true;	
	
	private int serverStopCount = 0;
	private ControlFailover serverControl = null;
	private boolean firstStop = true;
	private boolean forceStop = false;
	private boolean noFail = false;
	private long resynchTime = 0;
	private long timeBetweenFailovers = 3600000;
	private boolean mixedForced = false;
	private int mixedForcedCount = 0;
	private boolean tearDown = true;

	public FillStore(int numberMessages, String jmsipaddress, String cliaddressA, String cliaddressB, int port)
	{
		this.numberMessages = numberMessages;
		/*
		 * jmsipaddress may contain '+' as delimiter or ','. JMS api's use either <space> or comma for delimiter. Therefore,
		 * if '+' is used the convert to comma. This change is due to automation which cannot handle ',' as delimiter
		 */
		if(jmsipaddress.contains("+"))
		{	
			/*
			 * convert '+' to ','
			 */
			this.jmsipaddress = jmsipaddress.trim().replace('+',',');
		} else {
			/*
			 * Assume ',' is being used
			 * 
			 */
			this.jmsipaddress = jmsipaddress;
		}
		this.cliaddressA = cliaddressA;
		this.cliaddressB = cliaddressB;
		this.port = port;
		
	}
	
	public void setPrefix(String prefix)
	{
		this.prefix = prefix;
	}
	
	public void setClientIds(String clientA, String clientB)
	{
		clientId_A = clientA;
		clientId_B = clientB;
	}
	
	public void setTopicNames(String topicA, String topicB)
	{
		this.topicA = topicA;
		this.topicB = topicB;
	}
	
	public void setNonPersistedMessageSize(int size)
	{
		this.nonPersistedMessageSize = size;
	}
	
	public void setPersistedMessageSize(int size)
	{
		this.persistedMessageSize = size;
	}
	
	public void setFreeMemoryLimit(int limit)
	{
		this.freeMemoryLimit = limit;
	}
	
	public void setMixedForcedStop()
	{
		this.mixedForced = true;
	}
	
	public void setTimeout(long timeout)
	{
		this.timeout = timeout;
	}
	
	public void setForceStop(boolean force)
	{
		this.forceStop = force;
	}
	
	public void setNoFail(boolean failure)
	{
		this.noFail = failure;
	}
	
	public void setResynchTime(long time)
	{
		this.resynchTime = time;
	}
	
	public void setTimeBetweenFailovers(long time)
	{
		this.timeBetweenFailovers = time;
	}
	
	public void setTearDown(boolean tearDown)
	{
		this.tearDown = tearDown;
	}
	
	public void generateControlClient()
	{
		serverControl = new ControlFailover(cliaddressA, cliaddressB, timeout, forceStop, resynchTime);
	}
	
	public void setControlClient(ControlFailover serverControl)
	{
		this.serverControl = serverControl;
	}

	
	public void setup() throws Exception
	{
		if(cliaddressB != null)
		{
			config = new ImaConfig(cliaddressA,cliaddressB, "admin", "admin");			
			monitor = new ImaMonitoring(cliaddressA,cliaddressB, "admin", "admin");		
			
		}
		else
		{
			config = new ImaConfig(cliaddressA, "admin", "admin");			
			monitor = new ImaMonitoring(cliaddressA, "admin", "admin");		
		}
	
		config.connectToServer();
		monitor.connectToServer();
		
		if(! config.messageHubExists("FillStoreTest"))
		{
			config.createHub("FillStoreTest", "Used by Some Tester");
		}
		if(! config.connectionPolicyExists("FillStoreCP"))
		{
			config.createConnectionPolicy("FillStoreCP", "FillStoreCP", TYPE.JMS);
		}
		if(! config.messagingPolicyExists("FillStoreMP_T"))
		{
			config.createMessagingPolicy("FillStoreMP_T", "FillStoreMP_T", "*", TYPE.Topic, TYPE.JMS);
		}
		if(! config.endpointExists("FillStoreEP"))
		{
			config.createEndpoint("FillStoreEP", "FillStoreEP", Integer.toString(port), TYPE.JMS, "FillStoreCP", "FillStoreMP_T", "FillStoreTest");
			config.updateEndpoint("FillStoreEP", "MaxMessageSize", "262144KB");
		}		
		
		try
		{
			if(tearDown)
			{
				config.deleteSubscription(clientAsubId, clientId_A);
			}
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			if(tearDown)
			{
				config.deleteSubscription(clientBsubId, clientId_B);
			}
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.disconnectServer();
		}
		catch(Exception e)
		{
			// do nothing
		}
		
		
		File audit = new File("FillStore"+prefix);
		if(!audit.exists())
		{
			audit.createNewFile();
		}
		trace = new Audit("FillStore"+prefix, true);
		
		
			
		cfClientA = ImaJmsFactory.createConnectionFactory();
		((ImaProperties) cfClientA).put("Server", jmsipaddress);
		((ImaProperties) cfClientA).put("Port", port);
		
		cfClientB = ImaJmsFactory.createConnectionFactory();
		((ImaProperties) cfClientB).put("Server", jmsipaddress);
		((ImaProperties) cfClientB).put("Port", port);
		
		cfProducerA = ImaJmsFactory.createConnectionFactory();
		((ImaProperties) cfProducerA).put("Server", jmsipaddress);
		((ImaProperties) cfProducerA).put("Port", port);
		
		cfProducerB = ImaJmsFactory.createConnectionFactory();
		((ImaProperties) cfProducerB).put("Server", jmsipaddress);
		((ImaProperties) cfProducerB).put("Port", port);
		
		if(trace != null)
		{
			trace.trace("Set up complete");
			trace.trace("ClientA id="+ clientId_A);
			trace.trace("ClientB id="+ clientId_B);
			trace.trace("ClientA topic name="+ topicA);
			trace.trace("ClientB topic name="+ topicB);
			trace.trace("Persisted message size="+ persistedMessageSize);
			trace.trace("Non-persisted message size="+ nonPersistedMessageSize);
			trace.trace("Free memory limit="+ freeMemoryLimit);
			
		}
		
		try
		{
			config.disconnectServer();
			
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			monitor.connectToServer();
		}
		catch(Exception e)
		{
			// do nothing
		}
	}


	
	public void run() 
	{

		do{			
			
			try
			{
				try
				{
					connClientA.close();
				}
				catch(Exception e)
				{
					// do nothing
				}
				try
				{
					connClientB.close();
				}
				catch(Exception e)
				{
					// do nothing
				}
				try
				{
					connProducerA.close();
				}
				catch(Exception e)
				{
					// do nothing
				}
				try
				{
					connProducerB.close();
				}
				catch(Exception e)
				{
					// do nothing
				}
				
				connClientA = cfClientA.createConnection();
				
				connClientA.setClientID(clientId_A);	
				sessClientA = connClientA.createSession(false, Session.AUTO_ACKNOWLEDGE);
				topicClientA = sessClientA.createTopic(topicA);
				consumerA = sessClientA.createDurableSubscriber(topicClientA, clientAsubId);
				connClientA.start();
				
				
				connClientB = cfClientB.createConnection();
				
				connClientB.setClientID(clientId_B);	
				sessClientB = connClientB.createSession(false, Session.AUTO_ACKNOWLEDGE);
				topicClientB = sessClientB.createTopic(topicB);
				consumerB = sessClientB.createDurableSubscriber(topicClientB, clientBsubId);
				connClientB.start();			
				
				connProducerA = cfProducerA.createConnection();
				connProducerA.setClientID(UUID.randomUUID().toString().substring(0, 15));
				sessionPA = connProducerA.createSession(false, Session.AUTO_ACKNOWLEDGE);
				Topic topic_AP = sessionPA.createTopic(topicA);
				producerA = sessionPA.createProducer(topic_AP);
				
				connProducerB = cfProducerB.createConnection();
				connProducerB.setClientID(UUID.randomUUID().toString().substring(0, 15));
				sessionPB = connProducerA.createSession(false, Session.AUTO_ACKNOWLEDGE);
				Topic topic_BP = sessionPB.createTopic(topicB);
				producerB = sessionPB.createProducer(topic_BP);
				/*
				 * Before closing, empty all messages from ImaMessageConsume.receivedMessages
				 * which is a LinkedBlockingQueue<> type.
				 */
				while (consumerA.receiveNoWait() != null) {};
				while (consumerB.receiveNoWait() != null) {};
				consumerA.close(); // stop listening
				consumerB.close();
				trace.trace("All consumers and producers set up");
				
				
				do
				{
					// 	fill each section up
						for(int i=0; i<17; i++)
						{
							if(i != 8)
							{
								Message msg  = null;
								if(i==9)
								{
									msg = sessClientA.createTextMessage(createLargeMessage((nonPersistedMessageSize - 1), (1048576 - (persistedMessageSize * 1024))));
									trace.trace("Creating 255 plus extra MB message for topicA");
									
								}
								else
								{
									trace.trace("Creating " +  nonPersistedMessageSize + "MB message for topicA");
									msg = sessClientA.createTextMessage(createLargeMessage(nonPersistedMessageSize, 0));
								}
								msg.setJMSCorrelationID(Integer.toString(topicACorrelationCount));
								topicACorrelationCount++;
								producerA.send(msg);
								messageCount++;
								serverStopCount++;
								trace.trace("Sent message to topic A with correclationID="+msg.getJMSCorrelationID());
							}
							else
							{
								Message msg  = null;
								msg = sessClientB.createTextMessage(createSmallMessage(persistedMessageSize));
								msg.setJMSCorrelationID(Integer.toString(topicBCorrelationCount));							
								trace.trace("Sending " +  persistedMessageSize + "KB message to topicB");
								topicBCorrelationCount++;
								producerB.send(msg);
								messageCount++;
								serverStopCount++;
								trace.trace("Sent message to topic B with correclationID="+msg.getJMSCorrelationID());
							}
							
						}
						
						checkMemory();
						
						if(noFail)
						{
							trace.trace("We are not designed to fail so will just keep going");
						}
						else
						{
							trace.trace("Server count="+serverStopCount);
						
							if(serverStopCount > 250)
							{
								
								if(firstStop)
								{
									if(serverControl == null)
									{
										generateControlClient();
									}
									serverStopCount = 0;
									if(mixedForced)
									{
										trace.trace("Mixed failover. Will be using a forced stop="+(mixedForcedCount%2==0));
										serverControl.triggerFail(mixedForcedCount%2==0);
										mixedForcedCount++;
									}
									else
									{
										serverControl.triggerFail();
									}
									firstStop = false;
								}
								else
								{
									if(! serverControl.isStillRecovering())
									{
										trace.trace("Checking to make sure we do not failover within " + this.timeBetweenFailovers + " millseconds of previous one");
										if((System.currentTimeMillis() - serverControl.returnTimeCompleted()) > timeBetweenFailovers)
										{
											trace.trace("Previous failover was more than the configured time between failovers");
											serverStopCount = 0;
											if(mixedForced)
											{
												trace.trace("Mixed failover. Will be using a forced stop="+(mixedForcedCount%2==0));
												serverControl.triggerFail(mixedForcedCount%2==0);
												mixedForcedCount++;
											}
											else
											{
												serverControl.triggerFail();
											}
											
										}
										else
										{
											trace.trace("Previous failover finished less than the configured time between failures ago so keep going");
										}
									}
									else
									{
										trace.trace("Server is still coming back up from the previous test so continuing to send messages=" + serverControl.isStillRecovering());
									}
								}
								
							
							}
						}
						
				}while(keepRunning & (messageCount < numberMessages));
				
				if(messageCount >= numberMessages)
				{
					keepRunning = false;
					System.out.println("ALL MESSAGES SENT TO STORE");
					trace.trace("Test finished");
					
				}
				
			}
			catch(Exception e)
			{
				trace.trace("An exception occurred - checking memory");
				try
				{
					checkMemory();
				}
				catch(Exception ef)
				{
					// do nothing
				}
				trace.trace("An general exception occurred - sleeping for 5 seconds then will try again");
				try {
					Thread.sleep(5000);
				} catch (InterruptedException e1) {

					
				}
			}
			
		}while(keepRunning);
		
		
		try
		{
			producerA.close();
		}
		catch(Exception ex)
		{
			// 	do nothing
		}
		try
		{
			producerB.close();
		}
		catch(Exception ex)
		{	
			// 	do nothing
		}
		
		try
		{
			connClientA.close();
		}
		catch(Exception ex)
		{
			// do nothing
		}
		try
		{
			connClientB.close();
		}
		catch(Exception ex)
		{
			// do nothing
		}
		try
		{
			connProducerA.close();
		}
		catch(Exception ex)
		{
			// do nothing
		}
		try
		{
			connProducerB.close();
		}
		catch(Exception ex)
		{
			// do nothing
		}
		
		if(!config.isConnected())
		{
			try
			{
				config.connectToServer();
			}
			catch(Exception ef)
			{
				// do nothing
			}
		}
		try
		{
			config.deleteSubscription(clientAsubId, clientId_A);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteSubscription(clientBsubId, clientId_B);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteEndpoint("FillStoreEP");
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy("FillStoreMP_T");
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteConnectionPolicy("FillStoreCP");
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessageHub("FillStoreTest");
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.disconnectServer();
		}
		catch(Exception ez)
		{
			// do nothing
		}
		
		
	}

	public void checkMemory()throws Exception
	{
		if(!monitor.isConnected())
		{
			try
			{
				monitor.connectToServer();
			}
			catch(Exception ef)
			{
				// do nothing
			}
		}
		double currentStorePercentage = monitor.getStoreStatistics().getDiskUsedPercent();
		double currentMemoryPercentage = monitor.getMemoryStatistics().getMemoryFreePercent();
		
		trace.trace("currentStorePercentage="+currentStorePercentage);
		trace.trace("currentMemoryPercentage="+currentMemoryPercentage);
		if(currentMemoryPercentage < freeMemoryLimit)
		{
			trace.trace("Unsubscribing clientA and clientB to remove stored messages");
			sessClientA.unsubscribe(clientAsubId);
			sessClientB.unsubscribe(clientBsubId);
			consumerA = sessClientA.createDurableSubscriber(topicClientA, clientAsubId);
			consumerB = sessClientB.createDurableSubscriber(topicClientB, clientBsubId);
			
			/*((ImaMessageConsumer)(consumerA.))receivedMessages.clear();*/
			/*
			 * Before closing, empty all messages from ImaMessageConsume.receivedMessages
			 * which is a LinkedBlockingQueue<> type.
			 */
			while (consumerA.receiveNoWait() != null) {};
			while (consumerB.receiveNoWait() != null) {};

			consumerA.close();
			consumerB.close();
		
		}
		try
		{
			monitor.disconnectServer();
		}
		catch(Exception ef)
		{
			// do nothing
		}
		
	}
	
	public static String createLargeMessage(int size, int extraBits)
	{
		
		Random randomGenerator = new Random();
		StringBuilder messageBody = new StringBuilder();
		long messageSizeBytes = size * 1024 * 1024 + extraBits;
		for(int x = 0;x<messageSizeBytes;x++){
			int randomInt = randomGenerator.nextInt(10);
			messageBody.append(randomInt);
		}
		return messageBody.toString();
	}
	
	public static String createSmallMessage(int size)
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
