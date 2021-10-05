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

import java.io.IOException;
import java.util.Hashtable;

import com.ibm.mq.MQDestination;
import com.ibm.mq.MQException;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQPutMessageOptions;
import com.ibm.mq.MQQueue;
import com.ibm.mq.MQQueueManager;
import com.ibm.mq.MQTopic;
import com.ibm.mq.constants.CMQC;

/**
 * This class creates a connection to MQ using the base java classes
 */
public class MQConnection {

	private MQQueueManager mqQueueManager;
	private String hostName;
	private String queueManagerName;
	private String channel;
	private int port;

	/**
	 * This constructor creates a connection using the supplied parameters 
	 * It uses the hashtable version of the constructor which overrides the MQEnvironment
	 * This is because there can only be 1 instance of the MQEnvironment
	 * @param queueManagerName
	 * @param hostName
	 * @param port
	 * @param channel
	 * @throws MQException
	 */
	@SuppressWarnings("unchecked")
	public MQConnection(String queueManagerName, String hostName, int port, String channel) throws MQException {		
		this.channel =channel;
		this.port = port;
		this.hostName = hostName;
		this.queueManagerName = queueManagerName;
			Hashtable properties = new Hashtable();
			properties.put("hostname", hostName);
			properties.put("port", port);
			properties.put("channel", channel);
			mqQueueManager = new MQQueueManager(queueManagerName, properties);
	}
	/**
	 * creates a subscription on an MQ topic
	 * do not add the /# for wildcard subscriptions - this is added automatically
	 * uses CMQC.MQSO_CREATE
	 * if wildcard is true, adds CMQC.MQSO_WILDCARD_TOPIC to open options
	 * @param topicName
	 * @param wildcard
	 * @return
	 * @throws MQException
	 */
	public MQTopic createSubscription(String topicName, boolean wildcard) throws MQException {
		int options = CMQC.MQSO_CREATE;
		if (wildcard) {
			options = options | CMQC.MQSO_WILDCARD_TOPIC;
			topicName = topicName +"/#";
		}
		int openAs = CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION;
		MQTopic mqt =
			mqQueueManager.accessTopic(topicName, null,openAs, options);	
		return mqt;
	}
	/**
	 * creates a durable subscription on an MQ topic
	 * do not add the /# for wildcard subscriptions - this is added automatically
	 * uses CMQC.MQSO_CREATE | CMQC.MQSO_DURABLE | CMQC.MQSO_RESUME | CMQC.MQSO_ALTER
	 * if wildcard is true, adds CMQC.MQSO_WILDCARD_TOPIC to open options
	 * @param topicName
	 * @param wildcard
	 * @return
	 * @throws MQException
	 */
	public MQTopic createDurableSubscription(String topicName, boolean wildcard, String subscriptionName) throws MQException {
		int options = CMQC.MQSO_CREATE | CMQC.MQSO_DURABLE | CMQC.MQSO_RESUME | CMQC.MQSO_ALTER;
		if (wildcard) {
			options = options | CMQC.MQSO_WILDCARD_TOPIC;
			topicName = topicName +"/#";
		}	
		MQTopic mqt = mqQueueManager.accessTopic(topicName, null, options,null,subscriptionName);
		return mqt;
	}
	/**
	 * opens a topic for publication
	 * @param topicName - the topic to publish on
	 * @return MQTopic
	 * @throws MQException
	 */
	public MQTopic accessTopic (String topicName) throws MQException {
		MQTopic mqt = mqQueueManager.accessTopic(topicName, null,CMQC.MQTOPIC_OPEN_AS_PUBLICATION, CMQC.MQOO_OUTPUT);	
		return mqt;
	}
	
	/**
	 * creates a connection to an MQ Queue - does not check for existence of queue
	 * returns MQException if queue does not exist: "MQJE001: Completion Code '2', Reason '2085'."
	 * uses CMQC.MQOO_INPUT_SHARED as open options
	 * @param queueName - the name of the queue to open 
	 * @return
	 * @throws MQException
	 */
	public MQQueue getInputQueueConnection(String queueName) throws MQException {
		MQQueue mqq = mqQueueManager.accessQueue(queueName, CMQC.MQOO_INPUT_SHARED);
		return mqq;
	}
	/**
	 * creates a connection to an MQ Queue - does not check for existence of queue
	 * returns MQException if queue does not exist: "MQJE001: Completion Code '2', Reason '2085'."
	 * uses CMQC.MQOO_OUTPUT as open options
	 * @param queueName - the name of the queue to open 
	 * @return
	 * @throws MQException
	 */
	public MQQueue getOutputQueueConnection(String queueName) throws MQException {
		MQQueue mqq = mqQueueManager.accessQueue(queueName, CMQC.MQOO_OUTPUT);
		return mqq;
	}
	
	/**
	 * disconnect from queue manager
	 * @throws MQException
	 */
	public void disconnect () throws MQException {
		mqQueueManager.disconnect();
	}
	/**
	 * send message with payload messageData to MQ Destination mqDest with persistence messagePersistence
	 * @param mqDest
	 * @param messageData
	 * @param messagePersistence
	 * @throws MQException
	 */
	public void sendMessage(MQDestination mqDest, String messageData, int messagePersistence) throws MQException {
		MQMessage message = new MQMessage();
		byte[] byteArray = messageData.getBytes();
		try {
			message.write(byteArray);
		} catch (IOException e1) {
			e1.printStackTrace();
		}
		message.persistence = messagePersistence ;
		mqDest.put(message);
	}
	/*
	 * getters and setters
	 */
	public MQQueueManager getMqQueueManager() {
		return mqQueueManager;
	}

	public void setMqQueueManager(MQQueueManager mqQueueManager) {
		this.mqQueueManager = mqQueueManager;
	}

	public String getHostName() {
		return hostName;
	}

	public void setHostName(String hostName) {
		this.hostName = hostName;
	}

	public String getQueueManagerName() {
		return queueManagerName;
	}

	public void setQueueManagerName(String queueManagerName) {
		this.queueManagerName = queueManagerName;
	}

	public String getChannel() {
		return channel;
	}

	public void setChannel(String channel) {
		this.channel = channel;
	}

	public int getPort() {
		return port;
	}

	public void setPort(int port) {
		this.port = port;
	}
	
	public void sendRetainedMessage(MQDestination mqDest, String messageData, int messagePersistence) throws MQException {
			MQMessage message = new MQMessage();
			byte[] byteArray = messageData.getBytes();
			try {
				message.write(byteArray);
			} catch (IOException e1) {
				e1.printStackTrace();
			}
			message.persistence = messagePersistence;
			MQPutMessageOptions options = new MQPutMessageOptions();
	        options.options += CMQC.MQPMO_RETAIN;
	        mqDest.put(message, options);
		}
}
