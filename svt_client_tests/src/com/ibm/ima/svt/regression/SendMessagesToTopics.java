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
package com.ibm.ima.svt.regression;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;


import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.config.ImaConfig.TYPE;
import com.ibm.ima.test.cli.monitor.ImaMonitoring;
import com.ibm.ima.test.cli.monitor.SubscriptionStatResult;
import com.ibm.ims.svt.clients.JmsMessage;
import com.ibm.ims.svt.clients.MultiJmsConsumerClient;
import com.ibm.ims.svt.clients.MultiJmsProducerClient;
import com.ibm.ims.svt.clients.MultiMqttProducerClient;
import com.ibm.ims.svt.clients.MultiMqttSubscriberClient;
import com.ibm.ims.svt.clients.Trace;
import com.ibm.ima.test.cli.imaserver.Version;

/**
 * This class is my evil clients class. I have a variety of clients some which ack and commit messages
 * but some that deliberately don't.
 *  
 * Note: The clients DO NOT check any messages nor will they indicate if they have stopped etc. You need
 * to manually check that queues and topics are still processing messages. 
 * 
 * The intention of using this class is to create a constant load on the server whilst performing other messaging.
 *
 */
public class SendMessagesToTopics implements Runnable {

	private String port = null; // default but each class can use a different one
	private String ipaddress = null;
	private String prefix = "";

	private String topic1 = "TOPIC1";
	private String topic2 = "TOPIC2";
	private String topic3 = "TOPIC3";
	private String topic4 = "TOPIC4";
	private String topic5 = "TOPIC5";
	private String topic6 = "TOPIC6"; 
	private String topic7 = "TOPIC7";
	private String topic8 = "TOPIC8";
	private String topic9 = "TOPIC9";
	private String topic10 = "TOPIC10";
	private String topic11 = "TOPIC11";
	private String topic12 = "TOPIC12";
	
	private String endpoint = "SVTRegressionTopicEP";
	private String connectionPolicy = "SVTRegressionTopicCP";
	private String messagingPolicy = "SVTRegressionTopicMP";
	private String sharedMessagingPolicy = "SVTRegressionTopicMPShared";
	private String messagingHub = "SVTRegressionTopicHub";
	
	private ImaConfig config = null;
	private ImaMonitoring monitor = null;
	
	private ArrayList<String> topicList = new ArrayList<String>();
	private ArrayList<MultiJmsProducerClient> jmsProducerList = new ArrayList<MultiJmsProducerClient>();
	private ArrayList<MultiJmsConsumerClient> jmsConsumerList = new ArrayList<MultiJmsConsumerClient>();
	private ArrayList<MultiMqttProducerClient> mqttProducerList = new ArrayList<MultiMqttProducerClient>();
	private ArrayList<MultiMqttSubscriberClient> mqttConsumerList = new ArrayList<MultiMqttSubscriberClient>();
 	
	public boolean keepRunning = true;	
	private int pauseInterval = 1000;
	private boolean traceClients = false;
		
	
	public SendMessagesToTopics(String prefix, String ipaddress, String port, String cliaddress, boolean traceClients) throws Exception
	{
		this.ipaddress = ipaddress;
		this.port = port;
		this.prefix = prefix;
		config = new ImaConfig(cliaddress, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddress, "admin", "admin");
		monitor.connectToServer();
		this.traceClients = traceClients;
		this.setup();
	}
	
	public SendMessagesToTopics(String prefix, String ipaddress, String port, String cliaddressA, String cliaddressB, boolean traceClients) throws Exception
	{
		/*
		 * ipaddress may contain '+' as delimiter or ','. JMS api's use either <space> or comma for delimiter. Therefore,
		 * if '+' is used the convert to comma. This change is due to automation which cannot handle ',' as delimiter
		 */
		if(ipaddress.contains("+"))
		{	
			/*
			 * convert '+' to ','
			 */
			this.ipaddress = ipaddress.trim().replace('+',',');
		} else {
			/*
			 * Assuming ',' is being used.
			 */
			this.ipaddress = ipaddress;
		}
		this.port = port;
		this.prefix = prefix;
		config = new ImaConfig(cliaddressA, cliaddressB, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddressA, cliaddressB, "admin", "admin");
		monitor.connectToServer();
		this.traceClients = traceClients;
		this.setup();
	}
	
