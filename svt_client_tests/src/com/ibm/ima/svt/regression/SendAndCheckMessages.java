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

import java.io.File;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.UUID;

import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.config.ImaConfig.TYPE;
import com.ibm.ima.test.cli.imaserver.Version;
import com.ibm.ima.test.cli.monitor.ImaMonitoring;
import com.ibm.ima.test.cli.monitor.SubscriptionStatResult;
import com.ibm.ims.svt.clients.JmsConsumerClient;
import com.ibm.ims.svt.clients.JmsMessage;
import com.ibm.ims.svt.clients.JmsProducerClient;
import com.ibm.ims.svt.clients.MqttProducerClient;
import com.ibm.ims.svt.clients.MqttSubscriberClient;
import com.ibm.ims.svt.clients.MultiJmsConsumerClient;
import com.ibm.ims.svt.clients.MultiMqttSubscriberClient;
import com.ibm.ims.svt.clients.Trace;


/**
 * This test creates a number of clients and checks that the message sent are
 * correctly received by the subscribers.
 *
 */
public class SendAndCheckMessages implements Runnable  {
	
	private String port = null;
	private String ipaddress = null;
	private String prefix = "";
	private String cliaddressA = null;
	private String cliaddressB = null;

	private String topic1 = "TOPIC1";
	private String topic2 = "TOPIC2";
	private String topic3 = "TOPIC3";
	private String topic4 = "TOPIC4";
	private String topic5 = "TOPIC5";
	private String topic6 = "TOPIC6";
	private String topic7 = "TOPIC7";
	private String topic8 = "TOPIC8";
	private String topic9 = "TOPIC9";
	
	
	private String queue1 = "QUEUE1";
	private String queue2 = "QUEUE2";
	
	private String endpoint = "SVTIntegrityTopicEP";
	private String messagingPolicyQ = "SVTIntegrityQueueMP";
	private String messagingPolicyT = "SVTIntegrityTopicMP";
	private String messagingPolicyS = "SVTIntegritySharedMP";
	private String messageHub = "SVTIntegrityTopicHub";
	private String connectionPolicy = "SVTIntegrityCP";
	
	private ImaConfig config = null;
	private ImaMonitoring monitor = null;
	
	private ArrayList<String> topicList = new ArrayList<String>();
	private ArrayList<String> queueList = new ArrayList<String>();
	private ArrayList<JmsProducerClient> jmsProducerList = new ArrayList<JmsProducerClient>();
	private ArrayList<JmsConsumerClient> jmsConsumerList = new ArrayList<JmsConsumerClient>();
	private ArrayList<MqttProducerClient> mqttProducerList = new ArrayList<MqttProducerClient>();
	private ArrayList<MqttSubscriberClient> mqttConsumerList = new ArrayList<MqttSubscriberClient>();
	private ArrayList<MultiMqttSubscriberClient> sharedMqttConsumerList = new ArrayList<MultiMqttSubscriberClient>();
	
	private int numberMessages = 10000000;
	private int messageInterval = 1000;
	//
	// checkMsgIntervalVal initially set to 240000. However, fails are seen due to load: 'FailMessage:Client NOT IN USE has been configured 
	// to check for messages every 240000milliseconds.The last message arrived at 1403550393518 but the time is 140350393568'
	// failure messages. 
	//
	// Increasing interval to 500s
	//
	private int checkMsgIntervalVal = 500000;

	private File directory;
	private boolean traceClients = false;
 	
	public boolean keepRunning = true;	
	
	
			
	public SendAndCheckMessages(String prefix, String ipaddress, String port, String cliaddress, boolean traceClients) throws Exception
	{
		this.ipaddress = ipaddress;
		this.port = port;
		this.prefix = prefix;
		this.traceClients = traceClients;
		this.cliaddressA = cliaddress;
		config = new ImaConfig(cliaddress, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddress, "admin", "admin");
		monitor.connectToServer();
		this.setup();

	}
	
