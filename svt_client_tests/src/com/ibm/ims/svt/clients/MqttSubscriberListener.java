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

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttMessage;



/**
 * This call back class can be used to do some checking of the messages received by this listener
 *
 */
public class MqttSubscriberListener implements MqttCallback {

	private ArrayList<RetainedMap> retainedMessages = null;
	private MqttSubscriberClient testClass = null;
	private ArrayList<String> receivedMsgs = new ArrayList<String>();
	private boolean checkDuplicates = false;
	private boolean cleanSession = false;
	private boolean afterProducer = true;
	private boolean connectionLost = false;
	private boolean checkRetainedDuplicates = false;
	private boolean storeReceivedMsgs = false;
	private long lastMessageReceived = 0;
	private long throttleReceive = 0;
	private boolean checkSharedSubMsg = false;
	private int maxMessagesToStore = -1;
	private int minMessagesToStore = -1;
	
	public MqttSubscriberListener()
	{
		
	}
	
	/**
	 * 
	 * @param testClass - provides a way to call back to the client
	 * @param checkDuplicates - if true check whether we get an duplicate messages
	 * @param storeReceivedMsgs - if true we will store the messages
	 */
	public MqttSubscriberListener(MqttSubscriberClient testClass, boolean checkDuplicates, boolean storeReceivedMsgs)
	{
		this.testClass = testClass;
		this.checkDuplicates = checkDuplicates;
		this.storeReceivedMsgs = storeReceivedMsgs;
	}
	
	
	/**
	 * This is called when we lose connection to the server. 
	 */
	public void connectionLost(Throwable arg0) {
			
		connectionLost = true;
	
		if(testClass!= null && testClass.trace!= null &&testClass.trace.traceOn())
		{
			testClass.trace.trace("MQTT connection was lost. Message=" + arg0.getMessage());
		}
		if(! testClass.isreconnecting()) // if we are not already reconnecting the client attempt to reconnect
		{
			if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
			{
				testClass.trace.trace("Attempting to reconnect");
			}
			testClass.resetConnections();
		}
		else
		{
			if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
			{
				testClass.trace.trace("Already reconnecting so just wait");
			}
		}
		
	}

	/**
	 * We are using this callback specifically for subscribers so we ignore this method
	 */
	public void deliveryComplete(IMqttDeliveryToken arg0) {

		// do nothing in here
		
	}

	/**
	 * This is called asynchronously when a message arrives for the subscriber
	 */
	public void messageArrived(String arg0, MqttMessage message)
			throws Exception {
	
		try
		{
			// This is used to determine when we last received a message
			lastMessageReceived = System.currentTimeMillis();
			
			String msg = new String(message.getPayload());
			
			if(retainedMessages != null) // only use it we want to check retained messages
			{
				if(afterProducer) // We were told we subscribed after the producer send a retained messages
				{
					// We expect to see the first message as retained 
					if(msg.contains(".RETAINED.") && message.isRetained())
					{
						if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
						{
							testClass.trace.trace("We got the expected first retained message and it was marked as retained");
						}
						
						retainedMessages.add(new RetainedMap(msg, true));
						
						afterProducer = false; // ok we can only do this once
					}
					else
					{
						if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
						{
							testClass.trace.trace("We expected the first message to be retained");
						}
						testClass.failTest("We expected the first message to be retained but it wasn't so failing the test " + msg);
						// Set the failure message in the client - the test can then decide when to stop/fail the global test
					}
					
				}
				else
				{
					if(connectionLost && cleanSession) // We know we should always get a retained message when we reconnect with a clean session
					{
						connectionLost = false;
						// we should get a retained message
						if(msg.contains(".RETAINED.") && message.isRetained())
						{
							if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
							{
								testClass.trace.trace("We got the expected first retained message after a reconnected and it was marked as retained");
							}
							
							retainedMessages.add(new RetainedMap(msg, true));
							
							
						}
						else
						{
							if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
							{
								testClass.trace.trace("We expected the first message after a reconnection to be retained");
							}
							testClass.failTest("We expected the first message to be retained but it wasn't so failing the test " + msg);
							// Set the failure message in the client - the test can then decide when to stop/fail the global test
							
						}
					}
					else
					{
						connectionLost = false;
						if(msg.contains(".RETAINED."))
						{
							// OK the message body contains the test RETAINED so we know the producer sent a retained message
							// However, we are not clean session so we are effectively live so it shouldn't be seen as a retained message
							if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
							{
								testClass.trace.trace("We got a retained message");
							}	
						
							if(message.isRetained())
							{
								// 	we are active so we would not expect any messages marked as retained
								if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
								{
									testClass.trace.trace("It was marked as retained but we are active so this is wrong");
								}
								testClass.failTest("The retained message was marked as retained but we are active so this is wrong " + msg);
								// Set the failure message in the client - the test can then decide when to stop/fail the global test
								
							}
							else
							{
								if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
								{
									testClass.trace.trace("The retained message was not marked as retained as we are active which is correct");
								}
								int retainedMsg = new Integer(msg.substring(0,msg.indexOf(".RETAINED")));
								
								if(checkRetainedDuplicates)
								{
									// 	now check for duplicates
									if(retainedMessages.contains(new RetainedMap(msg, false)) || retainedMessages.contains(new RetainedMap(msg, true)))
									{
										if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
										{
											testClass.trace.trace("We received a duplicate retained message which is not correct:" + retainedMsg);
										}
										testClass.failTest("We received a duplicate retained message which is not correct:" + retainedMsg);
										// Set the failure message in the client - the test can then decide when to stop/fail the global test
										
									}
									else
									{
										if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
										{
											testClass.trace.trace("Duplicate check for retained messages passed");
										}
									}
								}
								
								retainedMessages.add(new RetainedMap(msg, false));
							}
							
						}
						
					}
				}
				
				
			}
			
			synchronized(this) 
			{
				if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
				{
					testClass.trace.trace("MQTT  Message=" + msg +  " was received by the client");
				}
				if(checkDuplicates)
				{
					if(receivedMsgs.contains(msg) && (! message.isDuplicate())) // check we don't have the duplicate flag set
					{
						
						if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
						{
							
							testClass.failTest("We received a duplicate message " + msg);
							// Set the failure message in the client - the test can then decide when to stop/fail the global test
							
						}
					}
					else
					{
						receivedMsgs.add(msg);
						if(testClass!= null && testClass.trace.traceOn())
						{
							testClass.trace.trace("We passed the duplicate test");
						}
						
						if(this.maxMessagesToStore != -1 && receivedMsgs.size() > maxMessagesToStore)
						{
							this.removeOldStoredMsgs(receivedMsgs);
						}
					}	
				}
				else if(!checkDuplicates && storeReceivedMsgs)
				{
					receivedMsgs.add(msg); // only store the message if configured to do so
					
					if(this.maxMessagesToStore != -1 && receivedMsgs.size() > maxMessagesToStore)
					{
						this.removeOldStoredMsgs(receivedMsgs);
					}
					
				}
				if(checkSharedSubMsg)
				{
					if(message.isDuplicate()) // check we don't have the duplicate flag set
					{
						
						if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
						{
							
							testClass.trace.trace("We received a marked duplicate message from the servcer " + msg + " so will bypass the duplicate check");

						}
					}
					else
					{
						
						if(testClass instanceof MultiMqttSubscriberClient)
						{
							((MultiMqttSubscriberClient)testClass).checkSharedSubMsg(msg);
						}
						else
						{
							testClass.checkSharedSubMsg(msg); // only check a duplicate message across all shared subs if configured to do so
						}
						
					}
				}
				
			}
			
			// This can be used to slow the message processing down so we have some 'slow' subscribers
			if(throttleReceive != 0)
			{
				if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
				{
					testClass.trace.trace("Throttling the message receive for " + throttleReceive + " milliseconds");
				}
				
				try
				{
					Thread.sleep(throttleReceive);
				}catch(InterruptedException ie)
				{
					// do nothing
				}
							
				
				
				
				
			}
		}
		catch(Exception e)
		{
			// I don't want to do anything if a problem occurs
			if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
			{
				testClass.trace.trace("An exception occurred in onmessage " + e.getMessage());
			}
		}
		
	}

