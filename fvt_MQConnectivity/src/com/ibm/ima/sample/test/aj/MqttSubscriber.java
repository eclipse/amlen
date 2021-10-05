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

import org.eclipse.paho.client.mqttv3.*;

public class MqttSubscriber	implements MqttCallback
{

	/**
	 */
	
	private static int RC = 0;
	static String topicNameToSubscribe ="";
	static int numberOfSubscribers;
	static String clientIdentifier= "AJ_Clients_";	
	public static String expectedMsg;
	
	private static int msgCount =0;
	private static int expectedCount;
	
	private MqttClient mqttClient[] = new MqttClient[1000];
	
	public static void main(String[] args) throws InterruptedException, IOException, MqttException 
	{
		MqttSubscriber mqttSub = new MqttSubscriber();
		String ip = "", prt = "";
		if (args.length==0)
		{
			System.out.println("\nTest aborted, test requires command line arguments!! (IP address, port)");
			RC=ReturnCodes.ERROR_NO_COMMANDLINE_ARGS;
			System.exit(RC);
		}
		else if(args.length == 6)
		{
			ip = args[0];
			prt = args[1];
			topicNameToSubscribe = args[2]; 
			numberOfSubscribers=Integer.parseInt(args[3]);
			expectedMsg = args[4];
			expectedCount = Integer.parseInt(args[5]);
		}
		else
		{
			System.out.println("\nInvalid number of commandline arguments! Test Aborted");
			RC=ReturnCodes.ERROR_INVALID_NUMBER_OF_COMMANDLINE_ARGS;
			System.exit(RC);
		}
		
		for(int i=1; i<=numberOfSubscribers; i++)
			mqttSub.createSubscriptions(ip, prt, topicNameToSubscribe, i);
		
		System.out.print("Waiting for msgs");
		while(true)
		{	
			if(msgCount==0)
				System.out.print(".");
			else
				System.out.println("\n"+msgCount);
			Thread.sleep(1000);
			if(msgCount==expectedCount)
			{
				System.out.println("\nALL MESSAGES ARRIVED! Count: "+msgCount);
				mqttSub.closeSubscriptions();
				System.exit(0);
			}
		}
	}

	private void createSubscriptions(String ip, String prt, String topicName, int count) 
	{
		String clientId = clientIdentifier + Integer.toString(count);
		try 
		{
			MqttConnectOptions conOpt= new MqttConnectOptions();
			String serverURI = "tcp://"+ip+":"+prt;
			mqttClient[count-1] = new MqttClient(serverURI, clientId);
			mqttClient[count-1].setCallback(this);
			conOpt.setCleanSession(false);
			mqttClient[count-1].connect(conOpt);
			mqttClient[count-1].subscribe(topicName, 2);		
			System.out.println("Client: "+clientId+" connected to IP: "+ip+" port: "+prt+". Subscribed to Topic: "+topicName);
		} 
		catch (MqttException e) 
		{		
			e.printStackTrace();
		}
		
	}
	@Override
	public void connectionLost(Throwable e) 
	{
		System.out.println("Connection Lost!!: ");
		e.printStackTrace();
	}

	@Override
	public void deliveryComplete(IMqttDeliveryToken e) 
	{
		
	}

	@Override
	public synchronized void messageArrived(String e1, MqttMessage payload) throws Exception 
	{
		String msg;
		msg = payload.toString();
		if(msg.contains(expectedMsg))
		{
			msgCount++;
		}
	}
	
	private void closeSubscriptions() throws MqttException 
	{
		for(int count = 0;count<numberOfSubscribers;count++)
		{
			mqttClient[count].unsubscribe(topicNameToSubscribe);
			mqttClient[count].disconnect();
		}
	}
}