	public SendAndCheckMessages(String prefix, String ipaddress, String port, String cliaddressA, String cliaddressB, boolean traceClients) throws Exception
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
			 * Assume ',' is being used
			 */
			this.ipaddress = ipaddress;
		}
		
		this.port = port;
		this.prefix = prefix;
		this.traceClients = traceClients;
		this.cliaddressA = cliaddressA;
		this.cliaddressB = cliaddressB;
		config = new ImaConfig(cliaddressA, cliaddressB, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddressA, cliaddressB, "admin", "admin");
		monitor.connectToServer();
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
		topicList.add(topic1);
		topicList.add(topic2);
		topicList.add(topic3);
		topicList.add(topic4);
		topicList.add(topic5);
		topicList.add(topic6);
		topicList.add(topic7);
		topicList.add(topic8);
		topicList.add(topic9);
		
		queue1=prefix+queue1;
		queue2=prefix+queue2;
		queueList.add(queue1);
		queueList.add(queue2);
		
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
		
		for(int i=0; i<queueList.size(); i++)
		{
			if(config.queueExists(queueList.get(i)))
			{
				config.deleteQueue(queueList.get(i), true);
			}
			config.createQueue(queueList.get(i), "SVT Regression checkMessages queue", 20000000);
		}
		
		if(config.endpointExists(endpoint))
		{
			config.deleteEndpoint(endpoint);
		}
		if(config.messagingPolicyExists(messagingPolicyT))
		{
			config.deleteMessagingPolicy(messagingPolicyT);
		}
		if(config.messagingPolicyExists(messagingPolicyQ))
		{
			config.deleteMessagingPolicy(messagingPolicyQ);;
		}
		if(config.messagingPolicyExists(messagingPolicyS))
		{
			config.deleteMessagingPolicy(messagingPolicyS);;
		}
		if(config.connectionPolicyExists(connectionPolicy))
		{
			config.deleteConnectionPolicy(connectionPolicy);
		}
		if(config.messageHubExists(messageHub))
		{
			config.deleteMessageHub(messageHub);
		}
		
		if(! config.messageHubExists(messageHub))
		{
			config.createHub(messageHub, "Used by Hursley SVT");
		}
		if(! config.connectionPolicyExists(connectionPolicy))
		{
			config.createConnectionPolicy(connectionPolicy, connectionPolicy, TYPE.JMSMQTT);
		}
		if(! config.messagingPolicyExists(messagingPolicyQ))
		{
			config.createMessagingPolicy(messagingPolicyQ, messagingPolicyQ, "*", TYPE.Queue, TYPE.JMS);
		}
		
		/*
		 * Subscriptions only support JMS for 13a,b. Seems starting at 14a both MQTT and JMS are supported
		 */
		switch(Version.getVersion()) {
			case VER_100:
			case VER_110:
				/* 
				 * 1.0 and 1.1 only support  JMS
				 */
				 
				if(! config.messagingPolicyExists(messagingPolicyS))
				{
					config.createMessagingPolicy(messagingPolicyS, messagingPolicyS, "*", TYPE.Subscription, TYPE.JMS);  // used for durable shared (non durable use topics)
					config.updateMessagingPolicy(messagingPolicyS, "MaxMessages", "20000000");
					config.updateMessagingPolicy(messagingPolicyS, "ActionList", "Receive,Control");
				}
				break;
			case VER_120:
			case VER_130:
			default:
				/*
				 * 2.0 and above should support both JMS and MQTT
				 */
				if(! config.messagingPolicyExists(messagingPolicyS))
				{
					config.createMessagingPolicy(messagingPolicyS, messagingPolicyS, "*", TYPE.Subscription, TYPE.JMSMQTT);  // used for durable shared (non durable use topics)
					config.updateMessagingPolicy(messagingPolicyS, "MaxMessages", "20000000");
					config.updateMessagingPolicy(messagingPolicyS, "ActionList", "Receive,Control");
				}
				break;
		}
		
		
		if(! config.messagingPolicyExists(messagingPolicyT))
		{
			config.createMessagingPolicy(messagingPolicyT, messagingPolicyT, "*", TYPE.Topic, TYPE.JMSMQTT);
			config.updateMessagingPolicy(messagingPolicyT, "MaxMessages", "20000000");
		}
		if(! config.endpointExists(endpoint))
		{
			config.createEndpoint(endpoint, endpoint, port, TYPE.JMSMQTT, connectionPolicy, messagingPolicyT+ "," + messagingPolicyQ + "," + messagingPolicyS, messageHub);
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
		
	
		for(int i=0; i<queueList.size(); i++)
		{
			if(config.queueExists(queueList.get(i)))
			{
				config.deleteQueue(queueList.get(i), true);
			}
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
			config.deleteMessagingPolicy(messagingPolicyT);
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
			config.deleteMessagingPolicy(messagingPolicyQ);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicyS);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessageHub(messageHub);
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
		
		// instantiate trace that uses systemproperty MyTraceFile MyTrace
		Trace testtracer = new Trace();
		directory= new File("./errors");
		if (directory.mkdir()) {
			//created directory. note creation
			System.out.println("Created ./errors directory\n");
		} else {
			if(directory.exists()) {
				//directory exits
				System.out.println("Directory ./errors already exists\n");
			} else {
				//note failure to create
				System.out.println("Unable to create ./errors directory\n");
			}
		}
		
		try
		{
			System.out.println("STARTING SETUP");
			HashMap<String, String> properties = new HashMap<String, String>();
			ArrayList<String> clientIDs = new ArrayList<String>();
			for(int i=0; i<5; i++)
			{
				String clientID = null;
				do{
					clientID = UUID.randomUUID().toString().substring(0, 15);
				}
				while(clientIDs.contains(clientID));
				clientIDs.add(clientID);
			}
			System.out.println("Setting up first subscribers");
			for(int i=0; i<2; i++)
			{
				
				JmsConsumerClient sub = new JmsConsumerClient(ipaddress, new Integer(port));
				sub.setSelector("Food = 'Chips'");
				sub.setDurable();
				sub.checkMsgInterval(this.checkMsgIntervalVal, cliaddressA, cliaddressB);
				sub.setRegressionListener(clientIDs);
				if(traceClients)
				{
					sub.setTraceFile(topic1 + "_Sub" + i);
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
			
			for(int i=0; i<5; i++)
			{
	
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setTransactedSession(1);
				prod.setClientID(clientIDs.get(i));
				if(traceClients)
				{
					prod.setTraceFile(topic1 + "_Prod" + i);
				}
				prod.setHeaderProperties(properties);
				prod.setup(null, null, topic1, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			clientIDs = new ArrayList<String>();
			for(int i=0; i<5; i++)
			{
				String clientID = null;
				do{
					clientID = UUID.randomUUID().toString().substring(0, 15);
				}
				while(clientIDs.contains(clientID));
				clientIDs.add(clientID);
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + topic2);
			}
			for(int i=0; i<2; i++)
			{

				JmsConsumerClient sub = new JmsConsumerClient(ipaddress, new Integer(port));
				sub.setRegressionListener(clientIDs);
				sub.checkMsgInterval(this.checkMsgIntervalVal, cliaddressA, cliaddressB);
				if(traceClients)
				{
					sub.setTraceFile(topic2 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic2);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
				
			}
			
			
		
			for(int i=0; i<5; i++)
			{
	
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setClientID(clientIDs.get(i));
				prod.setTransactedSession(1);
				if(traceClients)
				{
					prod.setTraceFile(topic2 + "_Prod" + i);
				}
				prod.setHeaderProperties(properties);
				prod.setup(null, null, topic2, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			clientIDs = new ArrayList<String>();
			for(int i=0; i<2; i++)
			{
				String clientID = null;
				do{
					clientID = UUID.randomUUID().toString().substring(0, 15);
				}
				while(clientIDs.contains(clientID));
				clientIDs.add(clientID);
			}
			
			JmsConsumerClient sub = new JmsConsumerClient(ipaddress, new Integer(port));
			sub.setSelector("Food = 'Chips'");
			sub.setRegressionListener(clientIDs);
			if(traceClients)
			{
				sub.setTraceFile(queue1 + "_SubA");
			}
			sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, queue1);
			jmsConsumerList.add(sub);
			new Thread(sub).start();
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating the first producers for " + queue1);
			}
			properties = new HashMap<String, String>();
			properties.put("Food", "Chips");
			
			for(int i=0; i<2; i++) 
			{
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setClientID(clientIDs.get(i));
				prod.setTransactedSession(1);
				if(traceClients)
				{
					prod.setTraceFile(queue1 + "_ProdA" + i);
				}
				prod.setHeaderProperties(properties);
				prod.setup(null, null, queue1, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			}
			
			clientIDs = new ArrayList<String>();
			for(int i=0; i<2; i++)
			{
				String clientID = null;
				do{
					clientID = UUID.randomUUID().toString().substring(0, 15);
				}
				while(clientIDs.contains(clientID));
				clientIDs.add(clientID);
			}
			sub = new JmsConsumerClient(ipaddress, new Integer(port));
			sub.setSelector("Food = 'Steak'");
			sub.setRegressionListener(clientIDs);
			if(traceClients)
			{
				sub.setTraceFile(queue1 + "_SubB");
			}
			sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, queue1);
			jmsConsumerList.add(sub);
			new Thread(sub).start();
			
			
			properties = new HashMap<String, String>();
			properties.put("Food", "Steak");
			for(int i=0; i<2; i++) 
			{
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setClientID(clientIDs.get(i));
				prod.setTransactedSession(1);
				if(traceClients)
				{
					prod.setTraceFile(queue1 + "_ProdB" + i);
				}
				prod.setHeaderProperties(properties);
				prod.setup(null, null, queue1, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			}
			
			
			clientIDs = new ArrayList<String>();
			for(int i=0; i<3; i++)
			{
				String clientID = null;
				do{
					clientID = UUID.randomUUID().toString().substring(0, 15);
				}
				while(clientIDs.contains(clientID));
				clientIDs.add(clientID);
			}
			
			sub = new JmsConsumerClient(ipaddress, new Integer(port));
			sub.setRegressionListener(clientIDs);
			if(traceClients)
			{
				sub.setTraceFile(queue2 + "_Sub");
			}
			sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, queue2);
			jmsConsumerList.add(sub);
			new Thread(sub).start();
						
			for(int i=0; i<3; i++) 
			{
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setClientID(clientIDs.get(i));
				prod.setTransactedSession(1);
				if(traceClients)
				{
					prod.setTraceFile(queue2 + "_Prod" + i);
				}
				prod.setHeaderProperties(properties);
				prod.setup(null, null, queue2, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			}
			String[] urls = null;
			if(ipaddress.contains(","))
			{
				String[] temp = ipaddress.trim().split(",", -1);
				urls = new String[temp.length];
				for(int i=0; i<temp.length; i++)
				{
					urls[i] = "tcp://" + temp[i] + ":" + port;
				}
				
			}
			
			clientIDs = new ArrayList<String>();
			for(int i=0; i<3; i++)
			{
				String clientID = null;
				do{
					clientID = UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
				}
				while(clientIDs.contains(clientID));
				clientIDs.add(clientID);
			}
			
			for(int i=0; i<3; i++)
			{
				MqttSubscriberClient submqtt = null;
				if(urls!= null)
				{
					submqtt = new MqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					submqtt = new MqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				submqtt.setCleanSession(true);
				submqtt.setQos(2);
				submqtt.setRegressionListener(clientIDs);
				if(traceClients)
				{
					submqtt.setTraceFile(topic3 + "_Sub" + i);
				}
				submqtt.setup(topic3);
				mqttConsumerList.add(submqtt);
				new Thread(submqtt).start();
			}
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the producers for " + topic3);
			}
			for(int i=0; i<3; i++)
			{
				MqttProducerClient prod = null;
				if(urls!= null)
				{
					prod = new MqttProducerClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					prod = new MqttProducerClient("tcp://" + ipaddress + ":" + port);
				}
				prod.setCleanSession(false);
				prod.setQos(2);
				prod.setClientID(clientIDs.get(i));
				prod.messageInterval(messageInterval);
				if(traceClients)
				{
					prod.setTraceFile(topic3 + "_Prod" + i);
				}
				prod.setUpClient(topic3);
				mqttProducerList.add(prod);
				new Thread(prod).start();
			}
			
			
			clientIDs = new ArrayList<String>();
			for(int i=0; i<3; i++)
			{
				String clientID = null;
				do{
					clientID = UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
				}
				while(clientIDs.contains(clientID));
				clientIDs.add(clientID);
			}
			
			for(int i=0; i<3; i++)
			{
				MqttSubscriberClient submqtt = null;
				if(urls!= null)
				{
					submqtt = new MqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					submqtt = new MqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				submqtt.setCleanSession(false);
				submqtt.setRegressionListener(clientIDs);
				if(traceClients)
				{
					submqtt.setTraceFile(topic4 + "_Sub" + i);
				}
				submqtt.setQos(2);
				submqtt.setup(topic4);
				mqttConsumerList.add(submqtt);
				new Thread(submqtt).start();
			}
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the producers for " + topic4);
			}
			for(int i=0; i<3; i++)
			{
				MqttProducerClient prod = null;
				if(urls!= null)
				{
					prod = new MqttProducerClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					prod = new MqttProducerClient("tcp://" + ipaddress + ":" + port);
				}
				prod.setCleanSession(false);
				prod.setQos(2);
				prod.setClientID(clientIDs.get(i));
				if(traceClients)
				{
					prod.setTraceFile(topic4 + "_Prod" + i);
				}
				prod.setCleanSession(false);
				prod.messageInterval(messageInterval);
				prod.setUpClient(topic4);
				mqttProducerList.add(prod);
				new Thread(prod).start();
			}
			
			clientIDs = new ArrayList<String>();
			for(int i=0; i<3; i++)
			{
				String clientID = null;
				do{
					clientID = UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
				}
				while(clientIDs.contains(clientID));
				clientIDs.add(clientID);
			}
			for(int i=0; i<2; i++)
			{

				JmsConsumerClient subjms = new JmsConsumerClient(ipaddress, new Integer(port));
				subjms.setRegressionListener(clientIDs);
				subjms.setDurable();
				if(traceClients)
				{
					subjms.setTraceFile(topic5 + "_Sub" + i);
				}
				subjms.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic5);
				jmsConsumerList.add(subjms);
				new Thread(subjms).start();
				
			}
			

			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the producers for " + topic5);
			}
			for(int i=0; i<3; i++)
			{
				MqttProducerClient prod = null;
				if(urls!= null)
				{
					prod = new MqttProducerClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					prod = new MqttProducerClient("tcp://" + ipaddress + ":" + port);
				}
				prod.setCleanSession(false);
				prod.setClientID(clientIDs.get(i));
				prod.messageInterval(messageInterval);
				prod.setQos(2);
				if(traceClients)
				{
					prod.setTraceFile(topic5 + "_Prod" + i);
				}
				prod.setUpClient(topic5);
				mqttProducerList.add(prod);
				new Thread(prod).start();
			}
			
			clientIDs = new ArrayList<String>();
			for(int i=0; i<3; i++)
			{
				String clientID = null;
				do{
					clientID = UUID.randomUUID().toString().substring(0, 15);
				}
				while(clientIDs.contains(clientID));
				clientIDs.add(clientID);
			}
			for(int i=0; i<3; i++)
			{
				MqttSubscriberClient submqtt = null;
				if(urls!= null)
				{
					submqtt = new MqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					submqtt = new MqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				submqtt.setCleanSession(false);
				submqtt.setRegressionListener(clientIDs);
				submqtt.setQos(2);
				if(traceClients)
				{
					submqtt.setTraceFile(topic6 + "_Sub" + i);
				}
				submqtt.setup(topic6);
				mqttConsumerList.add(submqtt);
				new Thread(submqtt).start();
			}
			
			for(int i=0; i<3; i++)
			{
	
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setClientID(clientIDs.get(i));
				prod.setTransactedSession(1);
				if(traceClients)
				{
					prod.setTraceFile(topic6 + "_Prod" + i);
				}
				prod.setHeaderProperties(properties);
				prod.setup(null, null, topic6, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			for(int i=0; i<3; i++)
			{
				MultiMqttSubscriberClient sharedSub = null;
				if(urls!= null)
				{
					sharedSub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					sharedSub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				sharedSub.setCleanSession(true);
				sharedSub.numberClients(5);
				sharedSub.setQos(0);
				sharedSub.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB);
				sharedSub.setMaxMessagesToKeep(1000, 200);
				if(traceClients)
				{
					sharedSub.setTraceFile(topic7 + "_SubA" + i);
				}
				sharedSub.setup("$SharedSubscription/" +prefix + "Sub" + i + "/" + topic7);
				sharedMqttConsumerList.add(sharedSub);
				new Thread(sharedSub).start();
			}
			for(int i=0; i<2; i++)
			{
				MultiMqttSubscriberClient sharedSub = null;
				if(urls!= null)
				{
					sharedSub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					sharedSub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				sharedSub.setCleanSession(false);
				sharedSub.numberClients(5);
				sharedSub.setQos(0);
				sharedSub.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB);
				sharedSub.setMaxMessagesToKeep(1000, 200);
				if(traceClients)
				{
					sharedSub.setTraceFile(topic7 + "_SubB" + i);
				}
				sharedSub.setup("$SharedSubscription/" +prefix + "Sub" + i + "/" + topic7);
				sharedMqttConsumerList.add(sharedSub);
				new Thread(sharedSub).start();
			}

			
			properties = new HashMap<String, String>();
			properties.put("Food", "Chips");
			for(int i=0; i<5; i++)
			{
	
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setTransactedSession(1);
				if(traceClients)
				{
					prod.setTraceFile(topic7 + "_ProdA" + i);
				}
				prod.setHeaderProperties(properties);
				prod.setup(null, null, topic7, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			for(int i=0; i<5; i++)
			{
	
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setTransactedSession(1);
				if(traceClients)
				{
					prod.setTraceFile(topic7 + "_ProdB" + i);
				}
				prod.setHeaderProperties(properties);
				prod.setup(null, null, topic7, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			
			for(int i=0; i<4; i++)
			{
				
				MultiJmsConsumerClient sharedSub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sharedSub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, 0);
				sharedSub.setSelector("Food = 'Chips'");
				sharedSub.setDurable();
				if(i%2 == 0)
				{	
					sharedSub.disconnect(60000);				
				}
				sharedSub.numberClients(2);
				sharedSub.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB);
				sharedSub.setSharedSubscription(prefix+"JmsSub", true);
				if(traceClients)
				{
					sharedSub.setTraceFile(topic8 + "_SubA" + i);
				}
				sharedSub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic8);
				jmsConsumerList.add(sharedSub);
				new Thread(sharedSub).start();
			}			
			for(int i=0; i<4; i++)
			{
				
				MultiJmsConsumerClient sharedSub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sharedSub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, 30);
				sharedSub.setSelector("Food = 'Chips'");
				if(i%2 == 0)
				{	
					sharedSub.disconnect(60000);				
				}
				sharedSub.numberClients(2);
				sharedSub.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB);
				sharedSub.setSharedSubscription(prefix+"JmsSub", true);
				if(traceClients)
				{
					sharedSub.setTraceFile(topic8 + "_SubB" + i);
				}
				sharedSub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, topic8);
				jmsConsumerList.add(sharedSub);
				new Thread(sharedSub).start();
			}
			
			properties = new HashMap<String, String>();
			properties.put("Food", "Chips");
			for(int i=0; i<5; i++)
			{
	
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setTransactedSession(1);
				if(traceClients)
				{
					prod.setTraceFile(topic8 + "_ProdA" + i);
				}
				prod.setHeaderProperties(properties);
				prod.setup(null, null, topic8, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			for(int i=0; i<5; i++)
			{
	
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setTransactedSession(1);
				if(traceClients)
				{
					prod.setTraceFile(topic8 + "_ProdB" + i);
				}
				prod.setHeaderProperties(properties);
				prod.setup(null, null, topic8, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			
			for(int i=0; i<2; i++)
			{
				MultiMqttSubscriberClient sharedSub = null;
				if(urls!= null)
				{
					sharedSub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					sharedSub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				sharedSub.setCleanSession(true);
				sharedSub.numberClients(5);
				sharedSub.setQos(0);
				sharedSub.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB);
				sharedSub.setMaxMessagesToKeep(1000, 200);
				if(traceClients)
				{
					sharedSub.setTraceFile(topic9 + "_SubA" + i);
				}
				sharedSub.setup("$SharedSubscription/" +prefix + "Sub"+topic9 + i + "/" + topic9);
				sharedMqttConsumerList.add(sharedSub);
				new Thread(sharedSub).start();
			}
			
			for(int i=0; i<2; i++)
			{
				MultiMqttSubscriberClient sharedSub = null;
				if(urls!= null)
				{
					sharedSub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					sharedSub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				sharedSub.setCleanSession(true);
				sharedSub.numberClients(5);
				sharedSub.setQos(1);
				sharedSub.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB);
				sharedSub.setMaxMessagesToKeep(1000, 200);
				if(traceClients)
				{
					sharedSub.setTraceFile(topic9 + "_SubB" + i);
				}
				sharedSub.setup("$SharedSubscription/" +prefix + "Sub"+topic9 + i + "/" + topic9);
				sharedMqttConsumerList.add(sharedSub);
				new Thread(sharedSub).start();
			}
			
			for(int i=0; i<2; i++)
			{
				MultiMqttSubscriberClient sharedSub = null;
				if(urls!= null)
				{
					sharedSub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					sharedSub = new MultiMqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				sharedSub.setCleanSession(false);
				sharedSub.numberClients(5);
				sharedSub.setQos(2);
				sharedSub.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB);
				sharedSub.setMaxMessagesToKeep(1000, 200);
				if(traceClients)
				{
					sharedSub.setTraceFile(topic9 + "_SubC" + i);
				}
				sharedSub.setup("$SharedSubscription/" +prefix + "Sub"+topic9 + i + "/" + topic9);
				sharedMqttConsumerList.add(sharedSub);
				new Thread(sharedSub).start();
			}
			
			
			for(int i=0; i<10; i++)
			{
	
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setTransactedSession(1);
				if(traceClients)
				{
					prod.setTraceFile(topic9 + "_Prod" + i);
				}
				prod.setHeaderProperties(properties);
				prod.setup(null, null, topic9, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			
			File audit = new File("./errors/SendAndCheckMessages.txt");
			PrintStream writer = null;
			
			while(keepRunning)
			{
				Thread.sleep(180000);
				
				// now check we don't have a problem with any of the messages
				for(int i=0; i<jmsConsumerList.size(); i++)
				{
					if(! jmsConsumerList.get(i).getFailMessage().equals(""))
					{				
						if(!audit.exists())
						{
							audit.createNewFile();
						}
						if(writer == null)
						{
							writer = new PrintStream("./errors/SendAndCheckMessages.txt", "UTF-8");
						}
						writer.println(jmsConsumerList.get(i).getFailMessage()+ "\n");
						writer.flush();
					}
							
				}
				for(int i=0; i<mqttConsumerList.size(); i++)
				{
					if(! mqttConsumerList.get(i).getFailMessage().equals(""))
					{
						if(!audit.exists())
						{
							audit.createNewFile();
						}
						if(writer == null)
						{
							writer = new PrintStream("./errors/SendAndCheckMessages.txt", "UTF-8");
						}
						writer.println( mqttConsumerList.get(i).getFailMessage()+ "\n");
						writer.flush();
					}
							
				}
				for(int i=0; i<sharedMqttConsumerList.size(); i++)
				{
					if(! sharedMqttConsumerList.get(i).getFailMessage().equals(""))
					{
						if(!audit.exists())
						{
							audit.createNewFile();
						}
						if(writer == null)
						{
							writer = new PrintStream("./errors/SendAndCheckMessages.txt", "UTF-8");
						}
						writer.println(sharedMqttConsumerList.get(i).getFailMessage()+ "\n");
						writer.flush();
					}
							
				}
				if(writer != null)
				{
					keepRunning = false;
					try
					{
						writer.close();
					}
					catch(Exception e)
					{
						// do nothing
					}
					Thread.sleep(60000); // Want to see what other messages we get after this so keep going for 1 minute to help debugging
				}
				else
				{
					System.out.println("All messages are accounted for");
					File[] contents = directory.listFiles();
					System.out.println("Checking to see if anyone else has registered errors");
					if (contents.length == 0) {
						// do nothing
						System.out.println("No errors found - continuing");
					}
					else {
						
						System.out.println("Another test reported errors so quiescing and then stopping the clients");
						keepRunning = false;
						break;
					}
				}
				
				
				
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
			for(int i=0; i<sharedMqttConsumerList.size(); i++)
			{
				sharedMqttConsumerList.get(i).keepRunning = false;
			}
			
			// sleep for 1 min before trying to delete everything
			Thread.sleep(60000);
			if(writer != null)
			{
				this.tearDown(); 
			}
			else
			{
				//  lets leave everything up for debugging
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Send and check messages STOPPED");
			}
			
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
	
	}
	

}
