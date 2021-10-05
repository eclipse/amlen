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
package com.ibm.ima.test.cli.monitor;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.ibm.ima.test.cli.imaserver.ImaCliHelper;
import com.ibm.ima.test.cli.imaserver.ImaCommandReference;

/**
 * 
 * NOTE: JSCH LIBRARIES ONLY TO BE USED FOR TESTING AND AUTOMATION
 * 
 * This class contains helper methods to return monitoring statistics back to the user in an easy to 
 * digest format. 
 * 
 */
public class ImaMonitoring extends ImaCliHelper{
	
	
	private String getQueueStat = null;
	private String getQueueStat_Q = null;
	private String getConnStat = null;
	private String getConnStat_EP = null;
	private String noDataFound = null;
	private String getTopicStat = null;
	private String getTopicStat_T = null;
	private String getMQTTStat = null;
	private String getMQTTStat_C = null;
	private String getEndpointStat = null;
	private String getEndpointStat_EP = null;
	private String getMappingStat = null;
	private String getMappingStat_RL = null;
	private String getSubStat = null;
	private String getSubStat_T = null;
	private String getSubStat_C = null;
	private String getStoreStat = null;
	private String getMemoryStat = null;
	private String getServerStat = null;
	private String createTopicMonitor = null;
	private String deleteTopicMonitor = null;
	private String showCommand = null;
	private String version100RegEx = "(.*)(version is 1.0.0)(.*)";
	private String version110RegEx = "(.*)(version is 1.1)(.*)"; // Any 1.1 minor version
	private String version120RegEx = "(.*)(version is 1.2)(.*)"; // Any 1.2 minor version

	private String noMonitoringData = "No monitoring data is found for the specified command";
	
	
	public ImaMonitoring(String ipaddress, String username, String password) throws Exception
	{
		super(ipaddress, username, password);
		
		getQueueStat = ImaCommandReference.QUEUE_STAT;
		getQueueStat_Q = ImaCommandReference.QUEUE_STAT_Q;
		getConnStat = ImaCommandReference.CONN_STAT;
		getConnStat_EP = ImaCommandReference.CONN_STAT_EP;
		noDataFound = ImaCommandReference.NO_STAT_FOUND;
		getTopicStat = ImaCommandReference.TOPIC_STAT;
		getTopicStat_T =  ImaCommandReference.TOPIC_STAT_T;
		getMQTTStat = ImaCommandReference.MQTT_STAT;
		getMQTTStat_C =  ImaCommandReference.MQTT_STAT_C;
		getEndpointStat = ImaCommandReference.EP_STAT;
		getEndpointStat_EP =  ImaCommandReference.EP_STAT_E;
		getMappingStat = ImaCommandReference.MAPPING_STAT;
		getMappingStat_RL =  ImaCommandReference.MAPPING_STAT_RL;
		getSubStat = ImaCommandReference.SUB_STAT;
		getSubStat_T =  ImaCommandReference.SUB_STAT_T;
		getSubStat_C =  ImaCommandReference.SUB_STAT_C;
		getStoreStat =  ImaCommandReference.STORE_STAT;
		getMemoryStat =  ImaCommandReference.MEMORY_STAT;
		getServerStat =  ImaCommandReference.SERVER_STAT;
		createTopicMonitor = ImaCommandReference.CREATE_TOPICMON;
		deleteTopicMonitor = ImaCommandReference.DELETE_TOPICMON;
		showCommand = ImaCommandReference.SHOW_SERVER_COMMAND;
	}
	
