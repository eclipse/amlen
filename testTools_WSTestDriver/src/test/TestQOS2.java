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
package test;

import java.io.File;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.concurrent.ConcurrentHashMap;

import com.ibm.ima.mqcon.resiliency.MqttProducers;
import com.ibm.ima.mqcon.resiliency.MqttSubscribers;
import com.ibm.ima.mqcon.utils.Audit;

public class TestQOS2 {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		
		try
		{
			
			ArrayList<MqttSubscribers> mqttSubscribersList = new ArrayList<MqttSubscribers>();
			ArrayList<MqttProducers> mqttProducersList = new ArrayList<MqttProducers>();
			ConcurrentHashMap<String, String> messagesSent = new ConcurrentHashMap<String, String>();
			ConcurrentHashMap<String, String> messagesReceived = new ConcurrentHashMap<String, String>();
			String server1 = "tcp://10.10.1.119:16102";
			String server2 = "tcp://10.10.1.85:16102";
			
			for(int i=0; i<5; i++)
			{
				MqttSubscribers temp = new MqttSubscribers(server1, server2, "StressT1", false, 2, messagesReceived, messagesSent, "StressT1Subscriber"+i, true );
				Thread t = new Thread(temp);
				mqttSubscribersList.add(temp);
				t.start();
			}
			Thread.sleep(5000); // give them time to setup
			
			for(int i=0; i<20; i++)
			{
				MqttProducers temp = new MqttProducers("StressT1Producer"+i, 2000, server1, server2, "StressT1", 2, false, messagesSent);
				Thread t = new Thread(temp);
				mqttProducersList.add(temp);
				t.start();
			}
			System.out.println("Set up complete");
			Thread.sleep(900000);
			System.out.println("Woken up and going to stop all the threads");
			
			File audit = new File("TestQOS2_results.txt");
			if(!audit.exists())
			{
				audit.createNewFile();
			}
			
			Audit trace = new Audit("TestQOS2_results.txt", true);
			
			for(int i=0; i<mqttProducersList.size(); i++)
			{
				mqttProducersList.get(i).keepRunning = false;
			}
			
			System.out.println("Sleeping to give the subscribers a chance to catch up");
			Thread.sleep(120000);
			
			for(int i=0; i<mqttSubscribersList.size(); i++)
			{
				mqttSubscribersList.get(i).keepRunning = false;
			}
			
			trace.trace("Sent Messages size = " + messagesSent.size());
			trace.trace("Received Messages size = " + messagesReceived.size());
			
			Enumeration<String> iter = messagesReceived.keys();
			
			while(iter.hasMoreElements())
			{
				String key = iter.nextElement();
				messagesSent.remove(key);
			}
			trace.trace("Sent messages not received = " + messagesSent.size());
			
			Enumeration<String> iter2 = messagesSent.keys();
			while(iter2.hasMoreElements())
			{
				String key = iter2.nextElement();
				trace.trace("Try and find message " + key + " " + messagesSent.get(key));
			}
			
			trace.trace("Checking for duplicates");
			for(int i=0; i<mqttSubscribersList.size(); i++)
			{
				if(! mqttSubscribersList.get(i).getDuplicates().isEmpty())
				{
					ArrayList<String> duplicates = mqttSubscribersList.get(i).getDuplicates();
					trace.trace(mqttSubscribersList.get(i).getName() + " had the following duplicates = " + duplicates.size());
					for(int j=0; j<duplicates.size(); j++)
					{
						trace.trace("Duplicate message= " + duplicates.get(j));
					}
				}
				else
				{
					trace.trace("No duplicates for "+ mqttSubscribersList.get(i).getName() );
					
				}
			}
			
			System.out.println("DONE");
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}

	}

}
