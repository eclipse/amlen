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
package com.ibm.ima.mqcon.resiliency;

import java.util.ArrayList;
import java.util.concurrent.ConcurrentHashMap;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttMessage;


public class MqttConsumerListener implements MqttCallback {
	
	private ConcurrentHashMap<String, String> messageKey = null;
	private ConcurrentHashMap<String, String> receivedKey = null;
	private MqttSubscribers testClass = null;
	private ArrayList<String> duplicates = new ArrayList<String>();
	private boolean checkDuplicates = false;
	
	
	/**
	 * The receivedKey is used by a consumer to record which keys we received. 
	 * @param testClass
	 * @param messageKey
	 * @param receivedKey
	 */
	public MqttConsumerListener(MqttSubscribers testClass, ConcurrentHashMap<String, String> messageKey, ConcurrentHashMap<String, String> receivedKey, boolean checkDuplicates)
	{
		this.testClass = testClass;
		this.messageKey = messageKey;
		this.receivedKey = receivedKey;
		this.checkDuplicates = checkDuplicates;
	}

	
	public void connectionLost(Throwable arg0) {
		
		if(testClass!= null && testClass.trace.traceOn())
		{
			testClass.trace.trace("MQTT connection was lost. Message=" + arg0.getMessage());
		}
		if(! testClass.isreconnecting())
		{
			if(testClass!= null && testClass.trace.traceOn())
			{
				testClass.trace.trace("Attempting to reconnect");
			}
			testClass.resetConnections();
		}
		else
		{
			if(testClass!= null && testClass.trace.traceOn())
			{
				testClass.trace.trace("Already reconnecting so just wait");
			}
		}
		
	}

	public void deliveryComplete(IMqttDeliveryToken arg0) {

		// do nothing in here
		
	}

	public void messageArrived(String arg0, MqttMessage message)
			throws Exception {
	
		try
		{
			String msg = new String(message.getPayload());

			if(testClass!= null && testClass.trace.traceOn())
			{
				testClass.trace.trace("MQTT message=" + msg + " arrived on Topic=" + arg0.toString());
			}
					
			synchronized(receivedKey) 
			{ 
				if(messageKey.containsKey(msg))
				{
					
						receivedKey.put(msg, "Received");
						if(testClass!= null && testClass.trace.traceOn())
						{
							testClass.trace.trace("MQTT  Message=" + msg +  " was added to the received key");
						}
						if(checkDuplicates)
						{
							if(duplicates.contains(msg))
							{
								if(testClass!= null && testClass.trace.traceOn())
								{
									testClass.trace.trace("We found a duplicate message");
									System.out.println("DUPLICATE");
								}
								testClass.addDuplicate(msg);
							}
							else
							{
								duplicates.add(msg);
								if(testClass!= null && testClass.trace.traceOn())
								{
									testClass.trace.trace("Duplicate test was false");
								}
							}
							
						}
	
				}
				else
				{
					if(testClass!= null && testClass.trace.traceOn())
					{
						testClass.trace.trace("MQTT Message=" + msg + " was not found in the received key list");
					}
				}
			}
		}
		catch(Exception e)
		{
			if(testClass!= null && testClass.trace.traceOn())
			{
				testClass.trace.trace("An exception occurred in onmessage " + e.getMessage());
			}
		}
		
	}

	

}