	private void setup() throws Exception
	{
		topic1=prefix+topic1;
		topic2=prefix+topic2;
		topic3=prefix+topic3;
		topic4=prefix+topic4;
		topic5=prefix+topic5;
		topic6=prefix+topic6;
		topic7=prefix+topic7;
		topic8=prefix+topic8;
		topic9=prefix+topic9;
		topic10=prefix+topic10;
		topic11=prefix+topic11;
		topic12=prefix+topic12;
		topicList.add(topic1);
		topicList.add(topic2);
		topicList.add(topic3);
		topicList.add(topic4);
		topicList.add(topic5);
		topicList.add(topic6);
		topicList.add(topic7);
		topicList.add(topic8);
		topicList.add(topic9);
		topicList.add(topic10);
		topicList.add(topic11);
		topicList.add(topic12);

		
		for(int i=0; i<topicList.size(); i++)
		{
			HashMap<String,SubscriptionStatResult> monitorResults = monitor.getSubStatisticsByTopic(topicList.get(i));
		
			if(!monitorResults.isEmpty())
			{
				Iterator<String> iter = monitorResults.keySet().iterator();
				while(iter.hasNext())
				{
					SubscriptionStatResult result = monitorResults.get(iter.next());
					String sub = result.getSubName();
					String client = result.getclientId();
					try
					{
						config.deleteSubscription(sub, client);
					}
					catch(Exception e)
					{
						// 	do nothing
					}
				}
			}
		}
		
		if(config.endpointExists(endpoint))
		{
			config.deleteEndpoint(endpoint);
		}
		if(config.messagingPolicyExists(messagingPolicy))
		{
			config.deleteMessagingPolicy(messagingPolicy);
		}
		if(config.messagingPolicyExists(sharedMessagingPolicy))
		{
			config.deleteMessagingPolicy(sharedMessagingPolicy);
		}
		if(config.connectionPolicyExists(connectionPolicy))
		{
			config.deleteConnectionPolicy(connectionPolicy);
		}
		if(config.messageHubExists(messagingHub))
		{
			config.deleteMessageHub(messagingHub);
		}
		
		if(! config.messageHubExists(messagingHub))
		{
			config.createHub(messagingHub, "Used by Hursley SVT");
		}
		if(! config.connectionPolicyExists(connectionPolicy))
		{
			config.createConnectionPolicy(connectionPolicy, connectionPolicy, TYPE.JMSMQTT);
		}
		if(! config.messagingPolicyExists(connectionPolicy))
		{
			config.createMessagingPolicy(messagingPolicy, messagingPolicy, "*", TYPE.Topic, TYPE.JMSMQTT);
			config.updateMessagingPolicy(messagingPolicy, "MaxMessages", "20000000");
		}
		
		/*
		 * Subscriptions only support JMS for 13a,b. Seems starting at 14a both MQTT and JMS are supported
		 */
		switch(Version.getVersion()) {
			case VER_100:
			case VER_110:
				/* 
				 * 13a and 13b only support JMS
				 */
				if(! config.messagingPolicyExists(sharedMessagingPolicy)) // need for durable shared subs (non durable use the topic messaging policy)
				{
					config.createMessagingPolicy(sharedMessagingPolicy, sharedMessagingPolicy, "*", TYPE.Subscription, TYPE.JMS);
					config.updateMessagingPolicy(sharedMessagingPolicy, "MaxMessages", "20000000");
					config.updateMessagingPolicy(sharedMessagingPolicy, "ActionList", "Receive,Control");
				}
	
				break;
			case VER_120:
			default:
				/*
				 * 14a and above should support both protocols
				 */
				if(! config.messagingPolicyExists(sharedMessagingPolicy)) // need for durable shared subs (non durable use the topic messaging policy)
				{
					config.createMessagingPolicy(sharedMessagingPolicy, sharedMessagingPolicy, "*", TYPE.Subscription, TYPE.JMSMQTT);
					config.updateMessagingPolicy(sharedMessagingPolicy, "MaxMessages", "20000000");
					config.updateMessagingPolicy(sharedMessagingPolicy, "ActionList", "Receive,Control");
				}
				break;
			
		}
		
		if(! config.endpointExists(endpoint))
		{
			config.createEndpoint(endpoint, endpoint, port, TYPE.JMSMQTT, connectionPolicy, messagingPolicy+","+sharedMessagingPolicy, messagingHub);
		}	
		
		try
		{
			config.disconnectServer();
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			monitor.disconnectServer();
		}
		catch(Exception e)
		{
			// do nothing
		}
	}
	
