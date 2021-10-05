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

import java.io.IOException;
import java.util.Hashtable;
import com.ibm.micro.client.mqttv3.MqttClient;
import com.ibm.micro.client.mqttv3.MqttConnectOptions;
import com.ibm.micro.client.mqttv3.MqttDeliveryToken;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttMessage;
import com.ibm.micro.client.mqttv3.MqttTopic;
import com.ibm.mq.MQDestination;
import com.ibm.mq.MQException;
import com.ibm.mq.MQGetMessageOptions;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQQueueManager;
import com.ibm.mq.constants.CMQC;
import com.ibm.mq.headers.MQDataException;
import com.ibm.mq.headers.MQHeaderIterator;

/**
 * 
 * @Name		: ImaToMQMultiConsumerMapping
 * @Author		:
 * @Created On	: 23/11/2012
 * @Category	: SVT
 * 
 * @Description : Publishes n MQTT messages to ISM, MQ_Connectivity 
 * 				passes that to MQ and subscribes it from MQ as a
 * 				base Java message.  
 * 
 * **/
public class ImaToMQMultiConsumerMapping
{
	String msgContents ="AJ_MSG";
	static int numberOfProdClients = 20;
	int numberOfMsgsPerProd = 100;
	int printAtEvery = 100;
	static String topicName = "";
	int waitForExtraMsgs = 1000;				/*Milliseconds to wait*/
	
	/*The following values are default values used for debugging
	 * purposes only, and the program is run without any args.*/
	static String MQHost = "";			
	static int MQPort;
	static String MQChannel = "";
	static String qmName = "";		
	MQDestination destinationForSub = null;
	
	static String IMAHost = "";
	static int IMAPort;
	MqttClient[] ismClient = new MqttClient[numberOfProdClients];

	String currentMsg = null;
	int msgsRecievedCount = 0;
	static int RC = 0;
	
	final int openOptionsForGet = CMQC.MQSO_CREATE | CMQC.MQSO_FAIL_IF_QUIESCING
    | CMQC.MQSO_MANAGED | CMQC.MQSO_NON_DURABLE;
	
	public static void main(String[] args)
	{
		String ismClientID=null;
		int i;
		ImaToMQMultiConsumerMapping ism2mq = new ImaToMQMultiConsumerMapping();		
		
		try
		{
			if(args.length == 0)
			{
				System.out.println("\nTest aborted, test requires command line arguments!! (IP address, port)");
				RC = ReturnCodes.ERROR_NO_COMMANDLINE_ARGS;
				System.exit(RC);
			}
			else if(args.length == 7)
			{
				IMAHost = args[0];
				MQHost = args[1];
				IMAPort = Integer.parseInt(args[2]);
				MQPort = Integer.parseInt(args[3]);
				qmName = args[4];
				topicName = args[5];
				MQChannel = args[6];
			}
			else
			{
				System.out.println("Number of arguments passed is invalid. Test aborted!");
				RC = ReturnCodes.ERROR_INVALID_COMMANDLINE_ARGS;
				System.exit(RC);
			}
			System.out.println("Using IMA IP: '"+IMAHost+"'; IMAPort: "+IMAPort+
								"; To MQ IP: "+MQHost+"; QM: "+qmName+"; MQPort: "+MQPort+
								"; MQChannel: "+MQChannel);

			ism2mq.createSubscriber();
			
			for(i=1;i<=numberOfProdClients;i++)
			{
				ismClientID = "ClientAJ-0"+String.valueOf(i);
				ism2mq.createProducer(ismClientID, i);
			}
			ism2mq.waitForMsg();
			ism2mq.checkForExtraMsgs();
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}		
		ism2mq.closeConnections();

		if(RC == 0)
			System.out.println("PASSED");
		else
			System.out.println("FAILED");
		System.exit(RC);
	}
	
	@SuppressWarnings("unchecked")
	public void createSubscriber() throws MQException
	{
		Hashtable properties = new Hashtable();
		properties.put("hostname", MQHost);
		properties.put("port", MQPort);
		properties.put("channel", MQChannel);
		
		MQQueueManager qM = new MQQueueManager(qmName, properties);
		destinationForSub = qM.accessTopic(topicName, "SYSTEM.BASE.TOPIC", 
							CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION, CMQC.MQSO_CREATE);
		
	}
	
