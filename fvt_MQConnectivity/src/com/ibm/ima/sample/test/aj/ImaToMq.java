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
 * @Name		: ImaToMQ 
 * @Author		:
 * @Created On	: 23/11/2012
 * @Category	: SVT
 * 
 * @Description : Publishes a MQTT message to IMA, MQ_Connectivity 
 * 				passes that to MQ and subscribes it from MQ as a
 * 				base Java message.  
 * 
 * **/
public class ImaToMq
{
	/*The following values are default values used for debugging
	 * purposes only, and the program is run without any args.*/

	MQDestination destinationForSub = null;
	MqttClient imaClient;
	
	static String MQHost = "";	
	static int MQPort = 0;
	static String MQChannel = "";
	static String qmName = "";		

	static String IMAHost = "";
	static int IMAPort = 0;
	static String topicName = "";	
	
	String publisherClientid = "ClientAJ";
	String consumerClientid = "ClientHQ";
	String msgContents ="AJ_MSG";
	static int numberOfMsgs = 0;
	

	String currentMsg = null;
	
	static int RC = ReturnCodes.DEFAULT_RC;
	
	final int openOptionsForGet = CMQC.MQSO_CREATE | CMQC.MQSO_FAIL_IF_QUIESCING
    | CMQC.MQSO_MANAGED | CMQC.MQSO_NON_DURABLE;
	
	public static void main(String[] args)
	{
		ImaToMq ima2mq = new ImaToMq();
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
			MQHost = args[1];
			IMAPort = Integer.parseInt(args[2]);
			MQPort = Integer.parseInt(args[3]);
			qmName = args[4];
			topicName = args[5];
			MQChannel = args[6];
			numberOfMsgs = Integer.parseInt(args[7]);
			
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
		ima2mq.createSubscriber();
		ima2mq.createProducer();
		ima2mq.sendMessages();
		ima2mq.waitForMsg();
		ima2mq.closeConnections();
		
		if(RC==ReturnCodes.PASSED)
			System.out.println("PASSED");
		else
			System.out.println("FAILED");
		System.out.println("Test Finished at: "+sdf.format(Calendar.getInstance().getTime()));
		System.exit(RC);
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
			System.out.println("Queue Manger Name: "+qmName+ " Topic: "+topicName);
			qM = new MQQueueManager(qmName, properties);
			destinationForSub = qM.accessTopic(topicName, "SYSTEM.BASE.TOPIC", 
					CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION, openOptionsForGet);
		} 
		catch (MQException e) 
		{
			e.printStackTrace();
		}
	}
	
	
	public void createProducer()
	{
		try 
		{
			imaClient = new MqttClient("tcp://"+IMAHost+":"+IMAPort,
													publisherClientid, null);
			MqttConnectOptions mqttConnectOptions = new MqttConnectOptions();
			mqttConnectOptions.setCleanSession(false);
			imaClient.connect(mqttConnectOptions);		

		} 
		catch (MqttException e) 
		{
			e.printStackTrace();
		}

	}
	
	public void sendMessages()
	{
		int i;
		int msgsSendCount = 0;
		MqttDeliveryToken deliveryToken = null;
		MqttTopic topic;
		MqttMessage mqttMsg = new MqttMessage();
		for(i=1;i<=numberOfMsgs;i++)
		{	
			topic = imaClient.getTopic(topicName);				
			mqttMsg.setPayload(msgContents.getBytes());
			mqttMsg.setQos(2);
			try
			{
				deliveryToken = topic.publish(mqttMsg);
				deliveryToken.waitForCompletion(3000);
				if(deliveryToken.isComplete())
					msgsSendCount++;
				else
				{
					RC=ReturnCodes.ERROR_SENDING_A_MESSAGE;
					System.out.println("Sending Msg no.:"+(msgsSendCount+1)+" failed.");
				}
			}
			catch(MqttException e)
			{
				RC=ReturnCodes.ERROR_SENDING_A_MESSAGE;
				System.out.println("Sending Msg no.:"+(msgsSendCount+1)+" failed.");
				e.printStackTrace();
			}
		}
		if(RC==ReturnCodes.DEFAULT_RC)
			System.out.println(numberOfMsgs+" messages sent.");
	}
	
	public void waitForMsg()
	{
		int i;
		int msgsRecievedCount = 0;
		String msgPayload;

		SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss");
		System.out.println(sdf.format(Calendar.getInstance().getTime())+": Looking for the first message.");

		for(i = 1;i<=numberOfMsgs; i++)
		{	
			MQGetMessageOptions getMsgOption = new MQGetMessageOptions();
			MQMessage messageIn = new MQMessage();
			MQHeaderIterator itr = new MQHeaderIterator(messageIn);
			
			getMsgOption.options = CMQC.MQGMO_WAIT;
			getMsgOption.waitInterval = 10000;
			try
			{
				destinationForSub.get(messageIn, getMsgOption);
				msgPayload = null;

				itr.skipHeaders();		//We are left with the body as header is removed.
				msgPayload = itr.getBodyAsText();
				if(null == msgPayload)
				{
					System.out.println(sdf.format(Calendar.getInstance().getTime())+": Failed to receive message no: "+i+" after waiting for 10 seconds!");
					RC = ReturnCodes.ERROR_NULL_MESSAGE_RECEIVED;
					System.exit(RC);
				}
				else 
				{
					if(msgPayload.compareTo(msgContents)==0)
					{
						msgsRecievedCount++;
						System.out.println(sdf.format(Calendar.getInstance().getTime())+": Received the first message.");
					}
					else
					{
						System.out.println("Test Failed.Incorrect msg recieved, msg: "+msgPayload+ 
											". \nThe failed msg number is "+msgsRecievedCount+".");
						RC = ReturnCodes.ERROR_INVALID_MSG_RECEIVED;
						System.exit(RC);
					}
				}
				if(msgsRecievedCount==numberOfMsgs && RC==ReturnCodes.DEFAULT_RC)
				{
					System.out.println("All Messages received!");
					RC=ReturnCodes.PASSED;
				}				
			}
			catch (MQDataException e1) 
			{
				e1.printStackTrace();
				RC=ReturnCodes.ERROR_RECEIVING_A_MESSAGE;
			}
			catch (MQException e2)
			{
				e2.printStackTrace();
				RC=ReturnCodes.ERROR_RECEIVING_A_MESSAGE;
			}
			catch (IOException e3)
			{
				e3.printStackTrace();
				RC=ReturnCodes.ERROR_RECEIVING_A_MESSAGE;
			} 

		}
	}

	public void closeConnections()
	{
		try
		{
			imaClient.disconnect();
			destinationForSub.close();
		}
		catch (MQException e)
		{
			RC=ReturnCodes.ERROR_DESTROYING_CONSUMER_FAILED;
			e.printStackTrace();
		} 
		catch (MqttException e) 
		{
			RC=ReturnCodes.ERROR_DESTROYING_PUBLISHER_FAILED;
			e.printStackTrace();
		}
	}
}