	private void tearDown() throws Exception
	{
		if(!config.isConnected())
		{
			config.connectToServer();
		}
		if(!monitor.isConnected())
		{
			monitor.connectToServer();
		}
		
	
		
		for(int i=0; i<topicList.size(); i++)
		{
			HashMap<String,SubscriptionStatResult> monitorResults = monitor.getSubStatisticsByTopic(topicList.get(i));
		
			if(!monitorResults.isEmpty())
			{
				Iterator<String> iter = monitorResults.keySet().iterator();
				while(iter.hasNext())
				{
					SubscriptionStatResult result = monitorResults.get(iter.next());
					String sub = result.getSubName();
					String client = result.getclientId();
					try
					{
						config.deleteSubscription(sub, client);
					}
					catch(Exception e)
					{
						// 	do nothing
					}
				}
			}
		}
		try
		{
			config.deleteEndpoint(endpoint);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(sharedMessagingPolicy);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteConnectionPolicy(connectionPolicy);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessageHub(messagingHub);
		}
		catch(Exception e)
		{
			// do nothing
		}
		
		try
		{
			config.disconnectServer();
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			monitor.disconnectServer();
		}
		catch(Exception e)
		{
			// do nothing
		}
	}

	public void run() {
		
		//instantiate trace object that uses system property
		Trace testtracer = new Trace();

		try
		{
			HashMap<String, String> properties = new HashMap<String, String>();
			for(int i=0; i<4; i++)
			{
				
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.CLIENT_ACKNOWLEDGE, 0);
				sub.setSelector("Food = 'Chips'");
				sub.setDurable();
				if(i%2 == 0)
				{	
					sub.disconnect(60000);				
				}
				sub.numberClients(2);
				if(traceClients)
				{
					sub.setTraceFile(topic1 + "_ASub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic1);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			for(int i=0; i<4; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.CLIENT_ACKNOWLEDGE, 4);
				sub.setSelector("Food = 'Steak'");
				sub.setDurable();
				if(i%2 == 0)
				{
					sub.unsubscribe(120000, false);
				}
				sub.numberClients(2);
				if(traceClients)
				{
					sub.setTraceFile(topic1 + "_BSub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic1);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + topic1);
			}
			properties = new HashMap<String, String>();
			properties.put("Food", "Chips");
			
			for(int i=0; i<2; i++)
			{
	
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.numberClients(4);
				prod.useSameConnection();
				prod.setHeaderProperties(properties);
				if(traceClients)
				{
					prod.setTraceFile(topic1 + "_AProd" + i);
				}
				prod.setup(null, null, topic1, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			properties = new HashMap<String, String>();
			properties.put("Food", "Steak");
			for(int i=0; i<2; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.numberClients(4);
				prod.useSameConnection();
				prod.setHeaderProperties(properties);
				if(traceClients)
				{
					prod.setTraceFile(topic1 + "_BProd" + i);
				}
				prod.setup(null, null, topic1, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();

			}
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + topic2);
			}
			for(int i=0; i<2; i++)
			{

				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.CLIENT_ACKNOWLEDGE, 0);
				sub.setDurable();
				sub.numberClients(2);
				if(traceClients)
				{
					sub.setTraceFile(topic2 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic2);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
				
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + topic3);
			}
			for(int i=0; i<2; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.numberClients(2);
				sub.setAckMode(javax.jms.Session.CLIENT_ACKNOWLEDGE, 4);
				sub.setDurable();
				if(traceClients)
				{
					sub.setTraceFile(topic3 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic3);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + topic2);
			}
			for(int i=0; i<2; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));				
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.numberClients(4);
				prod.useSameConnection();
				if(traceClients)
				{
					prod.setTraceFile(topic2 + "_Prod" + i);
				}
				prod.setup(null, null, topic2, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the producers for " + topic3);
			}
			for(int i=0; i<2; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));				
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.numberClients(4);
				prod.useSameConnection();
				if(traceClients)
				{
					prod.setTraceFile(topic3 + "_Prod" + i);
				}
				prod.setup(null, null, topic3, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();			
			}
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + topic4);
			}
			for(int i=0; i<2; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, 4);
				sub.numberClients(2);
				sub.setDurable();
				sub.setSelector("Food = 'Chips'");
				sub.setTransactedSession();
				if(traceClients)
				{
					sub.setTraceFile(topic4 + "_ASub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic4);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			for(int i=0; i<2; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, -1);
				sub.setDurable();
				sub.numberClients(2);
				sub.setSelector("Food = 'Steak'");
				if(traceClients)
				{
					sub.setTraceFile(topic4 + "_BSub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic4);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + topic4);
			}
			properties = new HashMap<String, String>();
			properties.put("Food", "Chips");
			for(int i=0; i<2; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));
				prod.sendPersistentMessages();
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.numberClients(4);
				prod.useSameConnection();
				prod.setHeaderProperties(properties);
				prod.setTransactedSession(1);
				if(traceClients)
				{
					prod.setTraceFile(topic4 + "_AProd" + i);
				}
				prod.setup(null, null, topic4, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			}
			properties = new HashMap<String, String>();
			properties.put("Food", "Steak");
			for(int i=0; i<2; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));
				prod.sendPersistentMessages();
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.numberClients(4);
				prod.useSameConnection();
				prod.setHeaderProperties(properties);
				prod.setTransactedSession(4);
				if(traceClients)
				{
					prod.setTraceFile(topic4 + "_BProd" + i);
				}
				prod.setup(null, null, topic4, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();				
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + topic5);
			}
			for(int i=0; i<2; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, -1);
				sub.setDurable();
				sub.numberClients(2);
				sub.setTransactedSession();
				if(traceClients)
				{
					sub.setTraceFile(topic5 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic5);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + topic6);
			}
			for(int i=0; i<2; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, 5);
				sub.setDurable();
				sub.numberClients(2);
				sub.setTransactedSession();
				if(i%2 == 0)
				{
					sub.unsubscribe(120000, true);
				}
				if(traceClients)
				{
					sub.setTraceFile(topic6 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic6);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + topic5);
			}
			for(int i=0; i<2; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));
				prod.sendPersistentMessages();
				prod.setMessageInterval(1000);
				prod.setNumberOfMessages(2147479999);
				prod.numberClients(4);
				prod.useSameConnection();
				prod.setTransactedSession(4);
				if(traceClients)
				{
					prod.setTraceFile(topic5 + "_Prod" + i);
				}
				prod.setup(null, null, topic5, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();			
			}
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + topic6);
			}
			for(int i=0; i<2; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));
				prod.sendPersistentMessages();
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.numberClients(4);
				prod.useSameConnection();
				prod.setTransactedSession(4);
				if(traceClients)
				{
					prod.setTraceFile(topic6 + "_Prod" + i);
				}
				prod.setup(null, null, topic6, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();				
			}
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + topic7);
			}
			for(int i=0; i<2; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, -1);
				sub.setSharedSubscription("SUB_TOPIC7", false);
				sub.numberClients(2);
				if(traceClients)
				{
					sub.setTraceFile(topic7 + "_ASub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic7);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			for(int i=0; i<5; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, 2);
				sub.numberClients(2);
				sub.setDurable();
				sub.setTransactedSession();
				if(i%2 == 0)
				{
					sub.disconnect(120000);
				}
				sub.setSharedSubscription("SUB_TOPIC7", false);
				if(traceClients)
				{
					sub.setTraceFile(topic7 + "_BSub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic7);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + topic7);
			}
			for(int i=0; i<4; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.numberClients(4);
				prod.useSameConnection();
				prod.setTransactedSession(4);
				if(traceClients)
				{
					prod.setTraceFile(topic7 + "_Prod" + i);
				}
				prod.setup(null, null, topic7, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();				
			}
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + topic8);
			}
			for(int i=0; i<4; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, 5);
				sub.setSharedSubscription("SUB_TOPIC8", false);
				sub.numberClients(2);
				sub.setTransactedSession();
				if(traceClients)
				{
					sub.setTraceFile(topic8 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic8);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + topic8);
			}
			for(int i=0; i<2; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));
				prod.sendPersistentMessages();
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.numberClients(4);
				prod.useSameConnection();
				prod.setTransactedSession(4);
				if(traceClients)
				{
					prod.setTraceFile(topic8 + "_Prod" + i);
				}
				prod.setup(null, null, topic8, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();				
			}
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + topic9);
			}
			
			String[] urls = null;
			if(ipaddress.contains(",") || ipaddress.contains("+"))
			{	
				/*
				 * split either on "," "|" or "+"
				 */
				String[] temp = ipaddress.trim().split(",|\\||\\+", -1);
				urls = new String[temp.length];
				for(int i=0; i<temp.length; i++)
				{
					urls[i] = "tcp://" + temp[i] + ":" + port;
				}
				
			}
			for(int i=0; i<2; i++)
			{
				MultiMqttSubscriberClient sub = null;
				if(urls!= null)
				{
					sub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					sub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				sub.setCleanSession(true);
				sub.numberClients(3);
				sub.setQos(0);
				if(traceClients)
				{
					sub.setTraceFile(topic9 + "_Sub" + i);
				}
				sub.setup("$SharedSubscription/" + prefix + "/"+ topic9);
				mqttConsumerList.add(sub);
				new Thread(sub).start();
			}
			
			for(int i=0; i<2; i++)
			{
				MultiMqttSubscriberClient sub = null;
				if(urls!= null)
				{
					sub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					sub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				sub.setCleanSession(false);
				sub.numberClients(3);
				sub.setQos(0);
				if(traceClients)
				{
					sub.setTraceFile(topic9 + "_Sub" + i);
				}
				sub.setup("$SharedSubscription/" + prefix + "/"+ topic9);
				mqttConsumerList.add(sub);
				new Thread(sub).start();
			}
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the producers for " + topic9);
			}
			for(int i=0; i<2; i++)
			{
				MultiMqttProducerClient prod = null;
				if(urls!= null)
				{
					prod = new MultiMqttProducerClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					prod = new MultiMqttProducerClient("tcp://" + ipaddress + ":" + port);
				}
				prod.setCleanSession(false);
				prod.setQos(1);
				prod.numberClients(3);
				if(traceClients)
				{
					prod.setTraceFile(topic9 + "_Prod" + i);
				}
				prod.setUpClient(topic9, null);
				mqttProducerList.add(prod);
				new Thread(prod).start();
			}
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + topic10);
			}
			for(int i=0; i<2; i++)
			{
				MultiMqttSubscriberClient sub = null;
				if(urls!= null)
				{
					sub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					sub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				sub.setCleanSession(false);
				sub.numberClients(3);
				sub.setQos(2);
				if(traceClients)
				{
					sub.setTraceFile(topic10 + "_Sub" + i);
				}
				sub.setup(topic10);
				mqttConsumerList.add(sub);
				new Thread(sub).start();
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the producers for " + topic10);
			}
			for(int i=0; i<2; i++)
			{
				MultiMqttProducerClient prod = null;
				if(urls!= null)
				{
					prod = new MultiMqttProducerClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					prod = new MultiMqttProducerClient("tcp://" + ipaddress + ":" + port);
				}
				prod.setCleanSession(true);
				prod.setQos(0);
				prod.numberClients(3);
				if(traceClients)
				{
					prod.setTraceFile(topic10 + "_Prod" + i);
				}
				prod.setUpClient(topic10, null);
				mqttProducerList.add(prod);
				new Thread(prod).start();
			}
			
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + topic11);
			}
			
			for(int i=0; i<5; i++)
			{
				MultiMqttSubscriberClient sub = null;
				if(urls!= null)
				{
					sub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					sub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				sub.setCleanSession(true);
				sub.numberClients(5);
				if(i%2 == 0)
				{
					sub.unsubscribe(120000, false);
				}
				sub.setQos(0);
				if(traceClients)
				{
					sub.setTraceFile(topic11 + "_Sub" + i);
				}
				sub.setup("$SharedSubscription/" +prefix + "Sub" + i + "/" + topic11);
				
				mqttConsumerList.add(sub);
				new Thread(sub).start();
			}
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the producers for " + topic11);
			}
			for(int i=0; i<10; i++)
			{
				MultiMqttProducerClient prod = null;
				if(urls!= null)
				{
					prod = new MultiMqttProducerClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					prod = new MultiMqttProducerClient("tcp://" + ipaddress + ":" + port);
				}
				prod.setCleanSession(false);
				prod.setQos(0);
				prod.sendSimpleTextMessage();
				prod.numberClients(6);
				if(traceClients)
				{
					prod.setTraceFile(topic11 + "_Prod" + i);
				}
				prod.setUpClient(topic11, null);
				mqttProducerList.add(prod);
				new Thread(prod).start();
			}
		
			
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + topic12);
			}
			
			for(int i=0; i<5; i++)
			{
				MultiMqttSubscriberClient sub = null;
				if(urls!= null)
				{
					sub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					sub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				sub.setCleanSession(true);
				sub.numberClients(5);
				if(i%2 == 0)
				{
					sub.unsubscribe(120000, false);
				}
				sub.setQos(0);
				if(traceClients)
				{
					sub.setTraceFile(topic12 + "_Sub" + i);
				}
				sub.setup("$SharedSubscription/" +prefix + "SubMixed" + i + "/" + topic12);
				
				mqttConsumerList.add(sub);
				new Thread(sub).start();
			}
			for(int i=0; i<5; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, -1);
				sub.setSharedSubscription(prefix + "SubMixed" + i, false);
				sub.numberClients(5);
				if(traceClients)
				{
					sub.setTraceFile(topic12 + "Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic12);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the producers for " + topic12);
			}
			for(int i=0; i<10; i++)
			{
				MultiMqttProducerClient prod = null;
				if(urls!= null)
				{
					prod = new MultiMqttProducerClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					prod = new MultiMqttProducerClient("tcp://" + ipaddress + ":" + port);
				}
				prod.setCleanSession(false);
				prod.setQos(0);
				prod.sendSimpleTextMessage();
				prod.numberClients(6);
				if(traceClients)
				{
					prod.setTraceFile(topic12 + "_Prod" + i);
				}
				prod.setUpClient(topic12, null);
				mqttProducerList.add(prod);
				new Thread(prod).start();
			}
			
			while(keepRunning)
			{
				Thread.sleep(360000);
			}
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("SendMessagesToAppliance Thread has been asked to stop so cleaning up all consumers and producers");
			}
			for(int i=0; i<jmsProducerList.size(); i++)
			{
				jmsProducerList.get(i).keepRunning = false;
			}
			for(int i=0; i<jmsConsumerList.size(); i++)
			{
				jmsConsumerList.get(i).keepRunning = false;
			}
			for(int i=0; i<mqttProducerList.size(); i++)
			{
				mqttProducerList.get(i).keepRunning = false;
			}
			for(int i=0; i<mqttConsumerList.size(); i++)
			{
				mqttConsumerList.get(i).keepRunning = false;
			}
			// sleep for 1 min before trying to delete everything
			Thread.sleep(60000);
			this.tearDown();
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("SendMessagesToAppliance STOPPED");
			}
			
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
	
	}
	
	

}
