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

import java.util.ArrayList;

import javax.jms.Message;
import javax.jms.MessageListener;
import javax.jms.TextMessage;

/**
 * This class is used to asynchronously receive messages
 *
 */
public class JmsListener implements MessageListener{
	
	private JmsConsumerClient subscriber = null;
	private int batchAck = 0;
	private int increment = 0;
	private ArrayList<String> waitingForCompletion = new ArrayList<String>();
	private int numberOfMessagesCompleted = 0;
	private boolean clearMsgs = false;
	private boolean transacted = false;
	private int transactedNum = 0;
	private ArrayList<RetainedMap> retainedKey = null;
	private ArrayList<String> receivedMessages = new ArrayList<String>();
	
	private boolean afterProducer = false;
	private boolean checkRetainedDuplicates = false;
	private boolean checkDuplicates = false;
	private boolean durable = false;
	private String jmsFlag = "JMS_IBM_Retain";
	private boolean connectionLost = false;
	private boolean storeReceivedMsgs = false;
	
	protected long lastMessageReceived = 0;
	private int maxMessagesToStore = -1;
	private int minMessagesToStore = -1;
	private boolean checkSharedSubMsg = false;
	
	private int id = -1;
	
	
	public JmsListener()
	{
		
	}
	/**
	 * This method can be used to create a consumer which will auto ack the messages
	 * @param testClass
	 */
	public JmsListener(JmsConsumerClient testClass, boolean storeReceivedMsgs)
	{
		this.subscriber = testClass;
		this.storeReceivedMsgs = storeReceivedMsgs;
		
		if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
		{
			subscriber.trace.trace("A listener was set up correctly with auto acknowledgement");
		}
	}
	
	/**
	 * This method can be used to create a consumer which will manually ack the messages
	 * @param testClass
	 * @param batchAck
	 */
	public JmsListener(JmsConsumerClient testClass, int batchAck, boolean storeReceivedMsgs)
	{
		this.subscriber = testClass;
		this.batchAck = batchAck;
		this.storeReceivedMsgs = storeReceivedMsgs;
		
		if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
		{
			subscriber.trace.trace("A listener was set up correctly with client acknowledgement");
		}
	}
	
	/**
	 * This consumer can be used to create a consumer which will be in a transacted session. It's the calling classes
	 * responsibility to handle the setup of the session correctly.
	 * @param testClass
	 * @param batchAck
	 * @param transacted
	 * @param transactedNum
	 */
	public JmsListener(JmsConsumerClient testClass, boolean transacted, int transactedNum, boolean storeReceivedMsgs)
	{
		this.subscriber = testClass;
		this.transacted = transacted;
		this.transactedNum = transactedNum;
		this.storeReceivedMsgs = storeReceivedMsgs;
		
		if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
		{
			subscriber.trace.trace("A listener was set up correctly for a transacted session");
		}
	}
	
	/**
	 * This method should be called by the test class if we experienced an exception connecting to the server, as we have a new session
	 * we cannot ack/commit previous messages on our list waiting to be ack'ed
	 */
	public void clearMsgs()
	{
		clearMsgs = true;
	}
	

