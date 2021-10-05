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
import com.ibm.ima.test.cli.config.ImaMQConnectivity;
import com.ibm.ima.test.cli.config.ImaConfig.TYPE;
import com.ibm.ima.test.cli.monitor.ImaMonitoring;
import com.ibm.ima.test.cli.monitor.SubscriptionStatResult;
import com.ibm.ima.test.cli.mq.MQConfig;
import com.ibm.ims.svt.clients.JmsConsumerClient;
import com.ibm.ims.svt.clients.JmsMessage;
import com.ibm.ims.svt.clients.JmsProducerClient;
import com.ibm.ims.svt.clients.MqttProducerClient;
import com.ibm.ims.svt.clients.MqttSubscriberClient;
import com.ibm.ims.svt.clients.Trace;

public class SendAndCheckMQMessages implements Runnable  {
	
	
	private String port = null; // default but each class can use a different one
	private String ipaddress = null;
	private String prefix = "";

	private String mappingRule1 ="IntmappingRule1";
	private String subtree1 = "MQREG1";
	private String imatopic1 = "MQREG1/TOPIC1";
	private String imatopic2 = "MQREG1/TOPIC2";
	private String imatopic3 = "MQREG1/TOPIC3";
	private String mqtopic123 = "MQREG/TOPIC123";
	
	private String mappingRule2 ="IntmappingRule2";
	private String imatopicA = "MQREG/TOPICA";
	private String mqqueueA = "MQREG/QUEUEA";
	
	private String mappingRule3 ="IntmappingRule3";
	private String imaqueueA = "MQREG/QUEUEA";
	private String mqtopicA = "MQREG/TOPICA";
	
	private String mappingRule4 ="IntmappingRule4";
	private String mqtopicB = "MQREG/TOPICB";
	private String imaqueueB = "MQREG/QUEUEB";
	
	private String mappingRule5 ="IntmappingRule5";
	private String mqqueueB = "MQREG/QUEUEB";
	private String imatopicB = "MQREG/TOPICB";
	
	private String subtree2 = "MQREG2";
	private String mappingRule6 ="IntmappingRule6";
	private String mqtopic1 = "MQREG2/TOPIC1";
	private String mqtopic2 = "MQREG2/TOPIC2";
	private String mqtopic3 = "MQREG2/TOPIC3";
	private String imatopic123 = "MQREG/TOPIC123";
	
	private String messageHub = "SVTIntegrityMQHub";
	private String connectionPolicy = "SVTIntegrityMQCP";
	private String messagingPolicyQ = "SVTIntegrityMQQMP";
	private String messagingPolicyT = "SVTIntegrityMQTMP";
	private String endpoint = "SVTIntegrityMQEP";
	
	private ImaConfig config = null;
	private ImaMonitoring monitor = null;
	private MQConfig mqconfig = null;
	private ImaMQConnectivity imamqconfig = null;
	
	private ArrayList<String> imatopicList = new ArrayList<String>();
	private ArrayList<String> mqtopicList = new ArrayList<String>();
	
	private ArrayList<String> imaqueueList = new ArrayList<String>();
	private ArrayList<String> mqqueueList = new ArrayList<String>();
	
	private ArrayList<JmsProducerClient> jmsProducerList = new ArrayList<JmsProducerClient>();
	private ArrayList<JmsConsumerClient> jmsConsumerList = new ArrayList<JmsConsumerClient>();
	private ArrayList<MqttProducerClient> mqttProducerList = new ArrayList<MqttProducerClient>();
	private ArrayList<MqttSubscriberClient> mqttConsumerList = new ArrayList<MqttSubscriberClient>();
	
	private String mqipaddress = null;
	private int mqjmsport = -1;
	private int mqmqttport = -1;
	private String mquser = null; // This needs to be a user that is not in the mqm group

	private static String qmgr = "INTREG";
	private static String listenerName = "IMA_LISTENER";
	private static String channelName = "IMA_MQCON";
	private static String clientChannelName = "IMS_CLIENTS";
	
	private File directory = new File("./errors");
	
	
	private int numberMessages = 10000000;
	private int messageInterval = 3000;
	
