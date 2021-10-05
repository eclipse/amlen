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
package com.ibm.ima.svt.clients.shared;

/**
 * This test will create a durable shared subscription, then publish a specified number of
 * non-durable messages to the subscription. Messages will be buffered on the subscription.
 * 
 * A separate connection will connect a single shared consumer to the subscription
 * under a single connection. The consumer is then started and will receive messages.
 * 
 * Consumers are created randomly, up the the maximum specified. *This is known to be against
 * JMS 'best practice'.*
 * 
 * Outputs are logged to a text file. The subscribing client is stopped when it is detected
 * that no messages are being received. This is checked on every 'retryInterval'. If no messages
 * have been received within the 3 last 'retryIntervals', then messaging is deemed to have
 * stopped.
 * 
 * NOTE: The ClientID in this test is used purely for naming the text file to which outputs
 * are saved to. Clients are actually created in the Global-Shared space.
 */

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.sql.Timestamp;
import java.util.Random;

import javax.jms.JMSException;

public class JMSSharedTestRandomConnect {
	/**
	 * @param args
	 * @throws JMSException 
	 * @throws InterruptedException 
	 * @throws IOException 
	 */
	static int 					maxNumberOfClients = 0;
	static int 					currentClientsConnected = 1;
	
	static String 				IP = null;
	static String 				port = null;
	static int 					messageCache = 0;
	static String 				clientID = null;
	static String 				topic = null;
	static JMSSharedClient[] 	sharedArray;
	static PrintWriter 			output;
	static JMSConnection 		conn;
	static String 				fileName;
	
	
	public static void main(String[] args) throws JMSException, InterruptedException, IOException {
		
		String publisherID = null;
		
		int total = 0;
		int messagesToSend = 0;
		int retryInterval = 0;
		
		if(args.length == 9){
			IP = args[0];
			port = args[1];
			clientID = args[2];
			topic = args[3];
			messageCache = Integer.parseInt(args[4]);
			maxNumberOfClients = Integer.parseInt(args[5]);
			publisherID = args[6];
			messagesToSend = Integer.parseInt(args[7]);
			retryInterval = Integer.parseInt(args[8]);
		} else {
			System.out.println("Please input arguments: IP, Port, Client ID, Topic, Client Message Cache, Number of Clients, Publisher ID, Messages to send, Retry Interval (seconds)");
			System.out.println("For example: 9.20.87.35 16230 Client1 sharedTopic 0 5 Publisher1 100000 15\n");
			System.out.println("This will create a client called 'Client1' subscribing to 'sharedTopic',\nwith a Message Cache of 0 and 5 subscribers.\n100000 messages will be sent from 'Publisher1', and total messages\nreceived will be checked and recorded every 15 seconds.");
			System.exit(0);
		}
		
		fileName = clientID + "_Subscribing_on_" + topic.replace("/", "{slash}");
		output = new PrintWriter(new FileWriter(fileName+".txt"));
		System.out.println("Writing results to file: " + fileName + ".txt");

		output.println(new Timestamp(new java.util.Date().getTime()) + " Params used: " +
				"Host = " + IP + ":" + port + ", ClientID = " + clientID +
				", Topic = " + topic + " MessageCache = " + messageCache +
				", Number of Clients = " + maxNumberOfClients +
				", PublisherID = " + publisherID +
				", Messages sent = " + messagesToSend +
				", Retry interval = " + retryInterval + " seconds.");
		
		output.flush();
		JMSConnection placeholderConn = new JMSConnection(IP, port, messageCache, topic);
		output.println(new Timestamp(new java.util.Date().getTime()) + " Creating a placeholder subscription to buffer messages...");
		output.flush();
		JMSSharedClient placeholderSubscription = new JMSSharedClient(placeholderConn);
		placeholderSubscription.createSharedDurableSubscriber();
		placeholderConn.session.close();
		placeholderConn.connection.close();
		
		//Arguments for publisher
		String[] client2args = new String[10];
		client2args[0] = "-s";client2args[1] = "tcp://" + IP + ":" + port;
		client2args[2] = "-a";client2args[3] = "publish";
		client2args[4] = "-t";client2args[5] = topic;
		client2args[6] = "-n";client2args[7] = String.valueOf(messagesToSend);
		client2args[8] = "-i";client2args[9] = publisherID;
		
		//Not run in new Thread as topic is to be 'pre-loaded' with messages.
		JMSSample producer = new JMSSample(client2args);
		producer.run();
		producer.close();
		
		conn = new JMSConnection(IP, port, messageCache, topic);
		
		output.println(new Timestamp(new java.util.Date().getTime()) + " Creating a shared subscription with " + maxNumberOfClients + " clients.");
		output.flush();
		sharedArray = new JMSSharedClient[maxNumberOfClients];
		
		//Create and connect initial subscriber
		sharedArray[0] = new JMSSharedClient(conn, fileName);
		sharedArray[0].createSharedDurableSubscriber();
		sharedArray[0].connect();
		
		output.println(new Timestamp(new java.util.Date().getTime()) + " Initial client created and connected.");
		output.flush();
		
		output.println(new Timestamp(new java.util.Date().getTime()) + " Clients connected");
		output.flush();
		
		boolean messagesStillBeingDelivered = true;
		int totalRingBuffer[] = new int[3];
		totalRingBuffer[2] = 1;
		int ringBufferCount = 0;
		
		while (messagesStillBeingDelivered){
			totalRingBuffer[ringBufferCount] = total;
			ringBufferCount++;
			Thread.sleep(retryInterval*1000);
			
			connectRandomClients();
			
			total = sharedArray[0].getGlobalCount();
			
			output.println(new Timestamp(new java.util.Date().getTime()) + " Messages received so far = " + total + "... Sleeping for more time!");
			output.flush();
			
			/*
			 * This 'ring buffer' is to stop the client getting into a loop
			 * where messages will never be received.
			 */
			if (totalRingBuffer[0] == totalRingBuffer[2])
				messagesStillBeingDelivered = false;
			
			if (ringBufferCount == 3)
				ringBufferCount = 0;
		}
		
		for(int i=0;i<=maxNumberOfClients-1;i++){
			if(sharedArray[i]!=null)
				output.println(new Timestamp(new java.util.Date().getTime()) + " " + "Client "+(i+1) + " = " + String.valueOf(sharedArray[i].count));
			else
				output.println(new Timestamp(new java.util.Date().getTime()) + " " + "Client "+(i+1) + " = 0");

			output.flush();
		}
		
		total = sharedArray[0].getGlobalCount();
		output.println(new Timestamp(new java.util.Date().getTime()) + " Message Total = " + total);
		output.flush();
		
		disconnectAllClients();
		
		System.exit(0);
	}
	
	/*
	 * Function connects clients 'randomly' up to the maximum number of allowed clients.
	 * Clients are added sequentially to the array.
	 */
	public static void connectRandomClients(){
		Random r = new Random();
		
		for(int i=0;i<=maxNumberOfClients-1;i++){
			int nextInt = r.nextInt(1000-1);
			if(nextInt % 5 == 0){
				if(sharedArray[i] == null){
					sharedArray[currentClientsConnected] = new JMSSharedClient(conn);
					sharedArray[currentClientsConnected].createSharedDurableSubscriber();
					output.println(new Timestamp(new java.util.Date().getTime()) + " Client " + currentClientsConnected + " created...");
					currentClientsConnected++;
					output.flush();
				}
			}
		}
	}
	
	//All clients that are currently connected get disconnected, then closes connection.
	public static void disconnectAllClients(){
		for(int i=0;i<=currentClientsConnected-1;i++){
			sharedArray[i].disconnect();
			output.println(new Timestamp(new java.util.Date().getTime()) + " Client " + i + " disconnected...");
		}
		try {
			conn.connection.close();
		} catch (JMSException e) {
			e.printStackTrace();
		}
	}
}
