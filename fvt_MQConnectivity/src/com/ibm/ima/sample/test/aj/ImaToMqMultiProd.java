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
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Hashtable;
import com.ibm.micro.client.mqttv3.MqttClient;
import com.ibm.micro.client.mqttv3.MqttConnectOptions;
import com.ibm.micro.client.mqttv3.MqttDeliveryToken;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttMessage;
import com.ibm.micro.client.mqttv3.MqttSecurityException;
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
 * @Name		: ImaToMQAdvanced
 * @Author		:
 * @Created On	: 23/11/2012
 * @Category	: SVT
 * 
 * @Description : Publishes n MQTT messages to IMA, MQ_Connectivity 
 * 				passes that to MQ and subscribes it from MQ as a
 * 				base Java message.  
 * 
 * **/
public class ImaToMqMultiProd
{
	String msgContents ="AJ_MSG";
	static int numberOfProdClients = 2;
	static int numberOfMsgsPerProd = 10000;
	int printAtEvery = 10000;
	static String topicName = "";
	
	/*The following values are default values used for debugging
	 * purposes only, and the program is run without any args.*/
	static String MQHost = "";						//9.20.36.144
	static int MQPort;
	static String MQChannel = "";
	static String qmName = "";		
	MQDestination destinationForSub = null;
	
	static String IMAHost = "";
	static int IMAPort;
	MqttClient[] imaClient = new MqttClient[numberOfProdClients];

	String currentMsg = null;
	int msgsRecievedCount = 0;
	static int RC = ReturnCodes.DEFAULT_RC;
	
	final int openOptionsForGet = CMQC.MQSO_CREATE | CMQC.MQSO_FAIL_IF_QUIESCING
    | CMQC.MQSO_MANAGED | CMQC.MQSO_NON_DURABLE;
	
	public static void main(String[] args)
	{
		String imaClientID=null;
		int i;
		ImaToMqMultiProd ima2mq = new ImaToMqMultiProd();		
		
		SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss");
		System.out.println("Test Commenced at: "+sdf.format(Calendar.getInstance().getTime()));
		System.out.println("Attempting test to send messages from IMA to MQ");
		if(args.length == 0)
		{
			System.out.println("\nTest aborted, test requires command line arguments!! (IP address, port)");
			RC = ReturnCodes.ERROR_NO_COMMANDLINE_ARGS;
			System.exit(RC);
		}
		else if(args.length == 8)
		{
			IMAHost = args[0];
			IMAPort = Integer.parseInt(args[1]);
			MQHost = args[2];
			MQPort = Integer.parseInt(args[3]);
			qmName = args[4];
			topicName = args[5];
			MQChannel = args[6];
			numberOfMsgsPerProd = Integer.parseInt(args[7]);
		}
		else
		{
			System.out.println("Number of arguments passed is invalid. Test aborted!");
			RC = ReturnCodes.ERROR_INVALID_COMMANDLINE_ARGS;
			System.exit(RC);
		}
		System.out.println("Using IMA: '"+IMAHost+"'; IMAPort: "+IMAPort+
							"; To MQ: "+MQHost+"; MQPort: "+MQPort+
							"; MQChannel: "+MQChannel+"; QM: "+qmName);
		
		ima2mq.createSubscriber();
		
		for(i=1;i<=numberOfProdClients;i++)
		{
			imaClientID = "ClientAJ-0"+String.valueOf(i);
			ima2mq.createProducer(imaClientID, i);
		}
		
		for(i=1;i<=numberOfProdClients;i++)
		{
			imaClientID = "ClientAJ-0"+String.valueOf(i);
			ima2mq.sendMessages(imaClientID, i);
		}
		ima2mq.waitForMsg();
		ima2mq.checkForExtraMsgs();
		ima2mq.closeConnections();

		if(RC == 0)
			System.out.println("PASSED");
		else
		{
			System.out.println("RC = "+RC);
			System.out.println("FAILED");
		}	
		System.out.println("Test finished at: "+sdf.format(Calendar.getInstance().getTime()));
		System.exit(RC);
	}