	public ImaMonitoring(String ipaddressA, String ipaddressB,  String username, String password) throws Exception
	{
		super(ipaddressA, ipaddressB, username, password);
		
		getQueueStat = ImaCommandReference.QUEUE_STAT;
		getQueueStat_Q = ImaCommandReference.QUEUE_STAT_Q;
		getConnStat = ImaCommandReference.CONN_STAT;
		getConnStat_EP = ImaCommandReference.CONN_STAT_EP;
		noDataFound = ImaCommandReference.NO_STAT_FOUND;
		getTopicStat = ImaCommandReference.TOPIC_STAT;
		getTopicStat_T =  ImaCommandReference.TOPIC_STAT_T;
		getMQTTStat = ImaCommandReference.MQTT_STAT;
		getMQTTStat_C =  ImaCommandReference.MQTT_STAT_C;
		getEndpointStat = ImaCommandReference.EP_STAT;
		getEndpointStat_EP =  ImaCommandReference.EP_STAT_E;
		getMappingStat = ImaCommandReference.MAPPING_STAT;
		getMappingStat_RL =  ImaCommandReference.MAPPING_STAT_RL;
		getSubStat = ImaCommandReference.SUB_STAT;
		getSubStat_T =  ImaCommandReference.SUB_STAT_T;
		getSubStat_C =  ImaCommandReference.SUB_STAT_C;
		getStoreStat =  ImaCommandReference.STORE_STAT;
		getMemoryStat =  ImaCommandReference.MEMORY_STAT;
		getServerStat =  ImaCommandReference.SERVER_STAT;
		createTopicMonitor = ImaCommandReference.CREATE_TOPICMON;
		deleteTopicMonitor = ImaCommandReference.DELETE_TOPICMON;
		showCommand = ImaCommandReference.SHOW_SERVER_COMMAND;
	}
	
	
	/**
	 * This method returns a HashMap of QueueStatResults for the specified queue or all
	 * the queues if the parameter name is null; The hashmap key is the queueName.
	 * 
	 * @param name of the queue or null if all queues are to be returned
	 * @return HashMap of all the queue stats
	 * @exception is thrown if the command could not be executed
	 */
	public HashMap<String, QueueStatResult> getQueueStatistics(String name) throws Exception
	{
		isConnectedToServer();
		
		String command = null;
		if(name == null)
		{
			command = getQueueStat;
		}
		else
		{
			command = getQueueStat_Q.replace("{NAME}", name);
		}
		HashMap<String, QueueStatResult> mainMap = new HashMap<String, QueueStatResult>();
		
		String result = sshExecutor.exec(command, timeout);
		
		
		if(isVersion100() || isVersion110())
		{
			String[] elements = result.trim().split(",", -1);
			
			for(int i=10; i<elements.length-1; i++)			
			{
				String qname = removeQuotes(elements[i]);		
				int producers = new Integer(removeQuotes(elements[++i]));	
				int consumers = new Integer(removeQuotes(elements[++i]));	
				int bufferredMsgs = new Integer(removeQuotes(elements[++i]));	
				int bufferedMsgsHWM = new Integer(removeQuotes(elements[++i]));	
				double bufferedPercent = new Double(removeQuotes(elements[++i]));	
				int maxMessages = new Integer(removeQuotes(elements[++i]));	
				int producedMsgs = new Integer(removeQuotes(elements[++i]));	
				int consumedMsgs = new Integer(removeQuotes(elements[++i]));	
				int rejectedMsgs = new Integer(removeQuotes(elements[++i]));
				
				QueueStatResult queueStat = new QueueStatResult(qname, producers, consumers, bufferredMsgs, bufferedMsgsHWM, 
					bufferedPercent, maxMessages, producedMsgs, consumedMsgs, rejectedMsgs, 0.0);
								mainMap.put(qname, queueStat);
			}
		}
		else
		{
			String[] elements = result.trim().split(",", -1);
		
			for(int i=11; i<elements.length-1; i++)			
			{
				String qname = removeQuotes(elements[i]);		
				int producers = new Integer(removeQuotes(elements[++i]));	
				int consumers = new Integer(removeQuotes(elements[++i]));	
				int bufferredMsgs = new Integer(removeQuotes(elements[++i]));	
				int bufferedMsgsHWM = new Integer(removeQuotes(elements[++i]));	
				double bufferedPercent = new Double(removeQuotes(elements[++i]));	
				int maxMessages = new Integer(removeQuotes(elements[++i]));	
				int producedMsgs = new Integer(removeQuotes(elements[++i]));	
				int consumedMsgs = new Integer(removeQuotes(elements[++i]));	
				int rejectedMsgs = new Integer(removeQuotes(elements[++i]));
				double bufferedHWMPercent = new Double(removeQuotes(elements[++i]));
				
				QueueStatResult queueStat = new QueueStatResult(qname, producers, consumers, bufferredMsgs, bufferedMsgsHWM, 
					bufferedPercent, maxMessages, producedMsgs, consumedMsgs, rejectedMsgs, bufferedHWMPercent);
		
				mainMap.put(qname, queueStat);
			}
		}
			
		return mainMap;
	}
	
	
	
