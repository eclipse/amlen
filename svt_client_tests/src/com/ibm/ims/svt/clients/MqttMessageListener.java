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

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttMessage;



/**
 * This callback is used specifically to check for sequential messages from individual message producers.
 * The producers must be configured to send in messages in the form of <clientID>:<sequence number>
 * 
 *
 */
public class MqttMessageListener extends MqttSubscriberListener {

	private MqttSubscriberClient testClass = null;
	private boolean cleanSession = false;
	private HashMap<String, Integer> trackMessages = new HashMap<String, Integer>();
	
	/**
	 * 
	 * @param testClass - to be used to call back to the client
	 * @param clientIDs - a list of producer client IDs
	 * @param cleanSession - specifies whether the client is durable or non durable
	 */
	public MqttMessageListener(MqttSubscriberClient testClass, ArrayList<String> clientIDs, boolean cleanSession)
	{
		super();
		this.testClass = testClass;
		this.cleanSession = cleanSession;
		String clientList = "";
		for(int i=0; i<clientIDs.size(); i++)
		{
			trackMessages.put(clientIDs.get(i), -1);
			clientList=clientList+":"+clientIDs.get(i);
		}
		
		if(testClass!= null && testClass.trace!= null &&testClass.trace.traceOn())
		{
			testClass.trace.trace("A listener was set up to listen for messages from clients "+ clientList);
		}
		
	}
	
	/**
	 * If a connection was lost then get the client to reconnect
	 */
	public void connectionLost(Throwable arg0) {
			
		if(testClass!= null && testClass.trace!= null &&testClass.trace.traceOn())
		{
			testClass.trace.trace("MQTT connection was lost. Message=" + arg0.getMessage());
		}
		if(! testClass.isreconnecting())
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
	 * This call back should only be used for subscribers so ignore this method
	 */
	public void deliveryComplete(IMqttDeliveryToken arg0) {

		// do nothing in here
		
	}

	/**
	 * Called asynchronously when a message arrives
	 */
	public void messageArrived(String arg0, MqttMessage message)
			throws Exception {
	
		try
		{
			synchronized(trackMessages)
			{
				String msg = new String(message.getPayload());
				
				//Extract out the client ID and the sequence number
				String[] bodyElements = msg.split(":", -1);
				String clientID = bodyElements[0];
				int messageNumber = new Integer(bodyElements[1]);
				
				// Get the last message number received for this particular client
				int lastNumber = trackMessages.get(clientID);
				
				if(this.cleanSession)
				{
					// As we are non durable we may lose some messages but really we should make sure that the message sequence is greater than the last message
					if(messageNumber > lastNumber)
					{
						if(testClass!= null && testClass.trace != null && testClass.trace.traceOn())
						{
							testClass.trace.trace("Message number " + messageNumber + " was greater than " + lastNumber + " for non durable client " + clientID);
						}
						trackMessages.put(clientID, messageNumber);
					}
					else
					{
						if(testClass!= null && testClass.trace != null && testClass.trace.traceOn())
						{
							testClass.trace.trace("Message number " + messageNumber + " was NOT greater than " + lastNumber + " for non durable client " + clientID);
							testClass.trace.trace("FAILING TEST");
						}
						testClass.failTest("Message number " + messageNumber + " was NOT greater than " + lastNumber + " for non durable client " + clientID);
					}
				}
				else
				{
					// We are durable so we need to check that we do get the correct sequence of messages for that particular client
					if(messageNumber == (lastNumber+1))
					{
						if(testClass!= null && testClass.trace != null && testClass.trace.traceOn())
						{
							testClass.trace.trace("Message number " + messageNumber + " was the next number " + " for durable client " + clientID);
						}
						trackMessages.put(clientID, messageNumber);
					}
					else
					{
						if(testClass!= null && testClass.trace != null && testClass.trace.traceOn())
						{
							testClass.trace.trace("Message number " + messageNumber + " was NOT the next number after " + lastNumber + " for durable client " + clientID);
							testClass.trace.trace("FAILING TEST");
						}
						testClass.failTest("Message number " + messageNumber + " was NOT the next number after " + lastNumber + " for durable client " + clientID);
					}
				}
			}
			
		}
		catch(Exception e)
		{
			if(testClass!= null && testClass.trace!= null && testClass.trace.traceOn())
			{
				testClass.trace.trace("An exception occurred in onmessage " + e.getMessage());
			}
		}
		
	}


}
