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
import java.util.HashMap;

import javax.jms.Message;
import javax.jms.TextMessage;

/**
 * This class is a very specific asynchronous listener. To be used when producers send in messages with the format
 * <clientID>:<sequence number>
 *
 */
public class JmsMessageListener extends JmsListener{
	
	private JmsConsumerClient subscriber = null;
	private boolean durable = false;
	private HashMap<String, Integer> trackMessages = new HashMap<String, Integer>();
	
	
	/**
	 * This method can be used to create a consumer who will keep track of messages arising from the following clients
	 * @param testClass
	 * @param clientIDs - the producers client IDs for the destination
	 * @param durable - if durable we should receive all persistent messages even after a failover.
	 */
	public JmsMessageListener(JmsConsumerClient testClass, ArrayList<String> clientIDs, boolean durable)
	{
		super();
		this.subscriber = testClass;
		this.durable = durable;
		String clientList = "";
		for(int i=0; i<clientIDs.size(); i++)
		{
			trackMessages.put(clientIDs.get(i), -1);
			clientList=clientList+":"+clientIDs.get(i);
		}
		
		if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
		{
			subscriber.trace.trace("A listener was set up to listen for messages from clients "+ clientList);
		}
	}
	
	
	/**
	 * Method for message processing
	 */
	public void onMessage(Message arg0) {
		
		try
		{
			lastMessageReceived = System.currentTimeMillis();
			synchronized(trackMessages)
			{
				TextMessage message = (TextMessage)arg0;
				String[] bodyElements = message.getText().split(":", -1);
				String clientID = bodyElements[0];
				int messageNumber = new Integer(bodyElements[1]);
				
				int lastNumber = trackMessages.get(clientID);
				
				if(!durable)
				{
					if(messageNumber > lastNumber)
					{
						if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
						{
							subscriber.trace.trace("Message number " + messageNumber + " was greater than " + lastNumber + " for non durable client " + clientID);
						}
						trackMessages.put(clientID, messageNumber);
					}
					else
					{
						if(arg0.getJMSRedelivered())
						{
							if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
							{
								subscriber.trace.trace("This message has been redelivered so we know we probably have a duplicate");
							}
						}
						else
						{
							if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
							{
								subscriber.trace.trace("Message number " + messageNumber + " was NOT greater than " + lastNumber + " for non durable client " + clientID);
								subscriber.trace.trace("FAILING TEST");
							}
							subscriber.failTest("Message number " + messageNumber + " was NOT greater than " + lastNumber + " for non durable client " + clientID);
						}
					}
				}
				else
				{
					if(messageNumber == (lastNumber+1))
					{
						if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
						{
							subscriber.trace.trace("Message number " + messageNumber + " was the next number " + " for durable client " + clientID);
						}
						trackMessages.put(clientID, messageNumber);
					}
					else
					{
						if(arg0.getJMSRedelivered())
						{
							if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
							{
								subscriber.trace.trace("This message has been redelivered so we know we probably have a duplicate");
							}
						}
						else
						{
							if(subscriber!= null && subscriber.trace != null && subscriber.trace.traceOn())
							{
								subscriber.trace.trace("Message number " + messageNumber + " was NOT the next number after " + lastNumber + " for durable client " + clientID);
								subscriber.trace.trace("FAILING TEST");
							}
							subscriber.failTest("Message number " + messageNumber + " was NOT the next number after " + lastNumber + " for durable client " + clientID);
						}
					}
				}
			}
		}
		catch(Exception e)
		{
			// do nothing
		}
		
	}
	


}
