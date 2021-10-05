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

import java.util.UUID;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;


/*	Simple Test - 
 * 		Testing IMA server by sending a message through the server. Will take two arguments: IP and Port number of the ISM server.
 * 	
 * Author:
 * Created on: 19/10/2012 
 * 
 * */
public class SimpleJMSSyncTest 
{
	ConnectionFactory consumerCF, producerCF;
	
	Connection consumerConnection = null, producerConnection = null;
	Session consumerSession = null, producerSession = null;
	MessageConsumer consumer = null;
	MessageProducer producer= null;
	Topic topic;
	
	private static int totalMsgNumber;
	
	private static int msgCount = 0;
	private static String topicName  = ""; 
	private static int RC = 0;
	private static boolean printAllMsgs = false;

	public static void main(String[] args)
	{
		String ip = "", prt = "";
		SimpleJMSSyncTest syncTest = new SimpleJMSSyncTest();
		if (args.length==0)
		{
			System.out.println("\nTest aborted, test requires command line arguments!! (IP address, port)");
			RC=ReturnCodes.ERROR_NO_COMMANDLINE_ARGS;
			System.exit(RC);
		}
		else if(args.length == 4)
		{
			ip = args[0];
			prt = args[1];
			topicName  = args[2]; 
			totalMsgNumber=Integer.parseInt(args[3]);
		}
		else
		{
			System.out.println("\nInvalid number of commandline arguments! Test Aborted");
			RC=ReturnCodes.ERROR_INVALID_NUMBER_OF_COMMANDLINE_ARGS;
			System.exit(RC);
		}
		syncTest.createConsumer(ip, prt);
		syncTest.createProducer(ip, prt);
		syncTest.sendMessages();
		System.out.println("All Messages Sent!!");
		syncTest.getMessages();
		
		
		if(msgCount==totalMsgNumber)
			System.out.println("All messages received, Total msg count: "+msgCount);
		
		else if(msgCount>totalMsgNumber)
		{
			System.out.println("Extra messages received that expected, test FAILED");
			RC = ReturnCodes.ERROR_EXTRA_MGS_RECEIVED;
		}
		
		syncTest.closeEverythingDown();
		
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
	
	
	
	private void getMessages() 
	{
		while(msgCount<totalMsgNumber)
		{
			try 
			{
				Message msg;
				msg = consumer.receive(3000);
				if (msg == null)
				{
					System.out.println("\nTest FAILED, no message recieved!");
					RC = ReturnCodes.ERROR_INVALID_MSG_RECEIVED;
				}
				else
				{
					msgCount ++;
					if( ((TextMessage)msg).getText().equals("aj_msg001") )
					{
						if(printAllMsgs)
							System.out.println("Message Recieved: "+((TextMessage)msg).getText()+". And the msg Count: "+msgCount);
					}
					else
					{
						System.out.println("Invalid Msg received. Msg: " + ((TextMessage)msg).getText());
						RC = ReturnCodes.ERROR_INVALID_MSG_RECEIVED;
					}
				}
			}
			catch (JMSException e1) 
			{
				e1.printStackTrace();
			}
		}
	}



	private void sendMessages() 
	{
		String body = "aj_msg001";
		TextMessage message;
		try 
		{
			message = consumerSession.createTextMessage(body);
			message.setJMSDeliveryMode(DeliveryMode.PERSISTENT);
			
			for(int i = 1; i<=totalMsgNumber; i++)
			{
				producer.send(message);
				if(printAllMsgs)
					System.out.println("Message sent:" + body);
			}
		} 
		catch (JMSException e) 
		{
			e.printStackTrace();
		}
		
	}

	private void createConsumer(String ip, String prt) 
	{
		try
		{	
			System.out.println("Setting up the consumer to talk to ISM");
			
			consumerCF = ImaJmsFactory.createConnectionFactory();
	        ((ImaProperties) consumerCF).put("Server", ip);
	        ((ImaProperties) consumerCF).put("Port", prt);
	        String consumerClientID = UUID.randomUUID().toString().substring(0, 10);

			consumerConnection = consumerCF.createConnection();
			consumerConnection.setClientID(consumerClientID);
			consumerSession = consumerConnection.createSession(false, Session.AUTO_ACKNOWLEDGE);
			topic = consumerSession.createTopic(topicName); 
			consumer = consumerSession.createConsumer(topic);
			
			consumerConnection.start();
			System.out.println("Subscribing ISM IP: "+ip+" and Port: "+prt+" with clientID: "+ consumerClientID);
			
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}		
	}

	public void createProducer(String ip, String prt)
	{
		try
		{
			System.out.println("Setting up the producer to talk to ISM");
		
			producerCF= ImaJmsFactory.createConnectionFactory();

	        ((ImaProperties) producerCF).put("Server", ip);
	        ((ImaProperties) producerCF).put("Port", prt);
			String producerClientID = UUID.randomUUID().toString().substring(0, 10);;
			
			System.out.println("Publishing to ISM IP: "+ip+" and Port: "+prt+" with clientID: "+ producerClientID);
			
			producerConnection = producerCF.createConnection();
			producerConnection.setClientID(producerClientID);
			
			producerSession = producerConnection.createSession(false, Session.AUTO_ACKNOWLEDGE);
			topic = producerSession.createTopic(topicName);
			producer = producerSession.createProducer(topic);
			producer.setDeliveryMode(DeliveryMode.PERSISTENT);
				
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
	}

	private void closeEverythingDown() 
	{
		System.out.println("Closing down all the connections");
		try 
		{
			consumerConnection.close();
		} 
		catch (JMSException e) 
		{
			e.printStackTrace();
		}	
		
		try 
		{
			producerConnection.close();
		} 
		catch (JMSException e) 
		{
			e.printStackTrace();
		}		
	}
}