	private boolean traceClients = false;

 	
	public boolean keepRunning = true;	

	public SendAndCheckMQMessages(String prefix, String ipaddress, String port, String cliaddress, String mqipaddress, int mqjmsport, int mqmqttport, String mqlocation, String mqusername, String mquser, String mqpassword, boolean traceClients) throws Exception
	{
		this.ipaddress = ipaddress;
		this.port = port;
		this.prefix = prefix;
		this.mqipaddress = mqipaddress;
		this.mqjmsport = mqjmsport;
		this.mqmqttport = mqmqttport;
		this.mquser = mquser;
		this.traceClients = traceClients;
		config = new ImaConfig(cliaddress, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddress, "admin", "admin");
		monitor.connectToServer();
		mqconfig = new MQConfig(mqipaddress, mqusername, mqpassword, mqlocation);
		mqconfig.connectToServer();
		imamqconfig = new ImaMQConnectivity(cliaddress, "admin", "admin");
		imamqconfig.connectToServer();
		this.setup();
		
	}
	
	public SendAndCheckMQMessages(String prefix, String ipaddress, String port, String cliaddressA, String cliaddressB, String mqipaddress, int mqjmsport, int mqmqttport, String mqlocation, String mqusername, String mquser, String mqpassword, boolean traceClients) throws Exception
	{
		this.ipaddress = ipaddress;
		this.port = port;
		this.prefix = prefix;
		this.mqipaddress = mqipaddress;
		this.mqjmsport = mqjmsport;
		this.mqmqttport = mqmqttport;
		this.mquser = mquser;
		this.traceClients = traceClients;
		config = new ImaConfig(cliaddressA,cliaddressB, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddressA,cliaddressB, "admin", "admin");
		monitor.connectToServer();
		mqconfig = new MQConfig(mqipaddress, mqusername, mqpassword, mqlocation);
		mqconfig.connectToServer();
		imamqconfig = new ImaMQConnectivity(cliaddressA,cliaddressB, "admin", "admin");
		imamqconfig.connectToServer();
		this.setup();
	}
		
	public SendAndCheckMQMessages(String prefix, String ipaddress, String port, String cliaddress, boolean traceClients) throws Exception
	{
		this.ipaddress = ipaddress;
		this.port = port;
		this.prefix = prefix;
		this.traceClients = traceClients;
		config = new ImaConfig(cliaddress, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddress, "admin", "admin");
		monitor.connectToServer();
		this.setup();
	}
	
