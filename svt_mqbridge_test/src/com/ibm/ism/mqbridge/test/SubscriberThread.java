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
package com.ibm.ism.mqbridge.test;

import com.ibm.micro.client.mqttv3.MqttClient;
import com.ibm.micro.client.mqttv3.MqttConnectOptions;

public class SubscriberThread implements Runnable {

	public void run() {
		System.out.println("starting subscriber thread");
	    Example.clientId = String.format(
	        "%-23.23s",
	        (System.getProperty("user.name") + "_" + System.getProperty("clientId",
	            "Subscribe."))).trim();
	    try {
	    //	for (String topicString: PubSub.getSent()) { 
	    	String topicString = Example.topicString;
	    		String clientID = Example.clientId;
	      MqttClient client = new MqttClient(Example.TCPAddress, clientID);
	      CallBack callback = new CallBack(Example.clientId);
	      client.setCallback(callback);
	      MqttConnectOptions conOptions = new MqttConnectOptions();
	      conOptions.setCleanSession(Example.cleanSession);
	      client.connect(conOptions);
	      System.out.println("Subscribing to topic \"" + topicString
	          + "\" for client instance \"" + client.getClientId()
	          + "\" using QoS " + Example.QoS + ". Clean session is "
	          + Example.cleanSession);
	      client.subscribe(topicString, Example.QoS);
	      System.out.println("Going to sleep for " + Example.sleepTimeout / 1000
	          + " seconds");
	      Thread.sleep(Example.sleepTimeout);
	      client.disconnect();
	    
	      System.out.println("Finished");
	    } catch (Exception e) {
	      e.printStackTrace();
	    }		
	}
}