	/**
	 * Method for message processing
	 */
	public void onMessage(Message arg0) {
		
		try
		{
			lastMessageReceived = System.currentTimeMillis();
			String msg = ((TextMessage)arg0).getText();

			if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
			{
				subscriber.trace.trace("JMS Message=" + msg +  " was received from destination=" + arg0.getJMSDestination());
			}
				
			synchronized(this) 
			{ 
				
				if(retainedKey != null)
				{
					if(afterProducer) // we have told this consumer it started after the producer and thus should get a retained message as the first
					{
						// We expect to see the first message as retained i.e. we started after a retained
						// message was stored on the system
						if(msg.contains(".RETAINED."))
						{
							try
							{
								int retainedFlag = arg0.getIntProperty(jmsFlag);
								if(retainedFlag == 1)
								{
									if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
									{
										subscriber.trace.trace("We got the expected first retained message and it was marked as retained");
									}
								}
								else
								{
									if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
									{
										subscriber.trace.trace("We expected the first message to be retained");
									}
									subscriber.failTest("We expected the first message to be retained " + msg);
								}
							}
							catch(NumberFormatException mfe)
							{
								if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
								{
									subscriber.trace.trace("We expected the first message to be retained");
								}
								subscriber.failTest("We expected the first message to be retained " + msg);
							}
							
							
							retainedKey.add(new RetainedMap(msg, true));							
							afterProducer = false;

						}
						
					}
					else
					{
						if(connectionLost && !durable)
						{
						
							connectionLost = false;
						
							int retainedFlag = -1; 
							try
							{
								retainedFlag = arg0.getIntProperty(jmsFlag);
							}
							catch(NumberFormatException nfe)
							{
								if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
								{
									subscriber.trace.trace("We expected the first message after a reconnection to be retained as we are not durable");
								}
								subscriber.failTest("We expected the first message to be retained after a reconnection " + msg);
								
							}
							
							if(msg.contains(".RETAINED.") && retainedFlag == 1)
							{
								if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
								{
									subscriber.trace.trace("We got the expected first retained message after a reconnected and it was marked as retained as we are not durable");
								}	
								retainedKey.add(new RetainedMap(msg, true));
							}
							else
							{
								
								if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
								{
									subscriber.trace.trace("We expected the first message after a reconnection to be retained as we are not durable " + msg);
								}
								subscriber.failTest("We expected the first message to be retained after a reconnection " + msg);	
							}
						}
						else
						{
							connectionLost = false;
							
							if(msg.contains(".RETAINED."))
							{
								if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
								{
									subscriber.trace.trace("We got a retained message");
								}	
								
								int retainedFlag = -1; 
								try
								{
									retainedFlag = arg0.getIntProperty(jmsFlag);
								}
								catch(NumberFormatException nfe)
								{
									// it is not retained (mq) so set to 0
									retainedFlag = 0;
								}
							
								if(retainedFlag == 1)
								{
									// 	we are active so we would not expect any messages marked as retained
									if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
									{
										subscriber.trace.trace("It was marked as retained but we are active so this is wrong");
									}
									subscriber.failTest("The retained message was marked as retained but we are active so this is wrong " + msg);
									
								}
								else
								{
									if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
									{
										subscriber.trace.trace("The retained message was not marked as retained as we are active");
									}
									
								
									if(checkRetainedDuplicates)
									{
										// 	now check for duplicates
										if(retainedKey.contains(new RetainedMap(msg, false)) || retainedKey.contains(new RetainedMap(msg, true)))
										{
											if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
											{
												subscriber.trace.trace("We received a duplicate retained message which is not correct:" + msg);
											}
											subscriber.failTest("We received a duplicate retained message which is not correct: " + msg);
										
										}
										else
										{
											if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
											{
												subscriber.trace.trace("Duplicate check for retained messages passed");
											}
										}
									}
									
									retainedKey.add(new RetainedMap(msg, false));
								}	
							}
						}
					}
				}
								
				if(transacted)
				{
					// check if we need to commit the session yet 
					if(transactedNum == -1)
					{
						if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
						{
							subscriber.trace.trace("We are not committing any messages so will not add any messages to the list");
						}
					}
					else if(transactedNum == 0)
					{
						try
						{
							subscriber.commitSession(id);
							
							if(checkDuplicates)
							{
								if(arg0.getJMSRedelivered())
								{
									if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
									{
										subscriber.trace.trace("The JMSRedelivered was set so not checking for unknown duplicates");
									}
								}
								else 
								{
									checkForDuplicateMsg(msg);
								}
							}
							else if(!checkDuplicates && storeReceivedMsgs)
							{
								receivedMessages.add(msg);
								if(maxMessagesToStore != -1 && receivedMessages.size() > maxMessagesToStore)
								{
									removeOldStoredMsgs(receivedMessages);
								}
							}
							
							if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
							{
								subscriber.trace.trace("We are at the required level to commit the transaction");
							}
						}
						catch(Exception e)
						{
							if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
							{
								subscriber.trace.trace("We were unable to commit the transaction");
							}
							connectionLost();
						}
						
					}
					else
					{
						increment ++;
						if(increment == transactedNum)
						{
							if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
							{
								subscriber.trace.trace("We are at the required level to commit the transaction");
							}
						
							try
							{
								subscriber.commitSession(id);
								increment = 0;
								numberOfMessagesCompleted++;
								if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
								{
									subscriber.trace.trace("We have committed over " + numberOfMessagesCompleted + " messages so far");
								}
								if(checkDuplicates)
								{
									if(arg0.getJMSRedelivered())
									{
										if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
										{
											subscriber.trace.trace("The JMSRedelivered was set so not checking for duplicates");
										}
									}
									else 
									{
										checkForDuplicateMsg(msg);
									}
								}
								else if(!checkDuplicates && storeReceivedMsgs)
								{
									receivedMessages.add(msg);
									if(maxMessagesToStore != -1 && receivedMessages.size() > maxMessagesToStore)
									{
										removeOldStoredMsgs(receivedMessages);
									}
								}
								for(int i=0; i<waitingForCompletion.size(); i++)
								{
									
									String oldMsg = waitingForCompletion.get(i);
									numberOfMessagesCompleted++;
									// now commit the ones that were part of this batch waiting for commit 
									if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
									{
										subscriber.trace.trace("We have committed old msg " + oldMsg);
										subscriber.trace.trace("We have committed over " + numberOfMessagesCompleted + " messages so far");
									}
									if(storeReceivedMsgs)
									{
										receivedMessages.add(oldMsg);
									}
									
								
								}
								waitingForCompletion = new ArrayList<String>();
							}
							catch(Exception e)
							{
								if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
								{
									subscriber.trace.trace("We were unable to commit the transaction");
								}
								connectionLost();
							}

						}
						else
						{
							// We aren't expecting to commit any messages just yet so add them to the list
							if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
							{
								subscriber.trace.trace("We are not at the specified batch level yet so not committing");
							}
							if(checkDuplicates)
							{
								if(arg0.getJMSRedelivered())
								{
									if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
									{
										subscriber.trace.trace("The JMSRedelivered was set so not checking for duplicates");
									}
								}
								else 
								{
									checkForDuplicateMsg(msg);
								}
							}
							waitingForCompletion.add(msg);
						}
					}		
					
				}
				else
				{
					if(batchAck == 0)
					{
						// We aren't expecting to ack any messages so just add them to the list
						if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
						{
							subscriber.trace.trace("No acknowledgement of messages required so not adding to any list");
						}
						if(checkDuplicates)
						{
							if(arg0.getJMSRedelivered())
							{
								if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
								{
									subscriber.trace.trace("The JMSRedelivered was set so not checking for duplicates");
								}
							}
							else 
							{
								checkForDuplicateMsg(msg);
							}
						}
						else if(!checkDuplicates && storeReceivedMsgs)
						{
							receivedMessages.add(msg);
							if(maxMessagesToStore != -1 && receivedMessages.size() > maxMessagesToStore)
							{
								removeOldStoredMsgs(receivedMessages);
							}
						}
				
					}
					else
					{
						// check if we need to ack them yet 
						increment ++;
						if(increment == batchAck)
						{
							if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
							{
								subscriber.trace.trace("We are at the required level to ack so going to ack the message");
							}
							
							try
							{
								arg0.acknowledge();
								increment = 0;
								numberOfMessagesCompleted++;
								if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
								{
									subscriber.trace.trace("We have acked over " + numberOfMessagesCompleted + " messages so far");
								}
								if(checkDuplicates)
								{
									if(arg0.getJMSRedelivered())
									{
										if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
										{
											subscriber.trace.trace("The JMSRedelivered was set so not checking for duplicates");
										}
									}
									else 
									{
										checkForDuplicateMsg(msg);
									}
								}
								else if(!checkDuplicates && storeReceivedMsgs)
								{
									receivedMessages.add(msg);
									if(maxMessagesToStore != -1 && receivedMessages.size() > maxMessagesToStore)
									{
										removeOldStoredMsgs(receivedMessages);
									}
								}
									
								for(int i=0; i<waitingForCompletion.size(); i++)
								{
									String oldMsg = waitingForCompletion.get(i);
									numberOfMessagesCompleted++;
									// now correct the ones that were part of this batch ack
									if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
									{
										subscriber.trace.trace("We have acked old message " + oldMsg);
										subscriber.trace.trace("We have acked over " + numberOfMessagesCompleted + " messages so far");
									}
									
									if(storeReceivedMsgs)
									{
										receivedMessages.add(oldMsg);
									}
															
								}
								waitingForCompletion = new ArrayList<String>();
							}
							catch(Exception e)
							{
								if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
								{
									subscriber.trace.trace("We were unable to ack the message");
								}
								connectionLost();
							}
							
									
						}
						else
						{
							// We aren't expecting to ack any messages so just add them to the list
							if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
							{
								subscriber.trace.trace("We are not at the batch level '" + batchAck + "' yet so not acking");
							}
							if(checkDuplicates)
							{
								if(arg0.getJMSRedelivered())
								{
									if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
									{
										subscriber.trace.trace("The JMSRedelivered was set so not checking for duplicates");
									}
								}
								else 
								{
									checkForDuplicateMsg(msg);
								}
							}
							waitingForCompletion.add(msg);
						}
					}		
				
				}
			}

		}
		catch(Exception e)
		{
			// We should not expect any exceptions here as this is all handled by the calling class. 
			// If we do we will print the stack and effectively exit
			e.printStackTrace();
		}
		
	}
	
