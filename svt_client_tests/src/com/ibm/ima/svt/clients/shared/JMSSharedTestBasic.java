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
 * A separate connection will connect a specified number of shared consumers to the subscription
 * under a single connection.
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

import javax.jms.JMSException;

public class JMSSharedTestBasic {
	/**
	 * @param args
	 * @throws JMSException 
	 * @throws InterruptedException 
	 * @throws IOException 
	 */
	
	static int numberOfClients;
	static String IP = null;
	static String port = null;
	static int messageCache;
	static String clientID = null;
	static String topic = null;
	static JMSSharedClient[] sharedArray;
	static PrintWriter output;
	static JMSConnection conn;
	static int total;
	static int messagesToSend ;
	static int retryInterval;
	
	public static void main(String[] args) throws JMSException, InterruptedException, IOException {
		
		String publisherID = null;

		if(args.length == 9){
			IP = args[0];
			port = args[1];
			clientID = args[2];
			topic = args[3];
			messageCache = Integer.parseInt(args[4]);
			numberOfClients = Integer.parseInt(args[5]);
			publisherID = args[6];
			messagesToSend = Integer.parseInt(args[7]);
			retryInterval = Integer.parseInt(args[8]);
		} else {
			System.out.println("Please input arguments: IP, Port, Client ID, Topic, Client Message Cache, Number of Clients, Publisher ID, Messages to send, and Retry Interval (seconds)");
			System.out.println("For example: 9.20.87.35 16230 Client1 sharedTopic 0 5 Publisher1 100000 15\n");
			System.out.println("This will create a client called 'Client1' subscribing to 'sharedTopic',\nwith a Message Cache of 0 and 5 subscribers.\n100000 messages will be sent from 'Publisher1', and total messages\nreceived will be checked and recorded every 15 seconds.");
			System.exit(0);
		}
		
		String fileName = clientID + "_Subscribing_on_" + topic.replace("/", "{slash}");
		PrintWriter output = new PrintWriter(new FileWriter(fileName+".txt"));
		System.out.println("Writing results to file: " + fileName + ".txt");

		output.println(new Timestamp(new java.util.Date().getTime()) + " Params used: " +
				"Host = " + IP + ":" + port + ", ClientID = " + clientID +
				", Topic = " + topic + " MessageCache = " + messageCache +
				", Number of Clients = " + numberOfClients +
				", PublisherID = " + publisherID +
				", Messages sent = " + messagesToSend +
				", Retry interval = " + retryInterval + " seconds.");
		
		output.flush();
		
		JMSConnection placeholderConn = new JMSConnection(IP, port, messageCache, topic);
		output.println(new Timestamp(new java.util.Date().getTime()) + " Creating a placeholder subscription to buffer messages...");
		output.flush();
		JMSSharedClient placeholderSubscription = new JMSSharedClient(placeholderConn, fileName);
		placeholderSubscription.createSharedDurableSubscriber();
		placeholderConn.session.close();
		placeholderConn.connection.close();
		
		//SENDER
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
		
		JMSConnection conn = new JMSConnection(IP, port, messageCache, topic);
		output.println(new Timestamp(new java.util.Date().getTime()) + " Creating a shared subscription with " + numberOfClients + " clients.");
		output.flush();
		JMSSharedClient[] sharedArray = new JMSSharedClient[numberOfClients];
		
		for(int i=0;i<=numberOfClients-1;i++){
			sharedArray[i] = new JMSSharedClient(conn);
			sharedArray[i].createSharedDurableSubscriber();
			if (numberOfClients < 10){
				output.println(new Timestamp(new java.util.Date().getTime()) + " Client " + i + " created...");
			} else if (numberOfClients < 100){
				if (i%10 == 0 || i == numberOfClients-1){
					output.println(new Timestamp(new java.util.Date().getTime()) + " Client " + i + " created...");
					output.flush();
				}
			} else if (numberOfClients < 1000) {
				if (i%100 == 0 || i == numberOfClients-1){
					output.println(new Timestamp(new java.util.Date().getTime()) + " Client " + i + " created...");
					output.flush();
				}
			} else if (numberOfClients > 1000){
				if (i%1000 == 0 || i == numberOfClients-1){
					output.println(new Timestamp(new java.util.Date().getTime()) + " Client " + i + " created...");
					output.flush();
				}
			}
		}
		output.println(new Timestamp(new java.util.Date().getTime()) + " Clients created");
		output.flush();
		
		sharedArray[numberOfClients-1].connect();
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
			
			total = sharedArray[0].getGlobalCount();
			
			output.println(new Timestamp(new java.util.Date().getTime()) + " Messages received so far = " + total + "... Sleeping for more time!");
			output.flush();

			//This 'ring buffer' is to stop the client getting into a loop
			//where messages will never be received.
			if (totalRingBuffer[0] == totalRingBuffer[2])
				messagesStillBeingDelivered = false;
			
			if (ringBufferCount == 3)
				ringBufferCount = 0;
		}
		
		for(int i=0;i<=numberOfClients-1;i++){
			output.println(new Timestamp(new java.util.Date().getTime()) + " " + "Client "+ i + " = " + String.valueOf(sharedArray[i].count));
			output.flush();
		}
		
		total = sharedArray[0].getGlobalCount();
		output.println(new Timestamp(new java.util.Date().getTime()) + " Message Total = " + total);
		output.flush();
		output.close();
		
		System.exit(0);
	}
}
