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
package com.ibm.ism.mqbridge.test;

import java.io.IOException;

import com.ibm.mq.MQEnvironment;
import com.ibm.mq.MQException;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQQueueManager;
import com.ibm.mq.MQTopic;
import com.ibm.mq.constants.CMQC;

public class MQClientTest {

	private static MQTopic mqDest;
	private static MQTopic subscriber;
	// the following are set up with default values. 
	//The values should be set from the command line using the appropriate arguments
	private static String topicName ="MQ/1";	  		//-t
	private static String hostname = "9.20.230.234"; 	//-h
	private static String channel  = "JAVA.CHANNEL"; 	//-c
	private static String port = "1414"; 				//-p
	private static String queueManager = "dttest3";		//-q

	/**
	 * step 1. subscribe to MQ topic.
	 * step 2. put message to that topic
	 * step 3. retrieve message from mq topic
	 * 
	 */
	public static void main(String[] args) {
		parseArgs(args);
		MQEnvironment.hostname = hostname;
		MQEnvironment.channel  = channel;
		MQEnvironment.port = Integer.parseInt(port);
		MQQueueManager mqqm = ConnectionDetails.getMqQueueManager(queueManager);	
		try {
			subscriber =
				mqqm.accessTopic(topicName,null,
						CMQC.MQTOPIC_OPEN_AS_SUBSCRIPTION, CMQC.MQSO_CREATE);
			mqDest =
				mqqm.accessTopic(topicName,null,
						CMQC.MQTOPIC_OPEN_AS_PUBLICATION, CMQC.MQOO_OUTPUT);
			MQMessage message = new MQMessage();
			String messageString = "Hello World";
			System.out.println("sent message is " + messageString);
			byte[] byteArray  = messageString.getBytes();
			message.write(byteArray);
			mqDest.put(message);
			mqDest.close();
			MQMessage receivedmessage = new MQMessage();
			subscriber.get(receivedmessage);
			System.out.println("received message is " + receivedmessage.readLine());
			subscriber.close();
			mqqm.close();
		} catch (MQException e) {
			e.printStackTrace();
		}
		catch (IOException e) {
			e.printStackTrace();
		}
	}

	private static void parseArgs(String[] args) {
		// iterate through arguments and assign parameters
		try {
			for (int x = 0; x< args.length;x++) {
				if (args[x].startsWith("-")){
					char parameter = args[x].charAt(1);
					x++;
					if (x >=args.length) {
						System.out.println("no value for parameter " + parameter);
						System.exit(1);
					}
					switch (parameter) {
					case 'h' :
						hostname = args[x];
						break;
					case 'p' :
						port = args[x];
						break;
					case 'c' :
						channel  = args[x];
						break;
					case 'q' :
						queueManager = args[x];	
						break;
					case 't' :
						topicName = args[x];	
						break;						
					default :
						System.out.println("parameter not recognised " + parameter);
						System.exit(1);
					}
				} else {
					System.out.println("invalid argument " + args[x]);
					System.exit(1);
				}
			}
		} catch (Exception e) {
			System.out.println("exception caught " + e.toString());
			System.exit(1);
		}
		
	}
}