	private void checkForDuplicateMsg(String msg)
	{
		if(receivedMessages.contains(msg))
		{
			if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
			{
				subscriber.trace.trace("We received a duplicate message which is not correct for :" + msg);
			}
			subscriber.failTest("We received a duplicate message which is not correct for :" + msg);
		}
		else if(checkSharedSubMsg)
		{
			if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
			{
				subscriber.trace.trace("Checking for duplicates across shared subscription");
			}
			if(subscriber instanceof MultiJmsConsumerClient)
			{
				((MultiJmsConsumerClient)subscriber).checkSharedSubMsg(msg);
			}
			else
			{
				subscriber.checkSharedSubMsg(msg);
			}
			
		}
		else
		{
			if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
			{
				subscriber.trace.trace("Passed duplicate check");
			}
			receivedMessages.add(msg);
			
			if(maxMessagesToStore != -1 && receivedMessages.size() > maxMessagesToStore)
			{
				removeOldStoredMsgs(receivedMessages);
			}
		}
	}
	
	/**
	 * This is used to specify that the listener should check for retained messages after a failover/server restart etc
	 * @param durable - if false we should get a retained message after any outage if one is on the topic.
	 * @param retainedKey - used to store the retained messages
	 * @param afterProducer - if true then the first ever message we get should be a retained msg
	 * @param checkRetainedDuplicates
	 */
	public void checkRetainedMessage(boolean durable, ArrayList<RetainedMap> retainedKey, boolean afterProducer, boolean checkRetainedDuplicates)
	{
		this.retainedKey = retainedKey;
		this.durable = durable;
		this.afterProducer = afterProducer;
		this.checkRetainedDuplicates = checkRetainedDuplicates;
	}
	
