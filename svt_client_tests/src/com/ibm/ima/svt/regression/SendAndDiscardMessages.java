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


import javax.jms.Session;

import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.config.ImaConfig.TYPE;
import com.ibm.ima.test.cli.monitor.ImaMonitoring;
import com.ibm.ima.test.cli.monitor.SubscriptionStatResult;
import com.ibm.ims.svt.clients.JmsConsumerClient;
import com.ibm.ims.svt.clients.JmsMessage;
import com.ibm.ims.svt.clients.JmsProducerClient;
import com.ibm.ims.svt.clients.MqttProducerClient;
import com.ibm.ims.svt.clients.MqttSubscriberClient;
import com.ibm.ims.svt.clients.MultiJmsConsumerClient;
import com.ibm.ims.svt.clients.Trace;
import com.ibm.ims.svt.clients.JmsMessage.DESTINATION_TYPE;
import com.ibm.ima.test.cli.imaserver.Version;

/**
 * This test creates a number of clients, some of which will hit the max messages and therefore
 * will trigger the discard messages.
 * 
 * JMS producers are used to check that they are not kicked off the appliance of the subscribers hit
 * the discard messages code.
 * 
 * The subscribers are also configured to check for messages arriving ever so often as discard messages
 * is fairly non deterministic in what they throw away and when.
 *
 */
public class SendAndDiscardMessages implements Runnable{
	
	private String port = null;
	private String ipaddress = null;
	private String prefix = "";
	private String cliaddressA = null;
	private String cliaddressB = null;
	private String topic1 = "TOPIC1";
	private String topic2 = "TOPIC2";
	private String topic3 = "TOPIC3";
	
	
	private ImaConfig config = null;
	private ImaMonitoring monitor = null;
	private String cliendIDOLD=UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	private String cliendIDOLD_JMS=UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	private String cliendIDNEW1=UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	private String cliendIDNEW2=UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	private String cliendIDNEW3=UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	private String cliendIDNEW4=UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	private String cliendIDNEW5=UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	private String cliendIDNEW1_JMS=UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	private String cliendIDNEW2_JMS=UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	private String cliendIDNEW3_JMS=UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	private String cliendIDNEW4_JMS=UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	private String cliendIDNEW5_JMS=UUID.randomUUID().toString().substring(0, 20).replace('-', '_');
	
	private String endpoint = "SVTDiscardMsgsTopicEP";
	private String messagingPolicy_p1 = "SVTDiscardMsgsTopicMP_P"; //pub 1
	private String messagingPolicy_p2 = "SVTDiscardMsgsTopicMP_2"; //pub 2
	private String messagingPolicy_p3 = "SVTDiscardMsgsTopicMP_3"; //pub 3
	private String messagingPolicy_SO = "SVTDiscardMsgsTopicMP_SO"; // subscriber old behaviour mqtt
	private String messagingPolicy_SO_JMS = "SVTDiscardMsgsTopicMP_SO_JMS"; // subscriber old behaviour jms
	private String messagingPolicy_SN1 = "SVTDiscardMsgsTopicMP_SN1"; // subscriber new behaviour 1 mqtt
	private String messagingPolicy_SN2 = "SVTDiscardMsgsTopicMP_SN2"; // subscriber new behaviour 2 mqtt
	private String messagingPolicy_SN3 = "SVTDiscardMsgsTopicMP_SN3"; // subscriber new behaviour 3 mqtt
	private String messagingPolicy_SN4 = "SVTDiscardMsgsTopicMP_SN4"; // subscriber new behaviour 4 mqtt
	private String messagingPolicy_SN5 = "SVTDiscardMsgsTopicMP_SN5"; // subscriber new behaviour 5 mqtt
	private String messagingPolicy_SN1_JMS = "SVTDiscardMsgsTopicMP_SN1_JMS"; // subscriber new behaviour 1 JMS
	private String messagingPolicy_SN2_JMS = "SVTDiscardMsgsTopicMP_SN2_JMS"; // subscriber new behaviour 1 JMS
	private String messagingPolicy_SN3_JMS = "SVTDiscardMsgsTopicMP_SN3_JMS"; // subscriber new behaviour 1 JMS
	private String messagingPolicy_SN4_JMS = "SVTDiscardMsgsTopicMP_SN4_JMS"; // subscriber new behaviour 1 JMS
	private String messagingPolicy_SN5_JMS = "SVTDiscardMsgsTopicMP_SN5_JMS"; // subscriber new behaviour 1 JMS
	private String messagingPolicy_SNA = "SVTDiscardMsgsTopicMP_SNA";// subscriber new behaviour all remaining mqtt
	private String messagingPolicy_SNA_JMS = "SVTDiscardMsgsTopicMP_SNA_JMS"; // subscriber new behaviour all remaining JMS
	private String messagingPolicy_SS_D = "SVTDiscardMsgsTopicMP_SS_D"; // subscriber shared sub durable behaviour
	private String messagingPolicy_SS_ND = "SVTDiscardMsgsTopicMP_SS_ND"; // subscriber shared sub non durable behaviour
	private String connectionPolicy = "SVTDiscardMsgsCP";
	private String messageHub = "SVTDiscardMsgsTopicHub";
	

	
	private ArrayList<String> mqttClientIDs = new ArrayList<String>();
	private ArrayList<String> jmsClientIDs = new ArrayList<String>();
	