	/**
	 * This method returns a Hashmap containing TopicStatResults for each TopicString matched. If null
	 * is passed in then we will return all TopicStatResults
	 * @param topicString - name of the topic String
	 * 
	 * @return HashMap containing the TopicStatResults with topic string as the map key
	 * @throws Exception
	 */
	public HashMap<String, TopicStatResult> getTopicStatistics(String topicString) throws Exception
	{
		isConnectedToServer();
		String command = null;
		if(topicString == null)
		{
			command = getTopicStat;
		}
		command = getTopicStat_T.replace("{NAME}", topicString);
		
		HashMap<String, TopicStatResult> mainMap = new HashMap<String, TopicStatResult>();
		
		String result = sshExecutor.exec(command, timeout);
		
		String[] elements = result.split(",", -1);
		
		for(int i=6; i<elements.length-1; i++)
		{
			String topic = removeQuotes(elements[i]);
			int subscriptions = new Integer(removeQuotes(elements[++i]));
			DateFormat dfm = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
			Date resetTime = dfm.parse(removeQuotes(elements[++i]));
			int publishedMsgs = new Integer(removeQuotes(elements[++i]));
			int rejectedMsgs = new Integer(removeQuotes(elements[++i]));
			int failedPublishes = new Integer(removeQuotes(elements[++i]));

			TopicStatResult topicStat = new TopicStatResult(topic, subscriptions, resetTime, publishedMsgs, rejectedMsgs, failedPublishes);
				
			mainMap.put(topic, topicStat);
		}
			
		return mainMap;
	}
		
	
	/**
	 * This method returns a HashMap which is key'ed off the name of the connection containing
	 * the ConnectionStatResult.
	 * 
	 * @param endpoint - the name of the endpoint or null if you want information for all
	 * @return HashMap containing ConnectionStatResults (name of connection is the  map key). If the
	 * hashmap contains no entries then no connection data was found.
	 * 
	 * @throws Exception
	 */
	public HashMap<String, ConnectionStatResult> getConnectionStatistics(String endpoint) throws Exception
	{
		isConnectedToServer();
		String command = null;
		
		if(endpoint == null)
		{
			command = getConnStat;
		}
		else
		{
			command = getConnStat_EP.replace("{NAME}", endpoint);
		}

		HashMap<String, ConnectionStatResult> mainMap = new HashMap<String, ConnectionStatResult>();

		String result = null;
		try
		{
			result = sshExecutor.exec(command, timeout); 
		}
		catch(Exception e)
		{
			String objNotFound = noDataFound.replace("{NAME}", "connection");
			if(e.getMessage().contains(objNotFound)|| e.getMessage().contains(noMonitoringData))
			{
				return null;
			}
			else
			{
				throw e;
			}
		}
		
		String[] elements = result.split(",", -1);
		
		for(int i=12; i<elements.length-1; i++)
		{
			String connectionName = removeQuotes(elements[i]);
			String protocol = removeQuotes(elements[++i]);
			String clientAddr = removeQuotes(elements[++i]);
			String userId = removeQuotes(elements[++i]);
			String endpointRet = removeQuotes(elements[++i]);
			int port = new Integer(removeQuotes(elements[++i]));
			DateFormat dfm = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
			Date connectTime = dfm.parse(removeQuotes(elements[++i]));
			long duration = new Long(removeQuotes(elements[++i]));
			long readBytes = new Long(removeQuotes(elements[++i]));
			long readMsg = new Long(removeQuotes(elements[++i]));
			long writebytes = new Long(removeQuotes(elements[++i]));
			long writeMsg = new Long(removeQuotes(elements[++i]));
			
			ConnectionStatResult connResult = new ConnectionStatResult(connectionName, protocol, clientAddr, userId,
						endpointRet, port, connectTime, duration, readBytes, readMsg, writebytes, writeMsg);
			mainMap.put(connectionName, connResult);
		}
	
	
		return mainMap;
	}
	
	
	
	
	/**
	 * This method returns a Hashmap for the MQTTStatResult for MQTTClients.
	 * @param clientID name of the ClientID, if this is null we will return information
	 * for all clients
	 * @return HashMap containing the MQTTStatResult with clientID as the key - an empty hashmap
	 * is returned if there are not result for that clientID.
	 * @throws Exception
	 */
	public HashMap<String, MqttStatResult> getMQTTStatistics(String clientID) throws Exception
	{
		isConnectedToServer();
		String command = null;
		if(clientID == null)
		{
			command = getMQTTStat;
		}
		else
		{
			command = getMQTTStat_C.replace("{NAME}", clientID);
		}
		HashMap<String, MqttStatResult> mainMap = new HashMap<String, MqttStatResult>();
		
	
		String result = sshExecutor.exec(command, timeout);
		
		String[] elements = result.trim().split(",", -1);
		
		for(int i=3; i<elements.length-1; i++)
		{
			String id = removeQuotes(elements[i]);
			
			boolean isConnected = new Boolean(removeQuotes(elements[++i]));
			DateFormat dfm = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
			String dateElement = removeQuotes(elements[++i]);
			Date lastConnectedTime = dfm.parse(dateElement);
			
			MqttStatResult mqttResult = new MqttStatResult(id, isConnected, lastConnectedTime);
			mainMap.put(id, mqttResult);
		
		}
	
		return mainMap;
	}
	
	
	