	public void createProducer(String clientID, int clientNo)
	{
		String publisherClientid = clientID;
		try 
		{
			imaClient[clientNo-1] = new MqttClient("tcp://"+IMAHost+":"+IMAPort,
													publisherClientid, null);
			MqttConnectOptions mqttConnectOptions = new MqttConnectOptions();
			mqttConnectOptions.setCleanSession(false);
			imaClient[clientNo-1].connect(mqttConnectOptions);	
		} 
		catch (MqttException e) 
		{
			RC=ReturnCodes.ERROR_CREATING_PRODUCER;
			e.printStackTrace();
		}	
	}
	
	public void sendMessages(String clientID, int clientNo)
	{
		int i;
		int msgsSendCount = 0;
		MqttDeliveryToken deliveryToken = null;
		MqttTopic topic = imaClient[clientNo-1].getTopic(topicName);
		SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss");

		MqttMessage mqttMsg = new MqttMessage();
		try
		{
			System.out.println("About to send MQTT messages from client "+clientID+" at: "+sdf.format(Calendar.getInstance().getTime()));
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
				System.out.println("All messages sent from "+clientID+" at "+sdf.format(Calendar.getInstance().getTime()));
			}
			else
			{
				System.out.println("Only "+numberOfMsgsPerProd+" msgs were sent at "+sdf.format(Calendar.getInstance().getTime()));
			}
		}
		catch(MqttSecurityException e1)
		{
			RC=ReturnCodes.ERROR_CREATING_PRODUCER;
			e1.printStackTrace();
		}
		catch(MqttException e2)
		{
			RC=ReturnCodes.ERROR_CREATING_PRODUCER;
			e2.printStackTrace();
		}
	}
	@SuppressWarnings("unchecked")
	public void createSubscriber()
	{
		Hashtable properties = new Hashtable();
		properties.put("hostname", MQHost);
		properties.put("port", MQPort);
		properties.put("channel", MQChannel);
		
		MQQueueManager qM;
		try 
		{
			qM = new MQQueueManager(qmName, properties);
			destinationForSub = qM.accessTopic(topicName, "SYSTEM.BASE.TOPIC", 
					CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION, openOptionsForGet);
		} 
		catch (MQException e) 
		{
			RC=ReturnCodes.ERROR_CREATING_CONSUMER;
			e.printStackTrace();
		}
	}
	
	public void waitForMsg()
	{
		int i;
		String msgPayload;
		
		SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss");
		System.out.println("about to get MQ messages at time: "+sdf.format(Calendar.getInstance().getTime()));

		for(i = 1;i<=numberOfMsgsPerProd*numberOfProdClients; i++)
		{	
			MQGetMessageOptions getMsgOption = new MQGetMessageOptions();
			MQMessage messageIn = new MQMessage();
			MQHeaderIterator itr = new MQHeaderIterator(messageIn);
			
			getMsgOption.options = CMQC.MQGMO_WAIT;
			getMsgOption.waitInterval = 30000;
			try
			{

				destinationForSub.get(messageIn, getMsgOption);
				msgPayload = null;

				itr.skipHeaders();		//We are left with the body as header is removed.
				msgPayload = itr.getBodyAsText();
				if(null == msgPayload)
				{
					System.out.println("No Message Recieved!");
					RC = ReturnCodes.ERROR_NO_MSGS_FOR_10SECONDS_FAILED;
					break;
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
						RC = ReturnCodes.ERROR_INVALID_MSG_RECEIVED;
						break;
					}
				}
				if(msgsRecievedCount == numberOfMsgsPerProd*numberOfProdClients)
				{
					System.out.println(msgsRecievedCount+" msgs recieved!");
				}
				
			}
			catch (MQDataException e1)
			{
				System.out.println("MQDataException caught at: "+sdf.format(Calendar.getInstance().getTime()));
				RC=ReturnCodes.ERROR_RECEIVING_A_MESSAGE;
				e1.printStackTrace();
				break;
			}
			catch (IOException e2)
			{
				System.out.println("IOException caught at: "+sdf.format(Calendar.getInstance().getTime()));
				RC=ReturnCodes.ERROR_RECEIVING_A_MESSAGE;
				e2.printStackTrace();
				break;
			} 
			catch (MQException e3) 
			{
				System.out.println("MQException caught at: "+sdf.format(Calendar.getInstance().getTime()));
				RC=ReturnCodes.ERROR_RECEIVING_A_MESSAGE;
				e3.printStackTrace();
				break;
			}
		}
		if(RC==ReturnCodes.DEFAULT_RC && msgsRecievedCount==numberOfMsgsPerProd*numberOfProdClients )
			RC=ReturnCodes.PASSED;			
		System.out.println("finished getting initial set of MQ messages at: "+sdf.format(Calendar.getInstance().getTime()));
	}
	
	public void checkForExtraMsgs()
	{
		int extraMsgsRecievedCount = 0;
		String msgPayload;
		boolean stillExtraMsgs = true;
		SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss");
		System.out.println("Looking for any duplicate messages, until no messages arrived for the 10 seconds, at time: "+sdf.format(Calendar.getInstance().getTime()));

		while(stillExtraMsgs)
		{
			MQGetMessageOptions getMsgOption = new MQGetMessageOptions();
			MQMessage messageIn = new MQMessage();
			MQHeaderIterator itr = new MQHeaderIterator(messageIn);
			
			getMsgOption.options = CMQC.MQGMO_WAIT;
			getMsgOption.waitInterval = 30000;
			try
			{
				destinationForSub.get(messageIn, getMsgOption);
				extraMsgsRecievedCount++;
				msgsRecievedCount++;
			}
			catch(MQException exe)
			{
				if(exe.reasonCode == CMQC.MQRC_NO_MSG_AVAILABLE)
				{
					System.out.println("No Extra msgs at "+sdf.format(Calendar.getInstance().getTime()));
					RC = ReturnCodes.PASSED;
					break;
				}
				else
				{
					System.out.println("MQException caught at: "+sdf.format(Calendar.getInstance().getTime()));
					System.out.println("Checking for extra messages failed: ");
					exe.printStackTrace();
					RC = ReturnCodes.ERROR_WHILE_CHECKING_FOR_EXTRA_MSGS;
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
					RC = ReturnCodes.ERROR_NULL_MESSAGE_RECEIVED;
				}
				else 
				{
					System.out.println("Extra msg: "+msgPayload+ 
											". Total msg number: "+msgsRecievedCount+"."+
											"Extra msg no: "+ extraMsgsRecievedCount);
					RC = ReturnCodes.ERROR_EXTRA_MGS_RECEIVED;
				}
				
			}
			catch (MQDataException e)
			{
				RC = ReturnCodes.ERROR_WHILE_CHECKING_FOR_EXTRA_MSGS;				
				e.printStackTrace();
				break;
			}
			catch (IOException e)
			{
				RC = ReturnCodes.ERROR_WHILE_CHECKING_FOR_EXTRA_MSGS;				
				e.printStackTrace();
				break;
			}
		}
		System.out.println("finished getting extra MQ messages at: "+sdf.format(Calendar.getInstance().getTime()));
		
	}
	

	private void closeConnections()
	{	
		try
		{
			for(int j=0;j<numberOfProdClients;j++)
				imaClient[j].disconnect();
			destinationForSub.close();
		}
		catch(MqttException e1)
		{
			RC=ReturnCodes.ERROR_DESTROYING_PUBLISHER_FAILED;
			e1.printStackTrace();
		}
		catch (MQException e2) 
		{
			RC=ReturnCodes.ERROR_DESTROYING_CONSUMER_FAILED;
			e2.printStackTrace();
		}
	}

}
