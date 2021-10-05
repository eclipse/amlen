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

import java.io.File;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.UUID;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import com.ibm.ima.test.cli.imaserver.ImaServerStatus;
import com.ibm.ims.svt.clients.MqttConnection;

public class MonitorForErrors implements MqttCallback {

	private static ArrayList<Integer> processIds = new ArrayList<Integer>();
	private static String ipaddressA = null;
	private static String ipaddressB = null;
	private static String mqaddress = null;
	private static String mqport = null;
	private static String mqtopicMonitor = null;
	private static ImaServerStatus imaServerStatusA = null;
	private static ImaServerStatus imaServerStatusB = null;
	private static int numberFilesOnA = -1;
	private static int numberFilesOnB = -1;
	private MqttClient mqmonitor = null;
	private MqttClient producer = null;
	private MqttConnectOptions mqmonitorOptions = new MqttConnectOptions();
	private MqttConnectOptions mqproducerOptions = new MqttConnectOptions();
	private boolean mqMessageReceived = false;
	private MqttTopic topic = null;
	private static String machineName = null;


	/**
	 * @param args
	 */
	public static void main(String[] args) {
		
		ipaddressA = args[0];
		imaServerStatusA = new ImaServerStatus(ipaddressA, "admin", "admin");
		String temp = args[1];
		if(! temp.equals("null"))
		{
			ipaddressB = temp;
			imaServerStatusB = new ImaServerStatus(ipaddressB, "admin", "admin");
		}
		temp = args[2];
		if(! temp.equals("null"))
		{
			mqaddress = temp;
		}
		temp = args[3];
		if(! temp.equals("null"))
		{
			mqport = temp;
		}
		temp = args[4];
		if(! temp.equals("null"))
		{
			mqtopicMonitor = temp;
		}
		temp = args[5];
		if(! temp.equals("null"))
		{
			machineName = temp;
		}
		for(int i=6; i<args.length; i++)
		{
			processIds.add(new Integer(args[i]));
		}

		
		MonitorForErrors check = new MonitorForErrors();
		check.startMonitor();
	}
	
	
	public void startMonitor()
	{
		try{
		
			if(mqaddress != null)
			{
				System.out.println("Establishing the first connection to mq monitor");
				MqttConnection mqttconnection = new MqttConnection("tcp://" + mqaddress + ":" + mqport);
				MqttConnection mqttconnection2 = new MqttConnection("tcp://" + mqaddress + ":" + mqport);
				mqmonitor = mqttconnection.getClientInMemoryPersistence(machineName + UUID.randomUUID().toString().substring(0, 10).replace('-', '_'));
				producer = mqttconnection2.getClientInMemoryPersistence(machineName + UUID.randomUUID().toString().substring(0, 9).replace('-', '_'));
				topic = producer.getTopic(mqtopicMonitor);
				mqproducerOptions.setCleanSession(true);
				mqmonitorOptions.setCleanSession(false);
				mqmonitor.setCallback(this);
				producer.connect(mqproducerOptions);
				mqmonitor.connect(mqmonitorOptions);
				mqmonitor.subscribe(mqtopicMonitor);
				
				
			
			}
			
			File directory = new File("./errors");
			imaServerStatusA.connectToServer();
			numberFilesOnA = imaServerStatusA.getFileNames().length;
			if(imaServerStatusB != null)
			{
				imaServerStatusB.connectToServer();
				numberFilesOnB = imaServerStatusB.getFileNames().length;
			}
			do
			{
				try
				{
					Thread.sleep(30000);
				
					File[] contents = directory.listFiles();
				
					if(mqmonitor != null && ! mqmonitor.isConnected())
					{
						System.out.println("Connecting to mq");
						mqmonitor.setCallback(this);
						mqmonitor.connect(mqmonitorOptions);
						mqmonitor.subscribe(mqtopicMonitor);
						// lets see if we get a message
					}
				
					if (contents.length == 0) {
					// 	do nothing
					}
					else {
					
						for(int i=0; i< processIds.size(); i++)
						{
							System.out.println("Errors were reported by other tests");
							int processid = processIds.get(i);
							System.out.println("Attempting to kill process id " + processid);
							Runtime.getRuntime().exec("kill " + processid).waitFor();
						}
						
						this.sendMessageToMQMonitor();
						break;
					}
				
					if(!imaServerStatusA.isConnected())
					{
						imaServerStatusA.connectToServer();
					}
					if(imaServerStatusA.getFileNames().length != numberFilesOnA)
					{
						System.out.println("The number of files did not match for server A");
						for(int i=0; i< processIds.size(); i++)
						{
							int processid = processIds.get(i);
							System.out.println("Attempting to kill process id " + processid);
							Runtime.getRuntime().exec("kill " + processid).waitFor();
						}
						System.out.println("Now writing a file to errors to stop any processes watching that directory");
						File audit = new File("./errors/Monitor.txt");
						if(!audit.exists())
						{
							audit.createNewFile();
						}
						PrintStream writer = new PrintStream("./errors/Monitor.txt", "UTF-8");
						writer.println("An error occurred during testing");
						writer.flush();
						writer.close();
						this.sendMessageToMQMonitor();
						break;
					}
					if(imaServerStatusB != null)
					{
						if(!imaServerStatusB.isConnected())
						{
							imaServerStatusB.connectToServer();
						}
						if(imaServerStatusB.getFileNames().length != numberFilesOnB)
						{
							System.out.println("The number of files did not match for server B");
							for(int i=0; i< processIds.size(); i++)
							{
								int processid = processIds.get(i);
								System.out.println("Attempting to kill process id " + processid);
								Runtime.getRuntime().exec("kill " + processid).waitFor();
							}
							System.out.println("Now writing a file to errors to stop any processes watching that directory");
							File audit = new File("./errors/Monitor.txt");
							if(!audit.exists())
							{
								audit.createNewFile();
							}
							PrintStream writer = new PrintStream("./errors/Monitor.txt", "UTF-8");
							writer.println("An error occurred during testing");
							writer.flush();
							writer.close();
							
							this.sendMessageToMQMonitor();
							break;
						}
					}
				
					if(mqMessageReceived)
					{
						for(int i=0; i< processIds.size(); i++)
						{
							System.out.println("Errors were reported by the mq monitor");
							int processid = processIds.get(i);
							System.out.println("Attempting to kill process id " + processid);
							Runtime.getRuntime().exec("kill " + processid).waitFor();
						}
						System.out.println("Now writing a file to errors to stop any processes watching that directory");
						File audit = new File("./errors/MqMonitor.txt");
						if(!audit.exists())
						{
						audit.createNewFile();
						}
						PrintStream writer = new PrintStream("./errors/MqMonitor.txt", "UTF-8");
						writer.println("An error occurred during testing");
						writer.flush();
						writer.close();
						break;
					}
					else
					{
						if(mqmonitor != null && mqmonitor.isConnected())
						{
							mqmonitor.disconnect();
						}
					}
				
				}
				catch(Exception e)
				{
					e.printStackTrace();
				}
		
			}while(true);
			
			if(mqmonitor != null && mqmonitor.isConnected())
			{
				
				mqmonitor.disconnect();
				
			}
			
		}catch(Exception me)
		{
			// do nothing
			me.printStackTrace();
		}
		
	}
	
	
	public void sendMessageToMQMonitor()
	{
		if(!mqMessageReceived)
		{
			System.out.println("Attempting to send a message to MQ");
			try
			{
				if(! producer.isConnected())
				{
					try
					{
						producer.connect(mqmonitorOptions);
					}
					catch(MqttException e)
					{
					// 	do nothing
					}
				}
				MqttMessage msg = new MqttMessage((machineName + " reported an error in the tests").getBytes());
				msg.setQos(2);
				try
				{
					topic.publish(msg);
				}
				catch(Exception e)
				{
					// try again
					try {
						Thread.sleep(5000);
					} catch (InterruptedException e1) {
						// do nothing
					}
					topic.publish(msg);
				}
				System.out.println("Message sent to warn other tests of failure");
				producer.disconnect(60000);
			}
			catch(MqttException me)
			{
				System.out.println("We were unable to send a message to the mq monitor");
				me.printStackTrace();
			}
		}
		else
		{
			System.out.println("We received a message from mq so no need to send one");
		}
	}


	public void connectionLost(Throwable arg0) {
		//do nothing
		
	}


	public void deliveryComplete(IMqttDeliveryToken arg0) {
		// do nothing
		
	}


	public void messageArrived(String arg0, MqttMessage arg1) throws Exception {
		
		System.out.println("A message arrived on the mq monitor " + new String(arg1.getPayload()));
		mqMessageReceived = true;
		
	}

}