	/**
	 * This method returns a Hashmap for the EndpointStatResult for Endpoint (current).
	 * @param endpoint name of the Endpoint, if this is null we will return information
	 * for all Endpoints
	 * @return HashMap containing the EndpointStatResult with endpoint name as the key. If no data is 
	 * found the hashmap will be empty.
	 * @throws Exception
	 */
	public HashMap<String, EndpointStatResult> getEndpointStatistics(String endpoint) throws Exception
	{
		isConnectedToServer();
		String command = null;
		if(endpoint == null)
		{
			command = getEndpointStat;
		}
		else
		{
			command = getEndpointStat_EP.replace("{NAME}", endpoint);
		}
		HashMap<String, EndpointStatResult> mainMap = new HashMap<String, EndpointStatResult>();

		String result = sshExecutor.exec(command, timeout);
		
		String[] elements = result.trim().split(",", -1);
					
		for(int i=10; i<elements.length-1; i++)
		{
			String name = removeQuotes(elements[i]);
			String ipAddr = removeQuotes(elements[++i]);
			boolean enabled = new Boolean(removeQuotes(elements[++i]));
			int total = new Integer(removeQuotes(elements[++i]));
			int active = new Integer(removeQuotes(elements[++i]));
			int messages = new Integer(removeQuotes(elements[++i]));
			long bytes = new Long(removeQuotes(elements[++i]));	
			String lastErrorCode = removeQuotes(elements[++i]);
			DateFormat dfm = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
			Date configTime = dfm.parse(removeQuotes(elements[++i]));
			int badConnections = new Integer(removeQuotes(elements[++i]));	
		
			EndpointStatResult endpointResult = new EndpointStatResult(name, ipAddr, enabled, total,
					active, messages, bytes, lastErrorCode, configTime, badConnections);
			mainMap.put(name, endpointResult);
		}
		
		return mainMap;
	}
	
	
	
