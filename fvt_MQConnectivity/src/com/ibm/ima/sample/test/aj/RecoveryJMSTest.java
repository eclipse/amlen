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

public class RecoveryJMSTest implements MessageListener
{
	private Connection connect1, connect2;
	private Session session1, session2;
	private MessageProducer producer1;
	private Topic topic1;
	private MessageConsumer consumer1;

	private static int RC = 0;
	private static final int CREATING_PUBLISHER_FAILED = 1011;
	private static final int CREATING_CONSUMER_FAILED = 1012;
	private static final int SENDING_A_MESSAGE_FAILED = 1004;
	private static final int RECEIVING_A_MESSAGE_FAILED = 1005;
	private static final int DESTROYING_PUBLISHER_FAILED = 1014;
	private static final int DESTROYING_CONSUMER_FAILED = 1015;
	private static final int INTERRUPTED_SLEEP_FAILED = 1019;
	
	private static int numberOfMsgs = 500;
	private static int printAtEvery = 25;
	private static int loopCount = 0;
	private static int msgCount = 0;
	private static int prevMsgCount = 0;
		
	private static String producerIP = "localhost", consumerIP = "localhost";
	private static int producerPort = 1987, consumerPort = 1987;
	public static Connection imaConnection1 = null;
	
	public static void main(String[] args) 
	{
		System.out.println("Started!!\n");
		RecoveryJMSTest recoveryJMS = new RecoveryJMSTest();

		recoveryJMS.createProducer();
		System.out.println("Producer created!!");
				
		recoveryJMS.createConsumer();
		System.out.println("Consumer created!!");
		
		recoveryJMS.sendMessages();
		
		while (msgCount < numberOfMsgs)
		{
			try
			{
				if(loopCount<=10)
				{
					prevMsgCount = msgCount;
					Thread.sleep(1000);
					System.out.println("Number of messages received: "+msgCount);
					loopCount++;
					
					if(prevMsgCount == msgCount && (msgCount != 0))
					{
						loopCount = 0;
					}
				}
				else
				{
					System.out.println("\nWaited for 10secs after the last response, so disconnectiong.");
					break;
				}
			}
			catch(InterruptedException e)
			{
				e.printStackTrace();
				RC = RecoveryJMSTest.INTERRUPTED_SLEEP_FAILED;
			}
		}
		if(msgCount == numberOfMsgs)
			System.out.println("All messages received, we are now closing down all connections.");
		recoveryJMS.destroyConsumer();
		recoveryJMS.destroyProducer();
		if (RC == 0)
		{
			System.out.println("Everything is done without any errors!!");
		}
		else
		{
			System.out.println("Program Finsihed with Errors, last error is RC: "+RC+"!!");
		}
		System.exit(RC);		
	}
	
	private void createProducer()
	{
		try
		{
			ConnectionFactory connectFact1 = ImaJmsFactory.createConnectionFactory();
			((ImaProperties) connectFact1).put("Server", producerIP);
			((ImaProperties) connectFact1).put("Port", producerPort);
			connect1 = connectFact1.createConnection();
			System.out.println("Producer connected to IP: "+consumerIP + " and port: "+ consumerPort);
			connect1.setClientID("ClientAJ");
			connect1.start();
			session1 = connect1.createSession(false, Session.AUTO_ACKNOWLEDGE);
			topic1 = session1.createTopic("FOOTBALL/ARSENAL");
			producer1 = session1.createProducer(topic1);
			producer1.setDeliveryMode(DeliveryMode.PERSISTENT);
			
			
		}
		catch(Exception e)
		{
			RC = CREATING_PUBLISHER_FAILED;
			e.printStackTrace();
		}				
	}
	
	private void sendMessages()
	{
		try
		{
			for(int i = 1; i <= numberOfMsgs; i++)
			{
				producer1.send(session1.createTextMessage("AJMSG-0"+i));
			}
			System.out.println("All Messages Sent!");
		}
		catch(Exception e)
		{
			RC = SENDING_A_MESSAGE_FAILED;
			e.printStackTrace();
		}
		
	}
	
	private void createConsumer()
	{
		try
		{
			ConnectionFactory connectFact2 = ImaJmsFactory.createConnectionFactory();
			((ImaProperties) connectFact2).put("Server", consumerIP);
			System.out.println("Consumer connected to IP: "+consumerIP + " and port: "+ consumerPort);
			((ImaProperties) connectFact2).put("Port", consumerPort);

			connect2 = connectFact2.createConnection();
			connect2.setClientID("ClientHQ");
			connect2.start();
			session2 = connect2.createSession(false, Session.AUTO_ACKNOWLEDGE);
			topic1 = session2.createTopic("FOOTBALL/ARSENAL");
			consumer1 = session2.createConsumer(topic1);
			consumer1.setMessageListener(this);
			
		}
		catch(Exception e)
		{
			RC = CREATING_CONSUMER_FAILED;
			e.printStackTrace();
		}
	}

	public void onMessage(Message msg)
	{
		//System.out.println("\nMessage arrived!");
		try 
		{	if (msg instanceof TextMessage)
			{
				TextMessage textmsg = (TextMessage) msg;
				
				if (textmsg.getText().contains("AJMSG-0"))
				{
					msgCount ++;
					if (msgCount%printAtEvery == 0)
						System.out.println("MsgCoutn = "+msgCount);
				}	
				else
				{
					System.out.println("The messsage received is: "+ textmsg.getText());
				}
				

			}
		}
		catch(Exception e)
		{
			RC = RECEIVING_A_MESSAGE_FAILED;
			e.printStackTrace();
		}
	}

	private void destroyProducer()
	{
		try
		{
			producer1.close();
			session1.close();
			connect1.close();
		}
		catch(Exception e)
		{
			RC = DESTROYING_PUBLISHER_FAILED;
			e.printStackTrace();
		}			
	}
	private void destroyConsumer()
	{
		try
		{
			consumer1.close();
			session2.close();
			connect2.close();
		}
		catch(Exception e)
		{
			RC = DESTROYING_CONSUMER_FAILED;
			e.printStackTrace();
		}			
	}

}