	public SendAndCheckMQMessages(String prefix, String ipaddress, String port, String cliaddressA, String cliaddressB, boolean traceClients) throws Exception
	{
		this.ipaddress = ipaddress;
		this.port = port;
		this.prefix = prefix;
		this.traceClients = traceClients;
		config = new ImaConfig(cliaddressA, cliaddressB, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddressA, cliaddressB, "admin", "admin");
		monitor.connectToServer();
		this.setup();
	}
	
	
	private void setup() throws Exception
	{
		subtree1=prefix+subtree1;
		imatopic1=prefix+imatopic1;
		imatopic2=prefix+imatopic2;
		imatopic3=prefix+imatopic3;
		mqtopic123=prefix+mqtopic123;
		imatopicA=prefix+imatopicA;
		mqqueueA=prefix+mqqueueA;
		imaqueueA=prefix+imaqueueA;
		mqtopicA=prefix+mqtopicA;
		mqtopicB=prefix+mqtopicB;
		imaqueueB=prefix+imaqueueB;
		mqqueueB=prefix+mqqueueB;
		imatopicB=prefix+imatopicB;
		subtree2=prefix+subtree2;
		mqtopic1=prefix+mqtopic1;
		mqtopic2=prefix+mqtopic2;
		mqtopic3=prefix+mqtopic3;
		imatopic123=prefix+imatopic123;	
		qmgr=prefix+qmgr;
		mappingRule1=prefix+mappingRule1;
		mappingRule2=prefix+mappingRule2;
		mappingRule3=prefix+mappingRule3;
		mappingRule4=prefix+mappingRule4;
		mappingRule5=prefix+mappingRule5;
		mappingRule6=prefix+mappingRule6;		
		
		imaqueueList.add(imaqueueA);
		imaqueueList.add(imaqueueB);
		
		mqqueueList.add(mqqueueA);
		mqqueueList.add(mqqueueB);
		
		imatopicList.add(imatopic1);
		imatopicList.add(imatopic2);
		imatopicList.add(imatopic3);
		imatopicList.add(imatopicA);
		imatopicList.add(imatopicB);
		imatopicList.add(imatopic123);
		
		mqtopicList.add(mqtopic123);
		mqtopicList.add(mqtopicA);
		mqtopicList.add(mqtopicB);
		mqtopicList.add(mqtopic1);
		mqtopicList.add(mqtopic2);
		mqtopicList.add(mqtopic3);
		
		if(config.endpointExists(endpoint))
		{
			config.deleteEndpoint(endpoint);
		}
		if(config.messagingPolicyExists(messagingPolicyQ))
		{
			config.deleteMessagingPolicy(messagingPolicyQ);
		}
		if(config.messagingPolicyExists(messagingPolicyT))
		{
			config.deleteMessagingPolicy(messagingPolicyT);
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
		if(! config.messagingPolicyExists(messagingPolicyT))
		{
			config.createMessagingPolicy(messagingPolicyT, messagingPolicyT, "*", TYPE.Topic, TYPE.JMSMQTT);
			config.updateMessagingPolicy(messagingPolicyT, "MaxMessages", "20000000");
		}
		if(! config.endpointExists(endpoint))
		{
		config.createEndpoint(endpoint, endpoint, port, TYPE.JMSMQTT, connectionPolicy, messagingPolicyQ+"," + messagingPolicyT, messageHub);
		}
		
		for(int i=0; i<imatopicList.size(); i++)
		{
			HashMap<String,SubscriptionStatResult> monitorResults = monitor.getSubStatisticsByTopic(imatopicList.get(i));
		
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

		if(imamqconfig.mappingRuleExists(mappingRule1))
		{
			imamqconfig.updateMappingRule(mappingRule1, "Enabled", "False");
			imamqconfig.deleteMappingRule(mappingRule1);

		}
		
		if(imamqconfig.mappingRuleExists(mappingRule2))
		{
			imamqconfig.updateMappingRule(mappingRule2, "Enabled", "False");
			imamqconfig.deleteMappingRule(mappingRule2);
		}
		
		if(imamqconfig.mappingRuleExists(mappingRule3))
		{
			imamqconfig.updateMappingRule(mappingRule3, "Enabled", "False");
			imamqconfig.deleteMappingRule(mappingRule3);
		}
		
		if(imamqconfig.mappingRuleExists(mappingRule4))
		{
			imamqconfig.updateMappingRule(mappingRule4, "Enabled", "False");
			imamqconfig.deleteMappingRule(mappingRule4);
		}
		
		if(imamqconfig.mappingRuleExists(mappingRule5))
		{
			imamqconfig.updateMappingRule(mappingRule5, "Enabled", "False");
			imamqconfig.deleteMappingRule(mappingRule5);
		}
		
		if(imamqconfig.mappingRuleExists(mappingRule6))
		{
			imamqconfig.updateMappingRule(mappingRule6, "Enabled", "False");
			imamqconfig.deleteMappingRule(mappingRule6);
		}
		
		for(int i=0; i<imaqueueList.size(); i++)
		{
			if(config.queueExists(imaqueueList.get(i)))
			{
				config.deleteQueue(imaqueueList.get(i), true);
			}
			config.createQueue(imaqueueList.get(i), "SVT MQ Integrity queue", 20000000);
		}
		
		if(imamqconfig.queueManagerConnectionExists(qmgr))
		{
			imamqconfig.deleteQueueManagerConnection(qmgr);
		}
		
		
		try
		{
		// See if QMGR exists
		if(mqconfig.qmgrExists(qmgr))
		{
			// try to delete
			mqconfig.deleteQmgr(qmgr);
	
		}
		}catch(Exception e)
		{
			e.printStackTrace();
		}
		
		mqconfig.createQmgr(qmgr);
		mqconfig.startQmgr(qmgr);
		mqconfig.createIMSChannel(channelName, mquser, qmgr);
		mqconfig.createListener(listenerName, mqjmsport, qmgr);
		mqconfig.startListener(listenerName, qmgr);
		mqconfig.configureMQChannelAuth(channelName, mquser, qmgr);
		mqconfig.createIMSChannel(clientChannelName, mquser, qmgr);
		mqconfig.configureMQChannelAuth(clientChannelName, mquser, qmgr);
		mqconfig.createMqttService(mqmqttport, qmgr);
			
		for(int i=0; i<mqqueueList.size(); i++)
		{
			mqconfig.createQueueLocal(mqqueueList.get(i), qmgr);
			mqconfig.setAuthRecordQueue(mqqueueList.get(i), mquser, qmgr);
		}
		for(int i=0; i<mqtopicList.size(); i++)
		{
			mqconfig.createTopic(mqtopicList.get(i).replace("/", "_"), mqtopicList.get(i), qmgr);
			mqconfig.setAuthRecordTopic(mqtopicList.get(i).replace("/", "_"), mquser, qmgr);
		}
		
		
		imamqconfig.createQueueManagerConnection(qmgr, qmgr, mqipaddress+"("+mqjmsport+")", channelName);
		imamqconfig.createMappingRule(mappingRule1, qmgr, "6", subtree1, mqtopic123);
		imamqconfig.createMappingRule(mappingRule2, qmgr, "1", imatopicA, mqqueueA);
		imamqconfig.createMappingRule(mappingRule3, qmgr, "11", imaqueueA, mqtopicA);
		imamqconfig.createMappingRule(mappingRule4, qmgr, "13", mqtopicB, imaqueueB);
		imamqconfig.createMappingRule(mappingRule5, qmgr, "3", mqqueueB, imatopicB);
		imamqconfig.createMappingRule(mappingRule6, qmgr, "8", subtree2, imatopic123);
			
		imamqconfig.updateMappingRule(mappingRule3, "RetainedMessages", "All");
		imamqconfig.updateMappingRule(mappingRule5, "RetainedMessages", "All");
		
		
		
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
		try
		{
			mqconfig.disconnectServer();
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
		if(!mqconfig.isConnected())
		{
			mqconfig.connectToServer();
		}
		if(!imamqconfig.isConnected())
		{
			imamqconfig.connectToServer();
		}
		
		for(int i=0; i<imatopicList.size(); i++)
		{
			HashMap<String,SubscriptionStatResult> monitorResults = monitor.getSubStatisticsByTopic(imatopicList.get(i));
		
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
		
		for(int i=0; i<imaqueueList.size(); i++)
		{
			if(config.queueExists(imaqueueList.get(i)))
			{
				config.deleteQueue(imaqueueList.get(i), true);
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
			config.deleteMessagingPolicy(messagingPolicyQ);
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
			config.deleteMessageHub(messageHub);
		}
		catch(Exception e)
		{
			// do nothing
		}
		
		if(imamqconfig.mappingRuleExists(mappingRule1))
		{
			imamqconfig.updateMappingRule(mappingRule1, "Enabled", "False");
			imamqconfig.deleteMappingRule(mappingRule1);
			
		}
		
		if(imamqconfig.mappingRuleExists(mappingRule2))
		{
			imamqconfig.updateMappingRule(mappingRule2, "Enabled", "False");
			imamqconfig.deleteMappingRule(mappingRule2);
			
		}
		
		if(imamqconfig.mappingRuleExists(mappingRule3))
		{
			imamqconfig.updateMappingRule(mappingRule3, "Enabled", "False");
			imamqconfig.deleteMappingRule(mappingRule3);
			
		}
		
		if(imamqconfig.mappingRuleExists(mappingRule4))
		{
			imamqconfig.updateMappingRule(mappingRule4, "Enabled", "False");
			imamqconfig.deleteMappingRule(mappingRule4);
			
		}
		
		if(imamqconfig.mappingRuleExists(mappingRule5))
		{
			imamqconfig.updateMappingRule(mappingRule5, "Enabled", "False");
			imamqconfig.deleteMappingRule(mappingRule5);
			
		}
		if(imamqconfig.mappingRuleExists(mappingRule6))
		{
			imamqconfig.updateMappingRule(mappingRule6, "Enabled", "False");
			imamqconfig.deleteMappingRule(mappingRule6);
			
		}
		
		if(imamqconfig.queueManagerConnectionExists(qmgr))
		{
			imamqconfig.deleteQueueManagerConnection(qmgr);
		}
		
		try
		{
			mqconfig.stopQmgr(qmgr);
		}
		catch(Exception e)
		{
			// do nothing
		}	

		try
		{
			mqconfig.deleteQmgr(qmgr);
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
		try
		{
			mqconfig.disconnectServer();
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			imamqconfig.disconnectServer();
		}
		catch(Exception e)
		{
			// do nothing
		}
	}

	public void run() {
		
		try
		{
			System.out.println("STARTING SETUP");
			HashMap<String, String> properties = new HashMap<String, String>();
			ArrayList<String> clientIDs = new ArrayList<String>();
			for(int i=0; i<6; i++)
			{
				String clientID = null;
				do{
					clientID = UUID.randomUUID().toString().substring(0, 15);
				}
				while(clientIDs.contains(clientID));
				clientIDs.add(clientID);
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
			
			
			
			// MAPPING RULE1 START
			//OK now we need to set the producers and consumers going
			if(Trace.traceOn())
			{
				Trace.trace("Starting all the consumers for " + mqtopic123);
			}
			// Set up selectors ack consumers and producers
			
			for(int i=0; i<2; i++) 
			{
				JmsConsumerClient sub = new JmsConsumerClient(mqipaddress, mqjmsport, qmgr, clientChannelName);
				sub.setSelector("Food = 'Chips'");
				sub.setDurable();
				sub.setRegressionListener(clientIDs);
				if(traceClients)
				{
					sub.setTraceFile(mappingRule1 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.TOPIC, mqtopic123);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + imatopic1);
			}
			properties = new HashMap<String, String>();
			properties.put("Food", "Chips");
			
			for(int i=0; i<2; i++) 
			{
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setTransactedSession(1);
				prod.setMessageInterval(messageInterval);
				prod.setHeaderProperties(properties);
				prod.setClientID(clientIDs.get(i));
				if(traceClients)
				{
					prod.setTraceFile(mappingRule1 + "_ProdTopic1" + i);
				}
				prod.setup(null, null, imatopic1, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			}
			if(Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + imatopic2);
			}
			
			
			
			for(int i=0; i<2; i++) 
			{
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setTransactedSession(1);
				prod.setHeaderProperties(properties);
				prod.setClientID(clientIDs.get(i+2));
				if(traceClients)
				{
					prod.setTraceFile(mappingRule1 + "_ProdTopic2" + i);
				}
				prod.setup(null, null, imatopic2, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			}
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + imatopic3);
			}
			
			for(int i=0; i<2; i++) 
			{
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setTransactedSession(1);
				prod.setMessageInterval(messageInterval);
				prod.setHeaderProperties(properties);
				prod.setClientID(clientIDs.get(i+4));
				if(traceClients)
				{
					prod.setTraceFile(mappingRule1 + "_ProdTopic3" + i);
				}
				prod.setup(null, null, imatopic3, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			}
			// MAPPING RULE1 FINISH
			
			
			// MAPPING RULE2 START
			if(Trace.traceOn())
			{
				Trace.trace("Starting all the consumers for " + mqqueueA);
			}
			// Set up selectors ack consumers and producers
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
			for(int i=0; i<2; i++) 
			{
				JmsConsumerClient sub = new JmsConsumerClient(mqipaddress, mqjmsport, qmgr, clientChannelName);
				sub.setRegressionListener(clientIDs);
				if(traceClients)
				{
					sub.setTraceFile(mappingRule2 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, mqqueueA);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			
			if(Trace.traceOn())
			{
				Trace.trace("Starting all the producers for " + imatopicA);
			}
			for(int i=0; i<2; i++)
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
				prod.numberMessages(numberMessages);
				if(traceClients)
				{
					prod.setTraceFile(mappingRule2 + "_Prod" + i);
				}
				prod.setUpClient(imatopicA);
				mqttProducerList.add(prod);
				new Thread(prod).start();
			}
			// MAPPING RULE2 FINISH
			
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
			// MAPPING RULE3 START
			if(Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + mqtopicA);
			}
			for(int i=0; i<3; i++)
			{
				MqttSubscriberClient sub = null;
				if(urls!= null)
				{
					sub = new MqttSubscriberClient("tcp://" + mqipaddress + ":" + mqmqttport, urls);
				}
				else
				{
					sub = new MqttSubscriberClient("tcp://" + mqipaddress + ":" + mqmqttport);
				}
				
				sub.setCleanSession(true);
				sub.setQos(2);
				sub.setRegressionListener(clientIDs);
				if(traceClients)
				{
					sub.setTraceFile(mappingRule3 + "_Sub" + i);
				}
				sub.setup(mqtopicA);
				mqttConsumerList.add(sub);
				new Thread(sub).start();
			}
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + imaqueueA);
			}
			
			for(int i=0; i<2; i++) 
			{				
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setTransactedSession(1);
				prod.setMessageInterval(messageInterval);
				prod.setClientID(clientIDs.get(i));
				if(traceClients)
				{
					prod.setTraceFile(mappingRule3 + "_Prod" + i);
				}
				prod.setup(null, null, imaqueueA, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();

			}
			
			// MAPPING RULE3 FINISH
			
			// MAPPING RULE4 START
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
			if(Trace.traceOn())
			{
				Trace.trace("Starting all the consumers for " + imaqueueB);
			}
			// Set up selectors ack consumers and producers
			
			for(int i=0; i<2; i++) 
			{
				JmsConsumerClient sub = new JmsConsumerClient(ipaddress, new Integer(port));
				sub.setRegressionListener(clientIDs);
				if(traceClients)
				{
					sub.setTraceFile(mappingRule4 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, imaqueueB);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + mqtopicB);
			}
			
			for(int i=0; i<2; i++) 
			{
				JmsProducerClient prod = new JmsProducerClient(mqipaddress, mqjmsport, qmgr, clientChannelName);				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setTransactedSession(1);
				prod.setMessageInterval(messageInterval);
				prod.setHeaderProperties(properties);
				prod.setClientID(clientIDs.get(i));
				if(traceClients)
				{
					prod.setTraceFile(mappingRule4 + "_Prod" + i);
				}
				prod.setup(null, null, mqtopicB, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			}
			
			// MAPPING RULE4 FINISH
			
			
			// MAPPING RULE5 START
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
			
			if(Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + imatopicB);
			}
			for(int i=0; i<2; i++)
			{
				MqttSubscriberClient sub = null;
				if(urls!= null)
				{
					sub = new MqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					sub = new MqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				sub.setCleanSession(false);
				sub.setQos(2);
				sub.setRegressionListener(clientIDs);
				if(traceClients)
				{
					sub.setTraceFile(mappingRule5 + "_Sub" + i);
				}
				sub.setup(imatopicB);
				mqttConsumerList.add(sub);
				new Thread(sub).start();
			}
			
			
			if(Trace.traceOn())
			{
				Trace.trace("Starting all the producers for " + mqqueueB);
			}
			// Set up selectors ack consumers and producers
			
			for(int i=0; i<2; i++) 
			{
				JmsProducerClient prod = new JmsProducerClient(mqipaddress, mqjmsport, qmgr, clientChannelName);				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setTransactedSession(1);
				prod.setHeaderProperties(properties);
				prod.setClientID(clientIDs.get(i));
				if(traceClients)
				{
					prod.setTraceFile(mappingRule5 + "_Prod" + i);
				}
				prod.setup(null, null, mqqueueB, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			}
			
			// MAPPING RULE5 FINISH
			// MAPPING RULE6 START
			
			clientIDs = new ArrayList<String>();
			for(int i=0; i<6; i++)
			{
				String clientID = null;
				do{
					clientID = UUID.randomUUID().toString().substring(0, 15);
				}
				while(clientIDs.contains(clientID));
				clientIDs.add(clientID);
			}
			
			
			if(Trace.traceOn())
			{
				Trace.trace("Starting all the subscribers for " + imatopic123);
			}
			for(int i=0; i<2; i++)
			{
				MqttSubscriberClient sub = null;
				if(urls!= null)
				{
					sub = new MqttSubscriberClient("tcp://" + ipaddress + ":" + port, urls);
				}
				else
				{
					sub = new MqttSubscriberClient("tcp://" + ipaddress + ":" + port);
				}
				sub.setCleanSession(false);
				sub.setQos(2);
				sub.setRegressionListener(clientIDs);
				if(traceClients)
				{
					sub.setTraceFile(mappingRule6 + "_Sub" + i);
				}
				sub.setup(imatopic123);
				mqttConsumerList.add(sub);
				new Thread(sub).start();

			}
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + mqtopic1);
			}
			
			for(int i=0; i<2; i++) 
			{
				JmsProducerClient prod = new JmsProducerClient(mqipaddress, mqjmsport, qmgr, clientChannelName);	
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.setTransactedSession(1);
				prod.setHeaderProperties(properties);
				prod.setClientID(clientIDs.get(i));
				if(traceClients)
				{
					prod.setTraceFile(mappingRule6 + "_ProdTopic1" + i);
				}
				prod.setup(null, null, mqtopic1, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
	
			}
			if(Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + mqtopic2);
			}
			
			for(int i=0; i<2; i++) 
			{
				JmsProducerClient prod = new JmsProducerClient(mqipaddress, mqjmsport, qmgr, clientChannelName);	
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setTransactedSession(1);
				prod.setMessageInterval(messageInterval);
				prod.setHeaderProperties(properties);
				prod.setClientID(clientIDs.get(i+2));
				if(traceClients)
				{
					prod.setTraceFile(mappingRule6 + "_ProdTopic2" + i);
				}
				prod.setup(null, null, mqtopic2, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			}
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + mqtopic3);
			}
			
			for(int i=0; i<2; i++) 
			{
				JmsProducerClient prod = new JmsProducerClient(mqipaddress, mqjmsport, qmgr, clientChannelName);	
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setTransactedSession(1);
				prod.setMessageInterval(messageInterval);
				prod.setHeaderProperties(properties);
				prod.setClientID(clientIDs.get(i+4));
				if(traceClients)
				{
					prod.setTraceFile(mappingRule6 + "_ProdTopic3" + i);
				}
				prod.setup(null, null, mqtopic3, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
	
			}			
			// MAPPING RULE6 FINISH
			
			
			String failMessage = "";
			while(keepRunning)
			{
				Thread.sleep(180000);
				
				// now check we don't have a problem with any of the messages
				for(int i=0; i<jmsConsumerList.size(); i++)
				{
					if(! jmsConsumerList.get(i).getFailMessage().equals(""))
					{
						failMessage = failMessage + jmsConsumerList.get(i).getFailMessage() + "\n";
					}
							
				}
				for(int i=0; i<mqttConsumerList.size(); i++)
				{
					if(! mqttConsumerList.get(i).getFailMessage().equals(""))
					{
						failMessage = failMessage + mqttConsumerList.get(i).getFailMessage() + "\n";
					}
							
				}
				if(!failMessage.equals(""))
				{
					File audit = new File("./errors/SendAndCheckMQMessages.txt");
					if(!audit.exists())
					{
						audit.createNewFile();
					}
					PrintStream writer = new PrintStream("./errors/SendAndCheckMQMessages.txt", "UTF-8");
					writer.println(failMessage);
					writer.flush();
					keepRunning = false;
				}
				else
				{
					System.out.println("All messages are accounted for");
				}
				
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
			if(Trace.traceOn())
			{
				Trace.trace("SendAndCheckMQMessages Thread has been asked to stop so cleaning up all consumers and producers");
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
			
			if(Trace.traceOn())
			{
				Trace.trace("SendAndCheckMQMessages STOPPED");
			}
			
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
	
	}
	

}