	public void waitForMsg() throws MQException
	{
		int i;
		String msgPayload;
		

		for(i = 1;i<=numberOfMsgsPerProd*numberOfProdClients; i++)
		{	
			MQGetMessageOptions getMsgOption = new MQGetMessageOptions();
			MQMessage messageIn = new MQMessage();
			MQHeaderIterator itr = new MQHeaderIterator(messageIn);
			
			getMsgOption.options = CMQC.MQGMO_WAIT;
			getMsgOption.waitInterval = 2000;
			
			destinationForSub.get(messageIn, getMsgOption);
			msgPayload = null;
			try
			{
				itr.skipHeaders();		//We are left with the body as header is removed.
				msgPayload = itr.getBodyAsText();
				if(null == msgPayload)
				{
					System.out.println("No Message Recieved!");
					RC = 2002;
					System.exit(RC);
				}
				else 
				{
					if(msgPayload.compareTo(msgContents)==0)
					{
						msgsRecievedCount++;
						if(msgsRecievedCount%printAtEvery==0)
							System.out.println("Msg No."+i+" Msg: "+msgPayload);
					}
					else
					{
						System.out.println("Test Failed.Incorrect msg recieved, msg: "+msgPayload+ 
											". \nThe msg number is "+msgsRecievedCount+".");
						RC = 2001;
						System.exit(RC);
					}
				}
				if(msgsRecievedCount == numberOfMsgsPerProd*numberOfProdClients)
				{
					System.out.println(msgsRecievedCount+" msgs recieved!");
				}
				
			}
			catch (MQDataException e)
			{
				e.printStackTrace();
			}
			catch (IOException e)
			{
				e.printStackTrace();
			}
		}
	}
	
	@SuppressWarnings("deprecation")
	public void checkForExtraMsgs()
	{
		int extraMsgsRecievedCount = 0;
		String msgPayload;
		boolean stillExtraMsgs = true;
		System.out.println("Looking for any duplicate messages, for "+waitForExtraMsgs/1000+" seconds. ");

		while(stillExtraMsgs)
		{
			MQGetMessageOptions getMsgOption = new MQGetMessageOptions();
			MQMessage messageIn = new MQMessage();
			MQHeaderIterator itr = new MQHeaderIterator(messageIn);
			
			getMsgOption.options = CMQC.MQGMO_WAIT;
			getMsgOption.waitInterval = waitForExtraMsgs;
			try
			{
				destinationForSub.get(messageIn, getMsgOption);
				extraMsgsRecievedCount++;
				msgsRecievedCount++;
			}
			catch(MQException exe)
			{
				if(exe.reasonCode == MQException.MQRC_NO_MSG_AVAILABLE)
				{
					System.out.println("No Extra msgs.");
					break;
				}
				else
				{
					System.out.println("Checking for extra messages failed: ");
					exe.printStackTrace();
					RC = 32;
				}
			}
			msgPayload = null;
			try
			{
				itr.skipHeaders();		//We are left with the body as header is removed.
				msgPayload = itr.getBodyAsText();
				if(null == msgPayload)
				{
					System.out.println("Null Message Recieved!");
					RC = 1999;
				}
				else 
				{
					if(msgPayload.compareTo(msgContents)==0)
					{
						System.out.println("Extra Msg: "+msgPayload);
						RC = 2000;
					}
					else
					{
						System.out.println("Extra msg: "+msgPayload+ 
											". Total msg number: "+msgsRecievedCount+"."+
											"Extra msg no: "+ extraMsgsRecievedCount);
						RC = 2001;
					}
				}
				
			}
			catch (MQDataException e)
			{
				e.printStackTrace();
			}
			catch (IOException e)
			{
				e.printStackTrace();
			}
		}
	}
	
	public void createProducer(String clientID, int clientNo)
	{
		int i;
		String publisherClientid = clientID;
		int msgsSendCount = 0;
		
		MqttDeliveryToken deliveryToken = null;
	
		try
		{
			ismClient[clientNo-1] = new MqttClient("tcp://"+IMAHost+":"+IMAPort,
													publisherClientid, null);

			MqttConnectOptions mqttConnectOptions = new MqttConnectOptions();
			MqttMessage mqttMsg = new MqttMessage();
			mqttConnectOptions.setCleanSession(false);
			ismClient[clientNo-1].connect(mqttConnectOptions);		

			MqttTopic topic = ismClient[clientNo-1].getTopic(topicName);
			for(i=1;i<=numberOfMsgsPerProd;i++)
			{	
				mqttMsg.setPayload(msgContents.getBytes());
				mqttMsg.setQos(2);
				deliveryToken = topic.publish(mqttMsg);
				deliveryToken.waitForCompletion(10000);
				if(deliveryToken.isComplete())
					msgsSendCount++;
			}
			if(numberOfMsgsPerProd == msgsSendCount)
			{
				System.out.println("All messages sent from "+clientID+"!");
			}
			else
			{
				System.out.println("Only "+numberOfMsgsPerProd+" msgs were sent!");
			}
		}
		catch(MqttException e2)
		{
			System.out.println("Error creating producer no.: "+clientNo);
			RC=ReturnCodes.ERROR_CREATING_PRODUCER;
			e2.printStackTrace();
		}
	}

	private void closeConnections()
	{
		for(int j=0;j<numberOfProdClients;j++)
		{
			try 
			{
				ismClient[j].disconnect();
			} 
			catch (MqttException e) 
			{
				System.out.println("Error destroying publisher no.: "+j);
				RC=ReturnCodes.ERROR_DESTROYING_PUBLISHER_FAILED;
				e.printStackTrace();
			}
		}
		try
		{
			destinationForSub.close();
		}
		catch (MQException e)
		{
			e.printStackTrace();
		}
	}

}