	/**
	 * This method returns a Hashmap for the DestMappingRuleStatResult for a specified DestinationMappingRule.
	 * @param ruleName of the Rule, if this is null we will return information
	 * for all Rules
	 * @return HashMap containing the DestMappingRuleStatResulta with the rulename as the key. If the hashmap
	 * is empty then no results were found
	 * @throws Exception
	 */
	public HashMap<String, DestMappingRuleStatResult> getMappingStatistics(String ruleName) throws Exception
	{
		isConnectedToServer();
		String command = null;
		if(ruleName == null)
		{
			command = getMappingStat;
		}
		else
		{
			command = getMappingStat_RL.replace("{NAME}", ruleName);
		}
		HashMap<String, DestMappingRuleStatResult> mainMap = new HashMap<String, DestMappingRuleStatResult>();
			
		String result = sshExecutor.exec(command, timeout);
		
		String[] elements = result.trim().split(",", -1);
					
		for(int i=9; i<elements.length-1; i++)
		{
		
			String name = removeQuotes(elements[i]);				
			String queueManagerConnection = removeQuotes(elements[++i]);
			int commitCount = new Integer(removeQuotes(elements[++i]));	
			int rollbackCount = new Integer(removeQuotes(elements[++i]));
			int committedMessageCount = new Integer(removeQuotes(elements[++i]));
			int persistentCount = new Integer(removeQuotes(elements[++i]));
			int nonpersistentCount = new Integer(removeQuotes(elements[++i]));
			int status = new Integer(removeQuotes(elements[++i]));
			String expandedMessage = removeQuotes(elements[++i]);
		
			DestMappingRuleStatResult mapResult = new DestMappingRuleStatResult(name, queueManagerConnection, commitCount,
					rollbackCount, committedMessageCount, persistentCount, nonpersistentCount,
				 status, expandedMessage);
			
			mainMap.put(name, mapResult);
		}
				
		return mainMap;
	}
	

