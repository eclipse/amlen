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
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

import com.ibm.micro.client.mqttv3.*;
public class PubSync {
  public static void main2(String[] args) {
    try {
      MqttClient client = new MqttClient(Example.TCPAddress, Example.clientId);
      MqttTopic topic = client.getTopic(Example.topicString);
      MqttMessage message;      
      client.connect();
      String publication;
      int qos=0;
      String [] topics = {"/Sport/Football/Results/Chelsea", "/Sport/Rugby/Results", Example.topicString };
      
      for (int count =0; count <5 ; count++) {
    	  Date now = new Date(System.currentTimeMillis());
    	  DateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS");
    	  publication = "Hello World " + df.format(now);
    	  message = new MqttMessage((publication + " message "+count).getBytes());
    	  qos = count % 3;
    	  message.setQos(qos);
    	  topic = client.getTopic(topics[qos]);
    	  System.out.println("Waiting for up to " + Example.sleepTimeout / 1000
    			  + " seconds for publication of \"" + message.toString()
    			  + "\" with QoS = " + message.getQos());
    	  System.out.println("On topic \"" + topic.getName()
    			  + "\" for client instance: \"" + client.getClientId()
    			  + "\" on address " + client.getServerURI() + "\"");
    	  MqttDeliveryToken token = topic.publish(message);
    	  token.waitForCompletion(Example.sleepTimeout);      
    	  System.out.println("Delivery token \"" + token.hashCode()
    			  + "\" has been received: " + token.isComplete());
      }
      client.disconnect();
      } catch (Exception e) {
      e.printStackTrace();
    }
  }
}