	/**
	 * Specifies that this listener should check for duplicates.
	 */
	public void checkDuplicates()
	{
		checkDuplicates = true;
	}
	
	/**
	 * Used to remove any unacked or uncommitted messages after an outage
	 */
	public void connectionLost()
	{
		this.connectionLost = true;
		if(clearMsgs)
		{
			// we have restarted and cannot ack the previous messages so clear all lists
			increment = 0;	
			if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
			{
				subscriber.trace.trace("The server was restarted so clearing the backlog of messages that were not acked");
			}
			for(int i=0; i<waitingForCompletion.size(); i++)
			{
				String oldMsg = waitingForCompletion.get(i);
				// now remove the ones we didn't ack from the list
				if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
				{
					subscriber.trace.trace("We have removed message " + oldMsg +  " from the list as ACKS have been cleared");
				}
				if(this.retainedKey != null && this.durable)
				{
					if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
					{
						subscriber.trace.trace("We have removed message " + oldMsg +  " from the list of retained messages as we didn't ack/commit it");
					}
					retainedKey.remove(new RetainedMap(oldMsg, false));
				}
				
			}
			waitingForCompletion = new ArrayList<String>();
			clearMsgs=false;
		}
	}
	
	/**
	 * Returns the list of stored messages
	 * @return
	 */
	public ArrayList<String> getReceivedMsgs()
	{
		return receivedMessages;
	}
	
	/**
	 * Returns the time of the last received message
	 * @return
	 */
	public long getLastMessageTime()
	{
		return lastMessageReceived;
	}
	
	/**
	 * Can be used to give this listener a specific identity name
	 * @param i
	 */
	public void setID(int i)
	{
		this.id = i;
	}
	
	/**
	 * Used to remove old stored message so we do not blow the memory if running for a very long time.
	 * @param msgArray
	 */
	private void removeOldStoredMsgs(ArrayList<String> msgArray)
	{
		synchronized(msgArray)
		{
			if(minMessagesToStore != -1)
			{
				if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
				{
					subscriber.trace.trace("Clearing the messages stored down to " + (minMessagesToStore-1));
				}
				msgArray.subList(0, maxMessagesToStore-minMessagesToStore).clear();
			}
			else
			{
				if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
				{
					subscriber.trace.trace("Clearing the messages stored down to 99");
				}
				msgArray.subList(0, maxMessagesToStore-100).clear();
			}
		}
	}
	
	/**
	 * This is used to reduced the amount of messages to keep for any particular test.
	 * If the test is going to be run over several days we need to limit the stored
	 * messages otherwise we will run out of memory.
	 * @param maxNumber
	 * @param minNumber
	 */
	public void setMaxMessagesToKeep(int maxNumber, int minNumber)
	{
		this.maxMessagesToStore = maxNumber;
		this.minMessagesToStore = minNumber;
	}
	
	
	/**
	 * Used to specify this listener is part of a group of shared subscriptions and we need to pass the message to the client to check for dupes
	 * as we can't do it alone in this callback class.
	 */
	public void checkSharedSubMsgs()
	{
		this.checkSharedSubMsg = true;
	}


}