	/**
	 * This method returns a Hashmap of SubscriptionStatResult for a particular Subscription.
	 * @param topicString, if this is null we will return information
	 * for all Subscriptions
	 * @return HashMap containing the name-value pairs with clientID as the key
	 * @throws Exception
	 */
	public HashMap<String, SubscriptionStatResult> getSubStatisticsByTopic(String topicString) throws Exception
	{
		isConnectedToServer();
		String command = null;
		if(topicString == null)
		{
			command = getSubStat;
		}
		else
		{
			command = getSubStat_T.replace("{NAME}", topicString);
		}
		HashMap<String, SubscriptionStatResult> mainMap = new HashMap<String, SubscriptionStatResult>();
		
		String result = null;
	
		try
		{
			result = sshExecutor.exec(command, timeout);
		}
		catch(Exception e)
		{
			String objNotFound = noDataFound.replace("{NAME}", "subscription");
			if(e.getMessage().contains(objNotFound)|| e.getMessage().contains(noMonitoringData))
			{
				return mainMap;
			}
			else
			{
				throw e;
			}
		}
		
		
		if(isVersion100())
		{
			String[] elements = result.trim().split(",", -1);
			
			for(int i=10; i<elements.length-1; i++)
			{
				
				String subName = (removeQuotes(elements[i]));		
				String topicStg = (removeQuotes(elements[++i]));
				String clientId = (removeQuotes(elements[++i]));
				boolean isDurable = new Boolean(removeQuotes(elements[++i]));
				int bufferedMsgs = new Integer(removeQuotes(elements[++i]));	
				int bufferedMsgsHWM = new Integer(removeQuotes(elements[++i]));;
				double bufferedPercent = new Double((removeQuotes(elements[++i])));
				int maxMessages = new Integer(removeQuotes(elements[++i]));
				int publishedMsgs = new Integer(removeQuotes(elements[++i]));
				int rejectedMsgs = new Integer(removeQuotes(elements[++i]));
			
				SubscriptionStatResult subResult = new SubscriptionStatResult(subName, topicStg, clientId, isDurable, bufferedMsgs,
						bufferedMsgsHWM, bufferedPercent, maxMessages, publishedMsgs, rejectedMsgs, 0.0, false, 0,0,0,null);
				mainMap.put(subName, subResult);
			}
		}				
		else if (isVersion110())
		{
			String[] elements = result.trim().split(",", -1);
		
			for(int i=13; i<elements.length-1; i++)
			{
			
				String subName = (removeQuotes(elements[i]));		
				String topicStg = (removeQuotes(elements[++i]));
				String clientId = (removeQuotes(elements[++i]));
				boolean isDurable = new Boolean(removeQuotes(elements[++i]));
				int bufferedMsgs = new Integer(removeQuotes(elements[++i]));	
				int bufferedMsgsHWM = new Integer(removeQuotes(elements[++i]));
				double bufferedPercent = new Double((removeQuotes(elements[++i])));
				int maxMessages = new Integer(removeQuotes(elements[++i]));
				int publishedMsgs = new Integer(removeQuotes(elements[++i]));
				int rejectedMsgs = new Integer(removeQuotes(elements[++i]));
				double bufferedHWMPercent = new Double((removeQuotes(elements[++i])));
				boolean isShared = new Boolean(removeQuotes(elements[++i]));
				int consumers = new Integer(removeQuotes(elements[++i]));
				

	
				SubscriptionStatResult subResult = new SubscriptionStatResult(subName, topicStg, clientId, isDurable, bufferedMsgs,
					bufferedMsgsHWM, bufferedPercent, maxMessages, publishedMsgs, rejectedMsgs, bufferedHWMPercent, isShared, consumers,0,0,null);
				mainMap.put(clientId, subResult);
			}
		}
		else if (isVersion120())
		{
			String[] elements = result.trim().split(",", -1);
	
			for(int i=16; i<elements.length-1; i++)
			{
				
				String subName = (removeQuotes(elements[i]));		
				String topicStg = (removeQuotes(elements[++i]));
				String clientId = (removeQuotes(elements[++i]));
				boolean isDurable = new Boolean(removeQuotes(elements[++i]));
				int bufferedMsgs = new Integer(removeQuotes(elements[++i]));	
				int bufferedMsgsHWM = new Integer(removeQuotes(elements[++i]));
				double bufferedPercent = new Double((removeQuotes(elements[++i])));
				int maxMessages = new Integer(removeQuotes(elements[++i]));
				int publishedMsgs = new Integer(removeQuotes(elements[++i]));
				int rejectedMsgs = new Integer(removeQuotes(elements[++i]));
				double bufferedHWMPercent = new Double((removeQuotes(elements[++i])));
				boolean isShared = new Boolean(removeQuotes(elements[++i]));
				int consumers = new Integer(removeQuotes(elements[++i]));
				int discardedmsgs = new Integer(removeQuotes(elements[++i]));
				int expiredmsgs = new Integer(removeQuotes(elements[++i]));
				String messagingpolicy = (removeQuotes(elements[++i]));

		
				SubscriptionStatResult subResult = new SubscriptionStatResult(subName, topicStg, clientId, isDurable, bufferedMsgs,
						bufferedMsgsHWM, bufferedPercent, maxMessages, publishedMsgs, rejectedMsgs, bufferedHWMPercent, isShared, consumers,discardedmsgs,expiredmsgs,messagingpolicy);
				mainMap.put(clientId, subResult);
			}
		}
		
	
		
		return mainMap;
	}
	
	
	
