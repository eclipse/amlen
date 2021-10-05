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

import java.util.concurrent.ConcurrentHashMap;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttMessage;


public class MqttProducerListener implements MqttCallback {
	
	private ConcurrentHashMap<String, String> messageKey = null;
	private ConcurrentHashMap<MqttDeliveryToken, String> idMap = null;
	private MqttProducers testClass = null;
	
	/**
	 * The receivedKey is used by a producer to ensure delivery was complete 
	 * @param testClass
	 * @param messageKey
	 * @param receivedKey
	 */
	public MqttProducerListener(MqttProducers testClass, ConcurrentHashMap<String, String> messageKey, ConcurrentHashMap<MqttDeliveryToken, String> idMap)
	{
		this.testClass = testClass;
		this.messageKey = messageKey;
		this.idMap = idMap;
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
				testClass.trace.trace("Calling reconnect");
			}
			testClass.resetConnections();
		}
		else
		{
			if(testClass!= null && testClass.trace.traceOn())
			{
				testClass.trace.trace("Already reconnecting, just need to wait");
			}
		}
		
	}

	public void deliveryComplete(IMqttDeliveryToken arg0) {

		if(arg0.isComplete())
		{
			String msgContents = idMap.get(arg0);
			if(msgContents != null)
			{
				messageKey.put(msgContents, "DELIVERED");
				if(testClass!= null && testClass.trace.traceOn())
				{
					testClass.trace.trace("MQTT delivery was complete for token " + arg0);
				}
				idMap.remove(arg0);
			}
			else
			{
				if(testClass!= null && testClass.trace.traceOn())
				{
					testClass.trace.trace("Old message token delivered for messageId " + arg0.getMessageId() + " " + arg0);
				}
			}
			
		}
		else
		{
			if(testClass!= null && testClass.trace.traceOn())
			{
				testClass.trace.trace("MQTT delivery was not complete for messageID " + arg0.getMessageId());
			}
		}
		
	}

	public void messageArrived(String arg0, MqttMessage message)
			throws Exception {
	// do nothing here
		
	}

	

}