	private ArrayList<String> topicList = new ArrayList<String>();
	private ArrayList<JmsProducerClient> jmsProducerList = new ArrayList<JmsProducerClient>();
	private ArrayList<MqttProducerClient> mqttProducerList = new ArrayList<MqttProducerClient>();
	private ArrayList<MqttSubscriberClient> mqttConsumerList = new ArrayList<MqttSubscriberClient>();
	private ArrayList<JmsConsumerClient> jmsConsumerList = new ArrayList<JmsConsumerClient>();
	private ArrayList<MultiJmsConsumerClient> jmsMultiConsumerList = new ArrayList<MultiJmsConsumerClient>();
	
	private int numberMessages = 10000000;
	private int messageInterval = 500;
	private int checkMsgIntervalVal = 240000; // Has to be very long as servers get slow when under stress when doing a imaserver stop (not ideal)

	private File directory;
 	
	public boolean keepRunning = true;	
	
	private boolean traceClients = false;
	
		
	public SendAndDiscardMessages(String prefix, String ipaddress, String port, String cliaddress, boolean traceClients) throws Exception
	{
		this.ipaddress = ipaddress.trim().replace('+',',');
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
	
	public SendAndDiscardMessages(String prefix, String ipaddress, String port, String cliaddressA, String cliaddressB, boolean traceClients) throws Exception
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
			 * Assumeing ',' is being used.
			 * 
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
		topicList.add(topic1);
		topic2=prefix+topic2;
		topicList.add(topic2);
		topic3=prefix+topic3;
		topicList.add(topic3);
		
		mqttClientIDs.add(cliendIDOLD);
		mqttClientIDs.add(cliendIDNEW1);
		mqttClientIDs.add(cliendIDNEW2);
		mqttClientIDs.add(cliendIDNEW3);
		mqttClientIDs.add(cliendIDNEW4);
		mqttClientIDs.add(cliendIDNEW5);
		jmsClientIDs.add(cliendIDOLD_JMS);
		jmsClientIDs.add(cliendIDNEW1_JMS);
		jmsClientIDs.add(cliendIDNEW2_JMS);
		jmsClientIDs.add(cliendIDNEW3_JMS);
		jmsClientIDs.add(cliendIDNEW4_JMS);
		jmsClientIDs.add(cliendIDNEW5_JMS);
			
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
		if(config.messagingPolicyExists(messagingPolicy_p1))
		{
			config.deleteMessagingPolicy(messagingPolicy_p1);
		}
		if(config.messagingPolicyExists(messagingPolicy_p2))
		{
			config.deleteMessagingPolicy(messagingPolicy_p2);
		}
		if(config.messagingPolicyExists(messagingPolicy_p3))
		{
			config.deleteMessagingPolicy(messagingPolicy_p3);
		}
		if(config.messagingPolicyExists(messagingPolicy_SO))
		{
			config.deleteMessagingPolicy(messagingPolicy_SO);
		}
		if(config.messagingPolicyExists(messagingPolicy_SO_JMS))
		{
			config.deleteMessagingPolicy(messagingPolicy_SO_JMS);
		}
		if(config.messagingPolicyExists(messagingPolicy_SN1))
		{
			config.deleteMessagingPolicy(messagingPolicy_SN1);
		}
		if(config.messagingPolicyExists(messagingPolicy_SN1_JMS))
		{
			config.deleteMessagingPolicy(messagingPolicy_SN1_JMS);
		}
		if(config.messagingPolicyExists(messagingPolicy_SN2))
		{
			config.deleteMessagingPolicy(messagingPolicy_SN2);
		}
		if(config.messagingPolicyExists(messagingPolicy_SN2_JMS))
		{
			config.deleteMessagingPolicy(messagingPolicy_SN2_JMS);
		}
		if(config.messagingPolicyExists(messagingPolicy_SN3))
		{
			config.deleteMessagingPolicy(messagingPolicy_SN3);
		}
		if(config.messagingPolicyExists(messagingPolicy_SN3_JMS))
		{
			config.deleteMessagingPolicy(messagingPolicy_SN3_JMS);
		}
		if(config.messagingPolicyExists(messagingPolicy_SN4))
		{
			config.deleteMessagingPolicy(messagingPolicy_SN4);
		}
		if(config.messagingPolicyExists(messagingPolicy_SN4_JMS))
		{
			config.deleteMessagingPolicy(messagingPolicy_SN4_JMS);
		}
		if(config.messagingPolicyExists(messagingPolicy_SN5))
		{
			config.deleteMessagingPolicy(messagingPolicy_SN5);
		}
		if(config.messagingPolicyExists(messagingPolicy_SN5_JMS))
		{
			config.deleteMessagingPolicy(messagingPolicy_SN5_JMS);
		}
		if(config.messagingPolicyExists(messagingPolicy_SNA))
		{
			config.deleteMessagingPolicy(messagingPolicy_SNA);
		}
		if(config.messagingPolicyExists(messagingPolicy_SNA_JMS))
		{
			config.deleteMessagingPolicy(messagingPolicy_SNA_JMS);
		}
		if(config.messagingPolicyExists(messagingPolicy_SS_D))
		{
			config.deleteMessagingPolicy(messagingPolicy_SS_D);
		}
		if(config.messagingPolicyExists(messagingPolicy_SS_ND))
		{
			config.deleteMessagingPolicy(messagingPolicy_SS_ND);
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
		if(! config.messagingPolicyExists(messagingPolicy_p1))
		{
			config.createMessagingPolicy(messagingPolicy_p1, messagingPolicy_p1, topic1, TYPE.Topic, TYPE.JMSMQTT);
			config.updateMessagingPolicy(messagingPolicy_p1, "MaxMessages", "20000000");
			config.updateMessagingPolicy(messagingPolicy_p1, "ActionList", "Publish");
		}
		if(! config.messagingPolicyExists(messagingPolicy_p2))
		{
			config.createMessagingPolicy(messagingPolicy_p2, messagingPolicy_p2, topic2, TYPE.Topic, TYPE.JMSMQTT);
			config.updateMessagingPolicy(messagingPolicy_p2, "MaxMessages", "20000000");
			config.updateMessagingPolicy(messagingPolicy_p2, "ActionList", "Publish");
		}
		if(! config.messagingPolicyExists(messagingPolicy_p3))
		{
			config.createMessagingPolicy(messagingPolicy_p3, messagingPolicy_p3, topic3, TYPE.Topic, TYPE.JMSMQTT);
			config.updateMessagingPolicy(messagingPolicy_p3, "MaxMessages", "20000000");
			config.updateMessagingPolicy(messagingPolicy_p3, "ActionList", "Publish");
		}
		
		/*
		 * RejectNewMessages and DiscardOldMessages available starting at 14a
		 */
		switch(Version.getVersion()) {
			case VER_100:
			case VER_110:
				/* 
				 * for 13a and 13b remove RejectMessage policy and DiscardOldMessages. 
				 * 
				 * !!!!!!!!CHECK if this test is really needed for 13 releases!!!!!!!!!
				 * Seems the point of testcase is to test DiscardOldMessages. Therefore this testcase may
				 * not really test features of 13 releases
				 * 
				 * Since 13 release does not have MaxMessagesBehavior feature, then expectation of this testcase
				 * is a failure in ./errors/<error file> to indicate  something simliar to
				 * 'FailMessage:The producer cbd54649-edfc-4 for topic SVTDIS-TOPIC2 was rejected during the publish and we 
				 * are configured to stop on publish failure'
				 */
				
				if(! config.messagingPolicyExists(messagingPolicy_SO))
				{
					config.createMessagingPolicy(messagingPolicy_SO, messagingPolicy_SO, topic1, TYPE.Topic, TYPE.JMSMQTT);
					config.updateMessagingPolicy(messagingPolicy_SO, "MaxMessages", "200");
					config.updateMessagingPolicy(messagingPolicy_SO, "ActionList", "Subscribe");
				}
				config.updateMessagingPolicy(messagingPolicy_SO, "ClientID", cliendIDOLD);
				
				if(! config.messagingPolicyExists(messagingPolicy_SO_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SO_JMS, messagingPolicy_SO_JMS, topic3, TYPE.Topic, TYPE.JMSMQTT);
					config.updateMessagingPolicy(messagingPolicy_SO_JMS, "MaxMessages", "200");
					config.updateMessagingPolicy(messagingPolicy_SO_JMS, "ActionList", "Subscribe");
				}
				config.updateMessagingPolicy(messagingPolicy_SO_JMS, "ClientID", cliendIDOLD_JMS);

				if(! config.messagingPolicyExists(messagingPolicy_SN1))
				{
					config.createMessagingPolicy(messagingPolicy_SN1, messagingPolicy_SN1, topic1, TYPE.Topic, TYPE.MQTT);
					config.updateMessagingPolicy(messagingPolicy_SN1, "MaxMessages", "200");
					config.updateMessagingPolicy(messagingPolicy_SN1, "ActionList", "Subscribe");
				}
				config.updateMessagingPolicy(messagingPolicy_SN1, "ClientID", cliendIDNEW1);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN1_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SN1_JMS, messagingPolicy_SN1_JMS, topic1, TYPE.Topic, TYPE.JMS);
					config.updateMessagingPolicy(messagingPolicy_SN1_JMS, "MaxMessages", "200");
					config.updateMessagingPolicy(messagingPolicy_SN1_JMS, "ActionList", "Subscribe");
				}
				config.updateMessagingPolicy(messagingPolicy_SN1_JMS, "ClientID", cliendIDNEW1_JMS);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN2))
				{
					config.createMessagingPolicy(messagingPolicy_SN2, messagingPolicy_SN2, topic1, TYPE.Topic, TYPE.MQTT);
					config.updateMessagingPolicy(messagingPolicy_SN2, "MaxMessages", "5");
					config.updateMessagingPolicy(messagingPolicy_SN2, "ActionList", "Subscribe");
				}
				config.updateMessagingPolicy(messagingPolicy_SN2, "ClientID", cliendIDNEW2);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN2_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SN2_JMS, messagingPolicy_SN2_JMS, topic1, TYPE.Topic, TYPE.JMS);
					config.updateMessagingPolicy(messagingPolicy_SN2_JMS, "MaxMessages", "5");
					config.updateMessagingPolicy(messagingPolicy_SN2_JMS, "ActionList", "Subscribe");
				}
				config.updateMessagingPolicy(messagingPolicy_SN2_JMS, "ClientID", cliendIDNEW2_JMS);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN3))
				{
					config.createMessagingPolicy(messagingPolicy_SN3, messagingPolicy_SN3, topic1, TYPE.Topic, TYPE.MQTT);
					config.updateMessagingPolicy(messagingPolicy_SN3, "MaxMessages", "10");
					config.updateMessagingPolicy(messagingPolicy_SN3, "ActionList", "Subscribe");
				}
				config.updateMessagingPolicy(messagingPolicy_SN3, "ClientID", cliendIDNEW3);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN3_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SN3_JMS, messagingPolicy_SN3_JMS, topic1, TYPE.Topic, TYPE.JMS);
					config.updateMessagingPolicy(messagingPolicy_SN3_JMS, "MaxMessages", "10");
					config.updateMessagingPolicy(messagingPolicy_SN3_JMS, "ActionList", "Subscribe");
				}
				config.updateMessagingPolicy(messagingPolicy_SN3_JMS, "ClientID", cliendIDNEW3_JMS);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN4))
				{
					config.createMessagingPolicy(messagingPolicy_SN4, messagingPolicy_SN4, topic1, TYPE.Topic, TYPE.MQTT);
					config.updateMessagingPolicy(messagingPolicy_SN4, "MaxMessages", "20000000");
					config.updateMessagingPolicy(messagingPolicy_SN4, "ActionList", "Subscribe");
				}
				config.updateMessagingPolicy(messagingPolicy_SN4, "ClientID", cliendIDNEW4);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN4_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SN4_JMS, messagingPolicy_SN4, topic1, TYPE.Topic, TYPE.JMS);
					config.updateMessagingPolicy(messagingPolicy_SN4_JMS, "MaxMessages", "20000000");
					config.updateMessagingPolicy(messagingPolicy_SN4_JMS, "ActionList", "Subscribe");
				}
				config.updateMessagingPolicy(messagingPolicy_SN4_JMS, "ClientID", cliendIDNEW4_JMS);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN5))
				{
					config.createMessagingPolicy(messagingPolicy_SN5, messagingPolicy_SN5, topic1, TYPE.Topic, TYPE.MQTT);
					config.updateMessagingPolicy(messagingPolicy_SN5, "MaxMessages", "1");
					config.updateMessagingPolicy(messagingPolicy_SN5, "ActionList", "Subscribe");
				}
				config.updateMessagingPolicy(messagingPolicy_SN5, "ClientID", cliendIDNEW5);	
				
				if(! config.messagingPolicyExists(messagingPolicy_SN5_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SN5_JMS, messagingPolicy_SN5_JMS, topic1, TYPE.Topic, TYPE.JMS);
					config.updateMessagingPolicy(messagingPolicy_SN5_JMS, "MaxMessages", "1");
					config.updateMessagingPolicy(messagingPolicy_SN5_JMS, "ActionList", "Subscribe");
				}
				config.updateMessagingPolicy(messagingPolicy_SN5_JMS, "ClientID", cliendIDNEW5_JMS);	
				
				if(! config.messagingPolicyExists(messagingPolicy_SNA))
				{
					config.createMessagingPolicy(messagingPolicy_SNA, messagingPolicy_SNA, topic1, TYPE.Topic, TYPE.JMSMQTT);
					config.updateMessagingPolicy(messagingPolicy_SNA, "MaxMessages", "500");
					config.updateMessagingPolicy(messagingPolicy_SNA, "ActionList", "Subscribe");
				}
				if(! config.messagingPolicyExists(messagingPolicy_SNA_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SNA_JMS, messagingPolicy_SNA_JMS, topic3, TYPE.Topic, TYPE.JMSMQTT);
					config.updateMessagingPolicy(messagingPolicy_SNA_JMS, "MaxMessages", "150");
					config.updateMessagingPolicy(messagingPolicy_SNA_JMS, "ActionList", "Subscribe");
				}
				if(! config.messagingPolicyExists(messagingPolicy_SS_ND))
				{
					config.createMessagingPolicy(messagingPolicy_SS_ND, messagingPolicy_SS_ND, topic2, TYPE.Topic, TYPE.JMSMQTT);
					config.updateMessagingPolicy(messagingPolicy_SS_ND, "MaxMessages", "13");
					config.updateMessagingPolicy(messagingPolicy_SS_ND, "ActionList", "Subscribe");
				}
				if(! config.messagingPolicyExists(messagingPolicy_SS_D))
				{
					/*
					 * Remove MQTT since 13a and 13b only support JMS subscriptions
					 */
					config.createMessagingPolicy(messagingPolicy_SS_D, messagingPolicy_SS_D, "*", TYPE.Subscription, TYPE.JMS);
					config.updateMessagingPolicy(messagingPolicy_SS_D, "MaxMessages", "8");
					config.updateMessagingPolicy(messagingPolicy_SS_D, "ActionList", "Receive,Control");
				}
				break;
			case VER_120:
			case VER_130:	
			default:
				/*
				 * RejectNewMessages should be supported for versions > 14a so for now its also a default
				 */
				if(! config.messagingPolicyExists(messagingPolicy_SO))
				{
					config.createMessagingPolicy(messagingPolicy_SO, messagingPolicy_SO, topic1, TYPE.Topic, TYPE.JMSMQTT);
					config.updateMessagingPolicy(messagingPolicy_SO, "MaxMessages", "200");
					config.updateMessagingPolicy(messagingPolicy_SO, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SO, "MaxMessagesBehavior", "RejectNewMessages");
				}
				config.updateMessagingPolicy(messagingPolicy_SO, "ClientID", cliendIDOLD);
				
				if(! config.messagingPolicyExists(messagingPolicy_SO_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SO_JMS, messagingPolicy_SO_JMS, topic3, TYPE.Topic, TYPE.JMSMQTT);
					config.updateMessagingPolicy(messagingPolicy_SO_JMS, "MaxMessages", "200");
					config.updateMessagingPolicy(messagingPolicy_SO_JMS, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SO_JMS, "MaxMessagesBehavior", "RejectNewMessages");
				}
				config.updateMessagingPolicy(messagingPolicy_SO_JMS, "ClientID", cliendIDOLD_JMS);
				if(! config.messagingPolicyExists(messagingPolicy_SN1))
				{
					config.createMessagingPolicy(messagingPolicy_SN1, messagingPolicy_SN1, topic1, TYPE.Topic, TYPE.MQTT);
					config.updateMessagingPolicy(messagingPolicy_SN1, "MaxMessages", "200");
					config.updateMessagingPolicy(messagingPolicy_SN1, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SN1, "MaxMessagesBehavior", "DiscardOldMessages");	
				}
				config.updateMessagingPolicy(messagingPolicy_SN1, "ClientID", cliendIDNEW1);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN1_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SN1_JMS, messagingPolicy_SN1_JMS, topic1, TYPE.Topic, TYPE.JMS);
					config.updateMessagingPolicy(messagingPolicy_SN1_JMS, "MaxMessages", "200");
					config.updateMessagingPolicy(messagingPolicy_SN1_JMS, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SN1_JMS, "MaxMessagesBehavior", "DiscardOldMessages");	
				}
				config.updateMessagingPolicy(messagingPolicy_SN1_JMS, "ClientID", cliendIDNEW1_JMS);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN2))
				{
					config.createMessagingPolicy(messagingPolicy_SN2, messagingPolicy_SN2, topic1, TYPE.Topic, TYPE.MQTT);
					config.updateMessagingPolicy(messagingPolicy_SN2, "MaxMessages", "5");
					config.updateMessagingPolicy(messagingPolicy_SN2, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SN2, "MaxMessagesBehavior", "DiscardOldMessages");
				}
				config.updateMessagingPolicy(messagingPolicy_SN2, "ClientID", cliendIDNEW2);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN2_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SN2_JMS, messagingPolicy_SN2_JMS, topic1, TYPE.Topic, TYPE.JMS);
					config.updateMessagingPolicy(messagingPolicy_SN2_JMS, "MaxMessages", "5");
					config.updateMessagingPolicy(messagingPolicy_SN2_JMS, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SN2_JMS, "MaxMessagesBehavior", "DiscardOldMessages");
				}
				config.updateMessagingPolicy(messagingPolicy_SN2_JMS, "ClientID", cliendIDNEW2_JMS);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN3))
				{
					config.createMessagingPolicy(messagingPolicy_SN3, messagingPolicy_SN3, topic1, TYPE.Topic, TYPE.MQTT);
					config.updateMessagingPolicy(messagingPolicy_SN3, "MaxMessages", "10");
					config.updateMessagingPolicy(messagingPolicy_SN3, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SN3, "MaxMessagesBehavior", "DiscardOldMessages");
				}
				config.updateMessagingPolicy(messagingPolicy_SN3, "ClientID", cliendIDNEW3);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN3_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SN3_JMS, messagingPolicy_SN3_JMS, topic1, TYPE.Topic, TYPE.JMS);
					config.updateMessagingPolicy(messagingPolicy_SN3_JMS, "MaxMessages", "10");
					config.updateMessagingPolicy(messagingPolicy_SN3_JMS, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SN3_JMS, "MaxMessagesBehavior", "DiscardOldMessages");
				}
				config.updateMessagingPolicy(messagingPolicy_SN3_JMS, "ClientID", cliendIDNEW3_JMS);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN4))
				{
					config.createMessagingPolicy(messagingPolicy_SN4, messagingPolicy_SN4, topic1, TYPE.Topic, TYPE.MQTT);
					config.updateMessagingPolicy(messagingPolicy_SN4, "MaxMessages", "20000000");
					config.updateMessagingPolicy(messagingPolicy_SN4, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SN4, "MaxMessagesBehavior", "DiscardOldMessages");
				}
				config.updateMessagingPolicy(messagingPolicy_SN4, "ClientID", cliendIDNEW4);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN4_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SN4_JMS, messagingPolicy_SN4, topic1, TYPE.Topic, TYPE.JMS);
					config.updateMessagingPolicy(messagingPolicy_SN4_JMS, "MaxMessages", "20000000");
					config.updateMessagingPolicy(messagingPolicy_SN4_JMS, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SN4_JMS, "MaxMessagesBehavior", "DiscardOldMessages");
				}
				config.updateMessagingPolicy(messagingPolicy_SN4_JMS, "ClientID", cliendIDNEW4_JMS);
				
				if(! config.messagingPolicyExists(messagingPolicy_SN5))
				{
					config.createMessagingPolicy(messagingPolicy_SN5, messagingPolicy_SN5, topic1, TYPE.Topic, TYPE.MQTT);
					config.updateMessagingPolicy(messagingPolicy_SN5, "MaxMessages", "1");
					config.updateMessagingPolicy(messagingPolicy_SN5, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SN5, "MaxMessagesBehavior", "DiscardOldMessages");	
				}
				config.updateMessagingPolicy(messagingPolicy_SN5, "ClientID", cliendIDNEW5);	
				
				if(! config.messagingPolicyExists(messagingPolicy_SN5_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SN5_JMS, messagingPolicy_SN5_JMS, topic1, TYPE.Topic, TYPE.JMS);
					config.updateMessagingPolicy(messagingPolicy_SN5_JMS, "MaxMessages", "1");
					config.updateMessagingPolicy(messagingPolicy_SN5_JMS, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SN5_JMS, "MaxMessagesBehavior", "DiscardOldMessages");	
				}
				config.updateMessagingPolicy(messagingPolicy_SN5_JMS, "ClientID", cliendIDNEW5_JMS);	
				
				if(! config.messagingPolicyExists(messagingPolicy_SNA))
				{
					config.createMessagingPolicy(messagingPolicy_SNA, messagingPolicy_SNA, topic1, TYPE.Topic, TYPE.JMSMQTT);
					config.updateMessagingPolicy(messagingPolicy_SNA, "MaxMessages", "500");
					config.updateMessagingPolicy(messagingPolicy_SNA, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SNA, "MaxMessagesBehavior", "DiscardOldMessages");
				}
				if(! config.messagingPolicyExists(messagingPolicy_SNA_JMS))
				{
					config.createMessagingPolicy(messagingPolicy_SNA_JMS, messagingPolicy_SNA_JMS, topic3, TYPE.Topic, TYPE.JMSMQTT);
					config.updateMessagingPolicy(messagingPolicy_SNA_JMS, "MaxMessages", "150");
					config.updateMessagingPolicy(messagingPolicy_SNA_JMS, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SNA_JMS, "MaxMessagesBehavior", "DiscardOldMessages");
				}
				if(! config.messagingPolicyExists(messagingPolicy_SS_ND))
				{
					config.createMessagingPolicy(messagingPolicy_SS_ND, messagingPolicy_SS_ND, topic2, TYPE.Topic, TYPE.JMSMQTT);
					config.updateMessagingPolicy(messagingPolicy_SS_ND, "MaxMessages", "13");
					config.updateMessagingPolicy(messagingPolicy_SS_ND, "ActionList", "Subscribe");
					config.updateMessagingPolicy(messagingPolicy_SS_ND, "MaxMessagesBehavior", "DiscardOldMessages");
				}
				if(! config.messagingPolicyExists(messagingPolicy_SS_D))
				{
					config.createMessagingPolicy(messagingPolicy_SS_D, messagingPolicy_SS_D, "*", TYPE.Subscription, TYPE.JMSMQTT);
					config.updateMessagingPolicy(messagingPolicy_SS_D, "MaxMessages", "8");
					config.updateMessagingPolicy(messagingPolicy_SS_D, "ActionList", "Receive,Control");
					config.updateMessagingPolicy(messagingPolicy_SS_D, "MaxMessagesBehavior", "DiscardOldMessages");
				}
				break;
		}
		
		
		
		if(! config.endpointExists(endpoint))
		{
			config.createEndpoint(endpoint, endpoint, port, TYPE.JMSMQTT, connectionPolicy, messagingPolicy_p1+"," + messagingPolicy_p2 +"," + messagingPolicy_p3 +"," + messagingPolicy_SO + "," + messagingPolicy_SO_JMS + "," + messagingPolicy_SN1 +"," + messagingPolicy_SN2 + ","+ messagingPolicy_SN3 + "," + messagingPolicy_SN4 + "," +messagingPolicy_SN5+ "," + messagingPolicy_SN1_JMS +"," + messagingPolicy_SN2_JMS + ","+ messagingPolicy_SN3_JMS + "," + messagingPolicy_SN4_JMS + "," +messagingPolicy_SN5_JMS + ","+messagingPolicy_SNA  + ","+messagingPolicy_SNA_JMS + "," + messagingPolicy_SS_ND + "," + messagingPolicy_SS_D, messageHub);
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
			config.deleteMessagingPolicy(messagingPolicy_p1);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_p2);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_p3);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SO);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SO_JMS);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SN1);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SN2);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SN3);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SN4);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SN5);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SN1_JMS);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SN2_JMS);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SN3_JMS);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SN4_JMS);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SN5_JMS);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SNA);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SNA_JMS);
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SS_ND);
		}
		catch(Exception e)
		{
			// do nothing
		}	
		try
		{
			config.deleteMessagingPolicy(messagingPolicy_SS_D);
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
	
	
	public void run() 
	{
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
			
			// create all the clients
			for(int i=0; i<mqttClientIDs.size(); i++)
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
				submqtt.setQos(0);
				submqtt.overrideClientID(mqttClientIDs.get(i));
				submqtt.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB);
				if(traceClients)
				{
					submqtt.setTraceFile(topic1 + "_SubMqtt" + mqttClientIDs.get(i));
				}
				submqtt.setup(topic1);				
				mqttConsumerList.add(submqtt);
				new Thread(submqtt).start();
			}
			for(int i=0; i<jmsClientIDs.size(); i++) 
			{
				JmsConsumerClient subjms = new JmsConsumerClient(ipaddress, new Integer(port));
				subjms.setDurable();
				subjms.setAckMode(Session.AUTO_ACKNOWLEDGE, 1);
				subjms.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB);
				subjms.overrideClientID(jmsClientIDs.get(i));
				if(i==0)
				{
					if(traceClients)
					{
						subjms.setTraceFile(topic3 + "_SubJms" + jmsClientIDs.get(i));
					}
					subjms.setup(null, null, DESTINATION_TYPE.TOPIC, topic3);
				}
				else
				{
					if(traceClients)
					{
						subjms.setTraceFile(topic1 + "_SubJms" + jmsClientIDs.get(i));
					}
					subjms.setup(null, null, DESTINATION_TYPE.TOPIC, topic1);
				}
				jmsConsumerList.add(subjms);
				new Thread(subjms).start();
			}
			for(int i=0; i<15; i++)
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
				if(i%3 == 0)
				{
					submqtt.setQos(0);
					submqtt.disconnect(10000);
				}
				else if(i%4 == 0)
				{
					submqtt.setQos(1);
					submqtt.setCleanSession(false);
					submqtt.disconnect(60000);
				}
				else if(i%5 == 0)
				{
					submqtt.setQos(2);	
					submqtt.setCleanSession(false);
					submqtt.disconnect(240000);
				}
				else
				{
					submqtt.setQos(2);
				}
				submqtt.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB);
				if(traceClients)
				{
					submqtt.setTraceFile(topic1 + "_SubMqtt" + i);
				}
				submqtt.setup(topic1);				
				mqttConsumerList.add(submqtt);
				new Thread(submqtt).start();
			}
			
			
			for(int i=0; i<10; i++)
			{
				MultiJmsConsumerClient subjms = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				subjms.setAckMode(Session.AUTO_ACKNOWLEDGE, 1);
				if(i%3 == 0)
				{
					subjms.isDurable();
					subjms.numberClients(5);
					subjms.disconnect(10000);
				}
				else if(i%4 == 0)
				{
					subjms.numberClients(5);
					subjms.disconnect(20000);
				}
				else if(i%5 == 0)
				{
					subjms.numberClients(5);
					subjms.unsubscribe(30000, false);
				}
				else
				{
					subjms.numberClients(5);
				}
				subjms.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB); 
				if(traceClients)
				{
					subjms.setTraceFile(topic1 + "_SubJms" + i);
				}
				subjms.setup(null, null, DESTINATION_TYPE.TOPIC, topic1);				
				jmsMultiConsumerList.add(subjms);
				new Thread(subjms).start();
			}
			
			for(int i=0; i<10; i++)
			{
				MultiJmsConsumerClient subjms = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				subjms.setAckMode(Session.AUTO_ACKNOWLEDGE, 1);
				if(i%3 == 0)
				{
					subjms.isDurable();
					subjms.numberClients(5);
					subjms.disconnect(10000);
				}
				else if(i%4 == 0)
				{
					subjms.numberClients(5);
					subjms.disconnect(20000);
				}
				else if(i%5 == 0)
				{
					subjms.numberClients(5);
					subjms.unsubscribe(30000, false);
				}
				else
				{
					subjms.numberClients(5);
				}
				subjms.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB); 
				if(traceClients)
				{
					subjms.setTraceFile(topic3 + "_SubJms" + i);
				}
				subjms.setup(null, null, DESTINATION_TYPE.TOPIC, topic3);				
				jmsMultiConsumerList.add(subjms);
				new Thread(subjms).start();
			}
			
			for(int i=0; i<10; i++)
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
				submqtt.setQos(0);
				if(i%3 == 0)
				{
					submqtt.unsubscribe(120000);
				}
				if(i%3 == 0)
				{
					submqtt.disconnect(60000);
				}
				if(traceClients)
				{
					submqtt.setTraceFile(topic2 + "_SubMqttA" + i);
				}
				submqtt.setMaxMessagesToKeep(1000, 200);
				submqtt.setup("$SharedSubscription/" +prefix + "Sub/"+ topic2);					
				mqttConsumerList.add(submqtt);
				new Thread(submqtt).start();
			}
			
			for(int i=0; i<10; i++)
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
				if(i%3 == 0)
				{
					submqtt.unsubscribe(120000);
				}
				if(i%3 == 0)
				{
					submqtt.disconnect(60000);
				}
				if(traceClients)
				{
					submqtt.setTraceFile(topic2 + "_SubMqttB" + i);
				}
				submqtt.setMaxMessagesToKeep(1000, 200);
				submqtt.setup("$SharedSubscription/" +prefix + "Sub/"+ topic2);					
				mqttConsumerList.add(submqtt);
				new Thread(submqtt).start();
			}
			
			for(int i=0; i<10; i++)
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
				submqtt.setQos(0);
				if(i%3 == 0)
				{
					submqtt.unsubscribe(120000);
				}
				if(i%3 == 0)
				{
					submqtt.disconnect(60000);
				}
				if(traceClients)
				{
					submqtt.setTraceFile(topic2 + "_SubMqttC" + i);
				}
				submqtt.setMaxMessagesToKeep(1000, 200);
				submqtt.setup("$SharedSubscription/" +prefix + "Sub/"+ topic2);					
				mqttConsumerList.add(submqtt);
				new Thread(submqtt).start();
			}
			
			for(int i=0; i<10; i++)
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
				submqtt.setQos(1);
				if(i%3 == 0)
				{
					submqtt.unsubscribe(120000);
				}
				if(i%3 == 0)
				{
					submqtt.disconnect(60000);
				}
				if(traceClients)
				{
					submqtt.setTraceFile(topic2 + "_SubMqttD" + i);
				}
				submqtt.setMaxMessagesToKeep(1000, 200);
				submqtt.setup("$SharedSubscription/" +prefix + "Sub/"+ topic2);					
				mqttConsumerList.add(submqtt);
				new Thread(submqtt).start();
			}
			
			for(int i=0; i<5; i++) // jms non durable
			{
				MultiJmsConsumerClient subjms = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				subjms.setAckMode(Session.AUTO_ACKNOWLEDGE, 1);
				if(i%3 == 0)
				{
					subjms.numberClients(3);
					subjms.disconnect(10000);
				}
				else if(i%4 == 0)
				{
					subjms.numberClients(3);
					subjms.disconnect(20000);
				}
				else if(i%5 == 0)
				{
					subjms.numberClients(3);
					subjms.unsubscribe(30000, false);
				}
				else
				{
					subjms.numberClients(3);
				}
				subjms.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB); 
				subjms.setSharedSubscription(prefix + "Sub", true);
				subjms.setMaxMessagesToKeep(1000, 200);
				if(traceClients)
				{
					subjms.setTraceFile(topic2 + "_SubJmsA" + i);
				}
				subjms.setup(null, null, DESTINATION_TYPE.TOPIC, topic2);				
				jmsMultiConsumerList.add(subjms);
				new Thread(subjms).start();
			}
			
			for(int i=0; i<5; i++) // jms durable
			{
				MultiJmsConsumerClient subjms = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				subjms.setAckMode(Session.AUTO_ACKNOWLEDGE, 1);
				subjms.setDurable();
				if(i%3 == 0)
				{
					subjms.numberClients(3);
					subjms.disconnect(10000);
				}
				else if(i%4 == 0)
				{
					subjms.numberClients(3);
					subjms.disconnect(20000);
				}
				else if(i%5 == 0)
				{
					subjms.numberClients(3);
					subjms.unsubscribe(30000, false);
				}
				else
				{
					subjms.numberClients(3);
				}
				subjms.checkMsgInterval(checkMsgIntervalVal, cliaddressA, cliaddressB); 
				subjms.setSharedSubscription(prefix + "Sub", true);
				subjms.setMaxMessagesToKeep(1000, 200);
				if(traceClients)
				{
					subjms.setTraceFile(topic2 + "_SubJmsB" + i);
				}
				subjms.setup(null, null, DESTINATION_TYPE.TOPIC, topic2);				
				jmsMultiConsumerList.add(subjms);
				new Thread(subjms).start();
			}
			
			
			for(int i=0; i<10; i++)
			{
	
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.stopOnPublishFailure();
				if(traceClients)
				{
					prod.setTraceFile(topic2 + "_ProdJms" + i);
				}
				prod.setup(null, null, topic2, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}

			
			for(int i=0; i<10; i++)
			{
	
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.stopOnPublishFailure();
				if(traceClients)
				{
					prod.setTraceFile(topic3 + "_ProdJms" + i);
				}
				prod.setup(null, null, topic3, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}

			
			for(int i=0; i<10; i++)
			{
	
				JmsProducerClient prod = new JmsProducerClient(ipaddress, new Integer(port));				
				prod.setNumberOfMessages(numberMessages);
				prod.setMessageInterval(messageInterval);
				prod.stopOnPublishFailure();
				if(traceClients)
				{
					prod.setTraceFile(topic1 + "_ProdJms" + i);
				}
				prod.setup(null, null, topic1, JmsMessage.DESTINATION_TYPE.TOPIC);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}

			for(int i=0; i<10; i++)
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
				prod.messageInterval(messageInterval);
				prod.setQos(2);
				if(traceClients)
				{
					prod.setTraceFile(topic1 + "_ProdMqttQOS2" + i);
				}
				prod.setUpClient(topic1);
				mqttProducerList.add(prod);
				new Thread(prod).start();
			}

			for(int i=0; i<10; i++)
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
				prod.messageInterval(messageInterval);
				prod.setQos(0);
				if(traceClients)
				{
					prod.setTraceFile(topic1 + "_ProdMqttQOS1" + i);
				}
				prod.setUpClient(topic1);
				mqttProducerList.add(prod);
				new Thread(prod).start();
			}
			
			File audit = new File("./errors/SendAndDiscardMessages.txt");
			PrintStream writer = null;
			
			// now keep going and check the clients at set intervals
			while(keepRunning)
			{
				Thread.sleep(180000);
				
				// now check we don't have a problem with any of the messages
				for(int i=0; i<jmsProducerList.size(); i++)
				{
					if(! jmsProducerList.get(i).getFailMessage().equals(""))
					{
						if(!audit.exists())
						{
							audit.createNewFile();
						}
						if(writer == null)
						{
							writer = new PrintStream("./errors/SendAndDiscardMessages.txt", "UTF-8");
						}
						writer.println(jmsProducerList.get(i).getFailMessage() + "\n");
						writer.flush();
					}
							
				}
				for(int i=0; i<mqttProducerList.size(); i++)
				{
					if(! mqttProducerList.get(i).getFailMessage().equals(""))
					{
						if(!audit.exists())
						{
							audit.createNewFile();
						}
						if(writer == null)
						{
							writer = new PrintStream("./errors/SendAndDiscardMessages.txt", "UTF-8");
						}
						writer.println(mqttProducerList.get(i).getFailMessage() + "\n");
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
							writer = new PrintStream("./errors/SendAndDiscardMessages.txt", "UTF-8");
						}
						writer.println(mqttConsumerList.get(i).getFailMessage() + "\n");
						writer.flush();
					}		
				}
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
							writer = new PrintStream("./errors/SendAndDiscardMessages.txt", "UTF-8");
						}
						writer.println(jmsConsumerList.get(i).getFailMessage() + "\n");
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
					Thread.sleep(60000); // Want to see what other messages we get after this so keep going for 1 minutes
				}
				else
				{
					System.out.println("Publishers and consumers have not reported any errors");
					
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
			for(int i=0; i<mqttProducerList.size(); i++)
			{
				mqttProducerList.get(i).keepRunning = false;
			}
			for(int i=0; i<mqttConsumerList.size(); i++)
			{
				mqttConsumerList.get(i).keepRunning = false;
			}
			for(int i=0; i<jmsConsumerList.size(); i++)
			{
				jmsConsumerList.get(i).keepRunning = false;
			}
			for(int i=0; i<jmsMultiConsumerList.size(); i++)
			{
				jmsMultiConsumerList.get(i).keepRunning = false;
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
				Trace.trace("SendAndDiscardMessages STOPPED");
			}
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
	}
	
	
	
	

}