	/**
	 * This method returns a Hashmap for the SubscriptionStatResult for a Subscription.
	 * @param clientID, if this is null we will return information
	 * for all Subscriptions
	 * @return HashMap containing the name-value pairs with SubName as the key
	 * @throws Exception
	 */
	public HashMap<String, SubscriptionStatResult> getSubStatisticsByClientID(String clientID) throws Exception
	{
		isConnectedToServer();
		String command = null;
		if(clientID == null)
		{
			command = getSubStat;
		}
		else
		{
			command = getSubStat_C.replace("{NAME}", clientID);
		}
		HashMap<String, SubscriptionStatResult> mainMap = new HashMap<String, SubscriptionStatResult>();
		

		String result = null;
		
		try
		{
			result = sshExecutor.exec(command, timeout);
		}
		catch(Exception e)
		{
			String objNotFound = noDataFound.replace("{NAME}", "subscription");
			if(e.getMessage().contains(objNotFound) || e.getMessage().contains(noMonitoringData))
			{
				return mainMap;
			}
			else
			{
				throw e;
			}
		}
		
		if(isVersion100() || isVersion110())
		{
			String[] elements = result.trim().split(",", -1);
				
			for(int i=10; i<elements.length-1; i++)
			{
					String subName =  removeQuotes(elements[i]);		
					String topicStg =  removeQuotes(elements[++i]);
					String clientId =  removeQuotes(elements[++i]);
					boolean isDurable = new Boolean(removeQuotes(elements[++i]));
					int bufferedMsgs = new Integer(removeQuotes(elements[++i]));	
					int bufferedMsgsHWM = new Integer(removeQuotes(elements[++i]));
					double bufferedPercent = new Double( removeQuotes((elements[++i])));
					int maxMessages = new Integer(removeQuotes(elements[++i]));
					int publishedMsgs = new Integer(removeQuotes(elements[++i]));
					int rejectedMsgs = new Integer(removeQuotes(elements[++i]));
					double bufferedHWMPercent = new Double((removeQuotes(elements[++i])));
					boolean isShared = new Boolean(removeQuotes(elements[++i]));
					int consumers = new Integer(removeQuotes(elements[++i]));
				
					SubscriptionStatResult subResult = new SubscriptionStatResult(subName, topicStg, clientId, isDurable, bufferedMsgs,
							bufferedMsgsHWM, bufferedPercent, maxMessages, publishedMsgs, rejectedMsgs,bufferedHWMPercent, isShared, consumers,0,0,null);
					mainMap.put(subName, subResult);
			}
		}
		else
		{
			String[] elements = result.trim().split(",", -1);
			for(int i=16; i<elements.length-1; i++)
			{
				String subName =  removeQuotes(elements[i]);		
				String topicStg =  removeQuotes(elements[++i]);
				String clientId =  removeQuotes(elements[++i]);
				boolean isDurable = new Boolean(removeQuotes(elements[++i]));
				int bufferedMsgs = new Integer(removeQuotes(elements[++i]));	
				int bufferedMsgsHWM = new Integer(removeQuotes(elements[++i]));
				double bufferedPercent = new Double( removeQuotes((elements[++i])));
				int maxMessages = new Integer(removeQuotes(elements[++i]));
				int publishedMsgs = new Integer(removeQuotes(elements[++i]));
				int rejectedMsgs = new Integer(removeQuotes(elements[++i]));
				double bufferedHWMPercent = new Double((removeQuotes(elements[++i])));
				boolean isShared = new Boolean(removeQuotes(elements[++i]));
				int consumers = new Integer(removeQuotes(elements[++i]));

				/*
				 * New stat columns for 2.0 below
				 */
				int discardedmsgs = new Integer(removeQuotes(elements[++i]));
				int expiredmsgs = new Integer(removeQuotes(elements[++i]));
				String messagingpolicy = (removeQuotes(elements[++i]));

				
					SubscriptionStatResult subResult = new SubscriptionStatResult(subName, topicStg, clientId, isDurable, bufferedMsgs,
							bufferedMsgsHWM, bufferedPercent, maxMessages, publishedMsgs, rejectedMsgs,bufferedHWMPercent, isShared, consumers,discardedmsgs,expiredmsgs,messagingpolicy);
					mainMap.put(subName, subResult);
			}
		}
				
		
				
		
		
		return mainMap;
	}
	
	/**
	 * This method returns a StoreStatResult for the Store statistics.
	 * 
	 * @return StoreStatResult
	 * @throws Exception
	 */
	public StoreStatResult getStoreStatistics() throws Exception
	{
		isConnectedToServer();
		String result = sshExecutor.exec(getStoreStat, timeout);
		StoreStatResult storeResult = null;
		
		String[] elements = result.trim().split(",", -1);
		double memoryUsedPercent = new Double((elements[0].split("="))[1].trim());
		double diskUsedPercent = new Double((elements[1].split("="))[1].trim());
		long diskFreeBytes = new Long((elements[2].split("="))[1].trim());
	
		storeResult = new StoreStatResult(memoryUsedPercent, diskUsedPercent, diskFreeBytes);
		
		return storeResult;
	}
	
	
	