	/**
	 * This method can be used to configure the callback to check retained messages
	 * @param retainedList - to store retained messages
	 * @param cleanSession - determines if the first message back after failover should be retained or not
	 * @param afterProducer - if the consumer was started after a producer has sent a retained message we should see the first ever one as retained
	 * @param checkRetainedDuplicates
	 */
	public void checkRetainedMessages(ArrayList<RetainedMap> retainedList, boolean cleanSession, boolean afterProducer, boolean checkRetainedDuplicates)
	{
		this.retainedMessages = retainedList;
		this.cleanSession = cleanSession;
		this.afterProducer = afterProducer;
		this.checkRetainedDuplicates = checkRetainedDuplicates;
	}
	
	/**
	 * Used to get the messages received if store messages was set to true
	 * @return
	 */
	public ArrayList<String> getReceivedMsgs()
	{
		return receivedMsgs;
	}
	
	/**
	 * Used to wipe the stored messages
	 */
	public void clearReceivedMsgs()
	{
		receivedMsgs = new ArrayList<String>();
	}
	

	/**
	 * Used to get the last time a message was received
	 * @return
	 */
	public long getLastMessageTime()
	{
		return lastMessageReceived;
	}
	
	/**
	 * Sets the time we will sleep whilst processing received messages
	 * @param throttleTime
	 */
	public void throttleReceive(long throttleTime)
	{
		this.throttleReceive = throttleTime;
	}
	
	/**
	 * Used to specify this listener is part of a group of shared subscriptions and we need to pass the message to the client to check for dupes
	 * as we can't do it alone in this callback class.
	 */
	public void checkSharedSubMsgs()
	{
		this.checkSharedSubMsg = true;
	}
	
	/**
	 * Remove some of the messages we are storing as the test may run for several days and we do not want to run out of memory
	 * @param msgArray
	 */
	private void removeOldStoredMsgs(ArrayList<String> msgArray)
	{
		synchronized(msgArray)
		{
			if(minMessagesToStore != -1)
			{
				if(testClass!= null && testClass.trace.traceOn())
				{
					testClass.trace.trace("Clearing the messages stored down to " + (minMessagesToStore-1));
				}
				msgArray.subList(0, maxMessagesToStore-minMessagesToStore).clear();
			}
			else
			{
				if(testClass!= null && testClass.trace.traceOn())
				{
					testClass.trace.trace("Clearing the messages stored down to 99");
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
}
