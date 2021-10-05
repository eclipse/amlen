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

import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.config.ImaConfig.TYPE;
import com.ibm.ima.test.cli.monitor.ImaMonitoring;
import com.ibm.ims.svt.clients.JmsMessage;
import com.ibm.ims.svt.clients.MultiJmsConsumerClient;
import com.ibm.ims.svt.clients.MultiJmsProducerClient;
import com.ibm.ims.svt.clients.Trace;

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
public class SendMessagesToQueues implements Runnable {

	private String port = null; // default but each class can use a different one
	private String ipaddress = null;
	private String prefix = "";
	private String queue1 = "QUEUE1";
	private String queue2 = "QUEUE2";
	private String queue3 = "QUEUE3";
	private String queue4 = "QUEUE4";
	private String queue5 = "QUEUE5";
	private String queue6 = "QUEUE6";
	private String queue7 = "QUEUE7";
	private String queue8 = "QUEUE8";
	
	private String messageHub = "SVTRegressionQueueHub";
	private String connectionPolicy = "SVTRegressionQueueCP";
	private String messagingPolicy = "SVTRegressionQueueMP";
	private String endpoint = "SVTRegressionQueueEP";
	
	private ImaConfig config = null;
	private ImaMonitoring monitor = null;
	
	private ArrayList<String> queueList = new ArrayList<String>();
	private ArrayList<MultiJmsProducerClient> jmsProducerList = new ArrayList<MultiJmsProducerClient>();
	private ArrayList<MultiJmsConsumerClient> jmsConsumerList = new ArrayList<MultiJmsConsumerClient>();
	
	private int numberClients = 3;
	private int pauseInterval = 1000;
	private boolean teardown = true;
	
	private boolean traceClients = false;

 	
	public boolean keepRunning = true;	
		
	public SendMessagesToQueues(String prefix, String ipaddress, String port, String cliaddress, boolean teardown, boolean traceClients) throws Exception
	{
		this.ipaddress = ipaddress;
		this.port = port;
		this.prefix = prefix;
		config = new ImaConfig(cliaddress, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddress, "admin", "admin");
		monitor.connectToServer();
		this.teardown = teardown;
		this.traceClients = traceClients;
		this.setup();
	}
	
