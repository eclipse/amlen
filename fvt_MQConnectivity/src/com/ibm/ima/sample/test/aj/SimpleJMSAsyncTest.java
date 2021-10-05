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
package com.ibm.ima.sample.test.aj;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

public class SimpleJMSAsyncTest implements MessageListener
{
	private Connection connect1, connect2;
	private Session session1, session2;
	private MessageProducer producer1;
	private Topic topic;
	private MessageConsumer consumer1;

	private static int dontWaitForever = 0;
	private static int numberOfMsgs = 1000;
	private static boolean printAllMessages = false;
	private static int RC = 0;
	private static String prodTopicName = "";
	private static String consTopicName = "";

	private static int msgCount = 0;
		
	private static String imaIP = "";
	private static String imaPort = "";
	private static String prodClientID = "ClientAJ";
	private static String consClientID = "ClientHQ";
	public static Connection imaConnection1 = null;
	
	public static void main(String[] args) 
	{
		SimpleJMSAsyncTest simpleJMSAsyncTest = new SimpleJMSAsyncTest();
		
		if (args.length==0)
		{
			System.out.println("\nTest aborted, test requires command line arguments!! (IP address, port)");
			System.exit(1);
		}
		else if(args.length == 3)
		{
			imaIP = args[0];
			imaPort = args[1];
			prodTopicName = args[2];
			consTopicName = args[2];
		}
		else
		{
			System.out.println("\nInvalid Commandline arguments! System Exit.");
			System.exit(1);
		}
		System.out.println("Test Commenced using IMA IP: "+ imaIP +" and Port: "+imaPort);

		simpleJMSAsyncTest.createProducer();				
		simpleJMSAsyncTest.createConsumer();
		
		try
		{
			Thread.sleep(1000);
		}
		catch(InterruptedException e)
		{
			e.printStackTrace();
			RC = ReturnCodes.INTERRUPTED_SLEEP_FAILED;
		}
		simpleJMSAsyncTest.sendMessages();
		while (msgCount < numberOfMsgs)
		{
			
			try
			{
				dontWaitForever++;
				Thread.sleep(1000);
			}
			catch(InterruptedException e)
			{
				e.printStackTrace();
				RC = ReturnCodes.INTERRUPTED_SLEEP_FAILED;
			}
			if(dontWaitForever>=10)
			{
				System.out.println(".");
				RC = ReturnCodes.ERROR_NO_MSGS_FOR_10SECONDS_FAILED;
				break;
			}
		}
		if(msgCount==numberOfMsgs)
			System.out.println("All messages received, Total msg count: "+msgCount);
		else if(msgCount>numberOfMsgs)
		{
			System.out.println("Extra messages received that expected, test FAILED");
			RC = ReturnCodes.ERROR_EXTRA_MGS_RECEIVED;
		}

		simpleJMSAsyncTest.closeEverythingDown();
		if (RC == 0)
		{
			System.out.println("PASSED");
		}
		else
		{
			System.out.println("TEST FAILED, RETURN CODE: "+RC+"!!");
		}
		System.exit(RC);		
	}
	
	private void createProducer()
	{
		try
		{
			ConnectionFactory connectFact1 = ImaJmsFactory.createConnectionFactory();
			((ImaProperties) connectFact1).put("Server", imaIP);
			((ImaProperties) connectFact1).put("Port", imaPort);
			connect1 = connectFact1.createConnection();
			connect1.setClientID(prodClientID );
			connect1.start();
			System.out.println("Producer connected to IP: "+imaIP + ". port: "+ imaPort+ ". ClientID: "+ prodClientID);
			session1 = connect1.createSession(false, Session.AUTO_ACKNOWLEDGE);
			topic = session1.createTopic(prodTopicName);
			producer1 = session1.createProducer(topic);
			producer1.setDeliveryMode(DeliveryMode.PERSISTENT);
		}
		catch(Exception e)
		{
			RC = ReturnCodes.ERROR_CREATING_PRODUCER;
			e.printStackTrace();
			System.exit(RC);
		}				
	}
	
	private void sendMessages()
	{
		try
		{
			String msgContent = "";
			int i;
			for(i = 1; i <= numberOfMsgs; i++)
			{
				msgContent = "AJMSG-0"+i;
				producer1.send(session1.createTextMessage(msgContent));
				if(printAllMessages)
					System.out.println("Msg No: "+i+" and Msg: "+ msgContent+" is sent");
			}
		}
		catch(Exception e)
		{
			RC = ReturnCodes.ERROR_SENDING_A_MESSAGE;
			e.printStackTrace();
		}
		
	}
	
	private void createConsumer()
	{
		try
		{
			System.out.println("Creating connection for the consumer");
			ConnectionFactory connectFact2 = ImaJmsFactory.createConnectionFactory();
			((ImaProperties) connectFact2).put("Server", imaIP);
			((ImaProperties) connectFact2).put("Port", imaPort);
			connect2 = connectFact2.createConnection();
			connect2.setClientID(consClientID);
			connect2.start();
			session2 = connect2.createSession(false, Session.AUTO_ACKNOWLEDGE);
			topic = session2.createTopic(consTopicName);
			consumer1 = session2.createConsumer(topic);
			consumer1.setMessageListener(this);
			System.out.println("Consumer connected to IP: "+imaIP + ". port: "+ imaPort+ ". ClientID: "+ consClientID);
		}
		catch(Exception e)
		{
			RC = ReturnCodes.ERROR_CREATING_CONSUMER;
			e.printStackTrace();
			System.exit(RC);
		}
	}

	public void onMessage(Message msg)
	{
		if(printAllMessages)
			System.out.println("A Message arrived!");
		dontWaitForever = 0;
		try 
		{	if (msg instanceof TextMessage)
			{
				TextMessage textmsg = (TextMessage) msg;
				msgCount ++;
					if (printAllMessages)
						System.out.println("Msgs Received is: "+textmsg.getText()+". Count = "+msgCount);
			}
			else
			{
				System.out.println("Message Received is Invalid");
				RC = ReturnCodes.ERROR_INVALID_MSG_RECEIVED;
			}
		}
		catch(Exception e)
		{
			RC = ReturnCodes.ERROR_RECEIVING_A_MESSAGE;
			e.printStackTrace();
		}
	}


	private void closeEverythingDown()
	{		
		try
		{
			producer1.close();
			session1.close();
			connect1.close();
		}
		catch(Exception e)
		{
			RC = ReturnCodes.ERROR_DESTROYING_PUBLISHER_FAILED;
			e.printStackTrace();
		}
		try
		{
			consumer1.close();
			session2.close();
			connect2.close();
		}
		catch(Exception e)
		{
			RC = ReturnCodes.ERROR_DESTROYING_CONSUMER_FAILED;
			e.printStackTrace();
		}			
	}

}
