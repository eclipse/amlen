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
package com.ibm.ima.mqcon.utils;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import com.ibm.ima.mqcon.msgconversion.TestHelper;

public class AsynchronousMqttListener implements MqttCallback{
	
	private TestHelper testClass = null;
	
	public AsynchronousMqttListener(TestHelper testClass)
	{
		this.testClass = testClass;
	}

	public void connectionLost(Throwable arg0) {
		
		if(Trace.traceOn())
		{
			Trace.trace("Connection was lost for the MQTT listener");
		}
		throw new RuntimeException(arg0.getCause());
		
	}

	public void deliveryComplete(IMqttDeliveryToken arg0) {
		
		if(Trace.traceOn())
		{
			Trace.trace("Delivery of message completed");
		}
	}

	public void messageArrived(String arg0, MqttMessage arg1) throws Exception {

		if(Trace.traceOn())
		{
			Trace.trace("Message with body: "+arg1.toString()+" arrived on destination " + arg0);
		}
		testClass.callback(arg1);
	}
	
	
	
}