	public SendMessagesToQueues(String prefix, String ipaddress, String port, String cliaddressA, String cliaddressB, boolean teardown, boolean traceClients) throws Exception
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
			 * no '+' exists so must be using ',' which is fine. assuming ',' is being used
			 */
			this.ipaddress = ipaddress;
		}
		
		this.port = port;
		this.prefix = prefix;
		config = new ImaConfig(cliaddressA, cliaddressB, "admin", "admin");
		config.connectToServer();
		monitor = new ImaMonitoring(cliaddressA, cliaddressB, "admin", "admin");
		monitor.connectToServer();
		this.teardown = teardown;
		this.traceClients = traceClients;
		this.setup();
	}
	
	private void setup() throws Exception
	{
		queue1=prefix+queue1;
		queue2=prefix+queue2;
		queue3=prefix+queue3;
		queue4=prefix+queue4;
		queue5=prefix+queue5;
		queue6=prefix+queue6;
		queue7=prefix+queue7;
		queue8=prefix+queue8;
		queueList.add(queue1);
		queueList.add(queue2);
		queueList.add(queue3);
		queueList.add(queue4);
		queueList.add(queue5);
		queueList.add(queue6);
		queueList.add(queue7);
		queueList.add(queue8);		
		
		for(int i=0; i<queueList.size(); i++)
		{
			if(teardown)
			{
				if(config.queueExists(queueList.get(i)))
				{
					config.deleteQueue(queueList.get(i), true);
				}
				config.createQueue(queueList.get(i), "SVT Regression queue", 20000000);
			}
		}
		
		if(config.endpointExists(endpoint))
		{
			config.deleteEndpoint(endpoint);
		}
		if(config.connectionPolicyExists(connectionPolicy))
		{
			config.deleteConnectionPolicy(connectionPolicy);
		}
		if(config.messagingPolicyExists(messagingPolicy))
		{
			config.deleteMessagingPolicy(messagingPolicy);
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
			config.createConnectionPolicy(connectionPolicy, connectionPolicy, TYPE.JMS);
		}
		if(! config.messagingPolicyExists(messagingPolicy))
		{
			config.createMessagingPolicy(messagingPolicy, messagingPolicy, "*", TYPE.Queue, TYPE.JMS);
		}
		if(! config.endpointExists(endpoint))
		{
		config.createEndpoint(endpoint, endpoint, port, TYPE.JMS, connectionPolicy, messagingPolicy, messageHub);
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

	public void run() {
		// instantiate trace that uses systemproperty MyTraceFile MyTrace
		Trace testtracer = new Trace();
		try
		{
			
			//OK now we need to set the producers and consumers going
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the consumers for " + queue1);
			}
			// Set up selectors ack consumers and producers
			
			for(int i=0; i<numberClients; i++) 
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.CLIENT_ACKNOWLEDGE, 0);
				sub.setSelector("Food = 'Chips'");
				sub.numberClients(5);
				sub.useSameConnection();
				if(traceClients)
				{
					sub.setTraceFile(queue1 + "_ASub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, queue1);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			for(int i=0; i<numberClients; i++) 
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.CLIENT_ACKNOWLEDGE, 4);
				sub.setSelector("Food = 'Steak'");
				sub.numberClients(5);
				sub.useSameConnection();
				if(traceClients)
				{
					sub.setTraceFile(queue1 + "_BSub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, queue1);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + queue1);
			}
			HashMap<String, String> properties = new HashMap<String, String>();
			properties.put("Food", "Chips");
			
			for(int i=0; i<numberClients; i++) 
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.numberClients(5);
				prod.useSameConnection();
				prod.setHeaderProperties(properties);
				if(traceClients)
				{
					prod.setTraceFile(queue1 + "_AProd" + i);
				}
				prod.setup(null, null, queue1, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			}
			properties = new HashMap<String, String>();
			properties.put("Food", "Steak");
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.useSameConnection();
				prod.setMessageInterval(pauseInterval);
				prod.numberClients(5);
				prod.setNumberOfMessages(2147479999);
				prod.setHeaderProperties(properties);
				if(traceClients)
				{
					prod.setTraceFile(queue1 + "_BProd" + i);
				}
				prod.setup(null, null, queue1, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the consumers for " + queue2);
			}
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.CLIENT_ACKNOWLEDGE, 0);
				sub.setSelector("Food = 'Steak'");
				sub.numberClients(5);
				sub.useSameConnection();
				if(traceClients)
				{
					sub.setTraceFile(queue2 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, queue2);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the consumers for " + queue3);
			}
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.CLIENT_ACKNOWLEDGE, 4);
				sub.numberClients(5);
				sub.useSameConnection();
				if(traceClients)
				{
					sub.setTraceFile(queue3 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, queue3);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
				
			}
			properties = new HashMap<String, String>();
			properties.put("Food", "Steak");
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + queue3);
			}
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.numberClients(5);
				prod.useSameConnection();
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.setHeaderProperties(properties);
				if(traceClients)
				{
					prod.setTraceFile(queue2 + "_Prod" + i);
				}
				prod.setup(null, null, queue2, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			}
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the producers for " + queue3);
			}
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.numberClients(5);
				prod.useSameConnection();
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				if(traceClients)
				{
					prod.setTraceFile(queue3 + "_Prod" + i);
				}
				prod.setup(null, null, queue3, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the consumers for " + queue4);
			}
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, 5);
				sub.setTransactedSession();
				sub.numberClients(5);
				sub.useSameConnection();
				sub.setSelector("Food = 'Chips'");
				if(traceClients)
				{
					sub.setTraceFile(queue4 + "_ASub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, queue4);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
				
			}
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, -1);
				sub.setTransactedSession();
				sub.numberClients(5);
				sub.useSameConnection();
				sub.setSelector("Food = 'Steak'");
				if(traceClients)
				{
					sub.setTraceFile(queue4 + "_BSub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, queue4);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			Thread.sleep(3000);
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + queue4);
			}
			properties = new HashMap<String, String>();
			properties.put("Food", "Chips");
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));			
				prod.sendPersistentMessages();
				prod.numberClients(5);
				prod.setMessageInterval(pauseInterval);
				prod.useSameConnection();
				prod.setNumberOfMessages(2147479999);
				prod.setHeaderProperties(properties);
				prod.setTransactedSession(1);
				if(traceClients)
				{
					prod.setTraceFile(queue4 + "_AProd" + i);
				}
				prod.setup(null, null, queue4, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			properties = new HashMap<String, String>();
			properties.put("Food", "Steak");
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.numberClients(5);
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.setHeaderProperties(properties);
				prod.useSameConnection();
				prod.setTransactedSession(4);
				if(traceClients)
				{
					prod.setTraceFile(queue4 + "_BProd" + i);
				}
				prod.setup(null, null, queue4, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the consumers for " + queue5);
			}
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, -1);
				sub.setTransactedSession();
				sub.numberClients(5);
				sub.useSameConnection();
				if(traceClients)
				{
					sub.setTraceFile(queue5 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, queue5);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			
			}
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the consumers for " + queue6);
			}
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, 5);
				sub.setTransactedSession();
				sub.numberClients(5);
				sub.useSameConnection();
				if(traceClients)
				{
					sub.setTraceFile(queue6 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, queue6);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			}
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + queue5);
			}
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.numberClients(5);
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.useSameConnection();
				prod.setTransactedSession(4);
				if(traceClients)
				{
					prod.setTraceFile(queue5 + "_Prod" + i);
				}
				prod.setup(null, null, queue5, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
			
			}
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + queue6);
			}
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));				
				prod.sendPersistentMessages();
				prod.numberClients(5);
				prod.setMessageInterval(pauseInterval);
				prod.useSameConnection();
				prod.setNumberOfMessages(2147479999);
				prod.setTransactedSession(4);
				if(traceClients)
				{
					prod.setTraceFile(queue6 + "_Prod" + i);
				}
				prod.setup(null, null, queue6, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the consumers for " + queue7);
			}
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, -1);
				sub.setTransactedSession();
				sub.useSameConnection();
				sub.numberClients(5);
				if(traceClients)
				{
					sub.setTraceFile(queue7 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, queue7);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + queue7);
			}
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));				
				prod.numberClients(5);
				prod.setMessageInterval(pauseInterval);
				prod.setNumberOfMessages(2147479999);
				prod.useSameConnection();
				prod.setTransactedSession(4);
				if(traceClients)
				{
					prod.setTraceFile(queue7 + "_Prod" + i);
				}
				prod.setup(null, null, queue7, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Starting all the consumers for " + queue8);
			}
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsConsumerClient sub = new MultiJmsConsumerClient(ipaddress, new Integer(port));
				sub.setAckMode(javax.jms.Session.AUTO_ACKNOWLEDGE, 10);
				sub.setSelector("Food = 'Steak'");
				sub.setTransactedSession();
				sub.useSameConnection();
				sub.numberClients(5);
				if(traceClients)
				{
					sub.setTraceFile(queue8 + "_Sub" + i);
				}
				sub.setup(null, null, JmsMessage.DESTINATION_TYPE.QUEUE, queue8);
				jmsConsumerList.add(sub);
				new Thread(sub).start();
			
			}
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("Creating all the producers for " + queue8);
			}
			properties = new HashMap<String, String>();
			properties.put("Food", "Steak");
			for(int i=0; i<numberClients; i++)
			{
				MultiJmsProducerClient prod = new MultiJmsProducerClient(ipaddress, new Integer(port));
				prod.setHeaderProperties(properties);
				prod.numberClients(5);
				prod.setMessageInterval(pauseInterval);
				prod.useSameConnection();
				prod.setNumberOfMessages(2147479999);
				if(traceClients)
				{
					prod.setTraceFile(queue8 + "_Prod" + i);
				}
				prod.setup(null, null, queue8, JmsMessage.DESTINATION_TYPE.QUEUE);
				jmsProducerList.add(prod);
				new Thread(prod).start();
				
			}
			
			while(keepRunning)
			{
				Thread.sleep(360000);
			}
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("SendMessagesToQueues Thread has been asked to stop so cleaning up all consumers and producers");
			}
			for(int i=0; i<jmsProducerList.size(); i++)
			{
				jmsProducerList.get(i).keepRunning = false;
			}
			for(int i=0; i<jmsConsumerList.size(); i++)
			{
				jmsConsumerList.get(i).keepRunning = false;
			}
			
			// sleep for 1 min before trying to delete everything
			Thread.sleep(60000);
			this.tearDown();
			
			if(traceClients && Trace.traceOn())
			{
				Trace.trace("SendMessagesToQueues STOPPED");
			}
			
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
	
	}
}
