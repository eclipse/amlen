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

import java.text.SimpleDateFormat;
import java.util.Calendar;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;



public class LWTMQConnectivity implements MqttCallback
{
	MqttClient imaClient;
	MqttClient subscriberClient;
	
	static String MQHost = "";	
	static int MQPort = 0;
	static String MQChannel = "";
	static String qmName = "";		

	static String IMAHost = "";
	static int IMAPort = 0;
	static String topicName = "";
	static String lastWillTopic = "lastwill/producer";
	
	String publisherClientid = "ClientAJ";
	String consumerClientid = "ClientHQ";
	String msgContents ="AJ_MSG";
	
	static int totalNumberOfMsgs = 2500;
	static int numberOfMsgsReceived = 0;
	static long dontWaitForever = 0;
	boolean printAllMsgs = false;
	
	String currentMsg = null;
	
	static int RC = ReturnCodes.DEFAULT_RC;
	
	public static void main(String[] args) 
	{
		LWTMQConnectivity lwtConn = new LWTMQConnectivity();
		SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss");
		System.out.println("Test Commenced at: "+sdf.format(Calendar.getInstance().getTime()));
		System.out.println("Attempting test to send messages from IMA to MQ");
		if(args.length == 0)
		{
			System.out.println("\nTest aborted, test requires command line arguments!! (IP address, port)");
			RC = ReturnCodes.ERROR_NO_COMMANDLINE_ARGS;
			System.exit(RC);
		}
		else if(args.length == 4)	/*Testing from RTC*/
		{
			IMAHost = args[0];
			MQHost = args[1];
			IMAPort = Integer.parseInt(args[2]);
			MQPort = Integer.parseInt(args[3]);
			qmName = "QM_AJ";
			topicName = "FOOTBALL/ARSENAL";
			MQChannel = "PlainText";				
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
		lwtConn.createSubscriber();
		lwtConn.createProducer();
		lwtConn.sendMessages();
		while (numberOfMsgsReceived < totalNumberOfMsgs)
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
				System.out.println("No messages for 10 seconds.");
				System.out.println("Only "+numberOfMsgsReceived+" message received!");
				RC = ReturnCodes.ERROR_NO_MSGS_FOR_10SECONDS_FAILED;
				break;
			}
		}
		//lwtConn.closeConnections();
		
		if(RC==ReturnCodes.DEFAULT_RC)
			System.out.println("PASSED");
		else
			System.out.println("FAILED");
		System.out.println("Test Finished at: "+sdf.format(Calendar.getInstance().getTime()));
		System.exit(RC);
	}

	public void createSubscriber()
	{		
		try 
		{
			subscriberClient = new MqttClient("tcp://"+MQHost+":"+MQPort, consumerClientid);
			subscriberClient.setCallback(this);
			MqttConnectOptions conOpt = new MqttConnectOptions();
			conOpt.setCleanSession(false);
			subscriberClient.connect(conOpt);
			subscriberClient.subscribe(topicName);
		} 
		catch (MqttException e) 
		{
			RC = ReturnCodes.ERROR_CREATING_CONSUMER;
			System.out.println("Error creating a consumer");
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
			mqttConnectOptions.setWill(lastWillTopic, "Connection Lost".getBytes(), 2, false);
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
		for(i=1;i<=totalNumberOfMsgs;i++)
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
			System.out.println(totalNumberOfMsgs+" messages sent.");
	}
	public void closeConnections()
	{
		try
		{
			imaClient.disconnect();
			subscriberClient.disconnect();
		}
		catch (MqttException e) 
		{
			RC=ReturnCodes.ERROR_DESTROYING_PUBLISHER_FAILED;
			e.printStackTrace();
		}
	}

	public void connectionLost(Throwable te) 
	{
		System.out.println("Connection Lost!");
		RC=ReturnCodes.ERROR_CONNECTION_LOST;
		te.printStackTrace();
	}

	public void deliveryComplete(IMqttDeliveryToken token) 
	{
		/*FINE*/
	}

	public void messageArrived(String topic, MqttMessage message) 
	{
		try
		{
			String msg = message.toString();
			if(msg.equals(msgContents))
			{
				numberOfMsgsReceived++;
				if(printAllMsgs)
					System.out.println("Topic: "+topic+". Message: "+msg);
			}
			else
			{
				RC=ReturnCodes.ERROR_INVALID_MSG_RECEIVED;
				System.out.println("Invalid message received. MSG: "+msg);
			}
		}
		catch(Exception e)
		{
			System.out.println("Error while receiving a message");
			RC=ReturnCodes.ERROR_RECEIVING_A_MESSAGE;
			e.printStackTrace();
		}
	}
}