	/**
	 * This method returns a MemoryStatResult for the Memory statistics.
	 * 
	 * @return MemoryStatResult
	 * @throws Exception
	 */
	public MemoryStatResult getMemoryStatistics() throws Exception
	{	
		isConnectedToServer();
		String result = sshExecutor.exec(getMemoryStat, timeout);
		MemoryStatResult memoryResult = null;
		
		String[] elements = result.trim().split(",", -1);
		long memoryTotalBytes = new Long((elements[0].split("="))[1].trim());
		long memoryFreeBytes = new Long((elements[1].split("="))[1].trim());
		double memoryFreePercent = new Double((elements[2].split("="))[1].trim());
		long serverVirtualMemoryBytes = new Long((elements[3].split("="))[1].trim());
		long serverResidentSetBytes = new Long((elements[4].split("="))[1].trim());
		long messagePayloads = new Long((elements[5].split("="))[1].trim());
		long publishSubscribe = new Long((elements[6].split("="))[1].trim());
		long destinations = new Long((elements[7].split("="))[1].trim());
		long currentActivity = new Long((elements[8].split("="))[1].trim());
			
		memoryResult = new  MemoryStatResult(memoryTotalBytes, memoryFreeBytes, memoryFreePercent, 
				 serverVirtualMemoryBytes, serverResidentSetBytes, messagePayloads,
				 publishSubscribe, destinations, currentActivity);
		
		return memoryResult;
	}
	
	/**
	 * This method returns a ServerStatResult for the Server statistics.
	 * 
	 * @return ServerStatResult
	 * @throws Exception
	 */
	public ServerStatResult getServerStatistics() throws Exception
	{
		isConnectedToServer();
		String result = sshExecutor.exec(getServerStat, timeout);
		ServerStatResult serverResult = null;
		
		String[] elements = result.trim().split(",", -1);
		int activeConnections = new Integer((elements[0].split("="))[1].trim());
		int totalConnections = new Integer((elements[1].split("="))[1].trim());
		long msgRead = new Long((elements[2].split("="))[1].trim());
		long msgWrite = new Long((elements[3].split("="))[1].trim());
		long bytesRead = new Long((elements[4].split("="))[1].trim());
		long bytesWrite = new Long((elements[5].split("="))[1].trim());
		int badConnCount = new Integer((elements[6].split("="))[1].trim());
		int totalEndpoints = new Integer((elements[7].split("="))[1].trim());		
		
	
		serverResult = new  ServerStatResult(activeConnections, totalConnections, msgRead, msgWrite,
				bytesRead, bytesWrite, badConnCount, totalEndpoints);
		
		return serverResult;
		
	}
	
	
	/**
	 * Method to create a topic monitor
	 * @param topic
	 * @throws Exception is thrown if the topic monitor could not be created
	 */
	public void createTopicMonitor(String topic) throws Exception
	{
		isConnectedToServer();
		String command = createTopicMonitor.replace("{TOPIC}", topic);
		sshExecutor.exec(command, timeout);
	}
	/**
	 * Method to delete a topic monitor
	 * @param topic
	 * @throws Exception is thrown if the topic monitor could not be deleted
	 */
	public void deleteTopicMonitor(String topic) throws Exception
	{
		isConnectedToServer();
		String command = deleteTopicMonitor.replace("{TOPIC}", topic);
		sshExecutor.exec(command, timeout);
	}
	
	private String removeQuotes(String value)
	{
		String temp = value;
		if(temp.startsWith("\"")){
			temp = temp.substring(1, temp.length()-1);
		}
		return temp;
	}
	
	private boolean isVersion100() throws Exception
	{
		isConnectedToServer();
		String result = sshExecutor.exec(showCommand, timeout);
		Pattern p = Pattern.compile(version100RegEx);
	    Matcher m = p.matcher(result); // get a matcher object
	    return m.find();
	}
	private boolean isVersion110() throws Exception
	{
		isConnectedToServer();
		String result = sshExecutor.exec(showCommand, timeout);
		Pattern p = Pattern.compile(version110RegEx);
	    Matcher m = p.matcher(result); // get a matcher object
	    return m.find();
	}
	private boolean isVersion120() throws Exception
	{
		isConnectedToServer();
		String result = sshExecutor.exec(showCommand, timeout);
		Pattern p = Pattern.compile(version120RegEx);
	    Matcher m = p.matcher(result); // get a matcher object
	    return m.find();
	}
}
