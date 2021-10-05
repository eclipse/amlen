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
import javax.jms.MessageProducer;
import javax.jms.Queue;
import javax.jms.Session;
import javax.jms.TextMessage;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;


/*	Simple Producer- 
 * 		Creates a simple producer and sends messages to a queue. 
 * 	
 * Author:
 * Created on: 30/07/2013 
 * 
 * */
	

public class SimpleJmsQueueProducer {
	
	private ConnectionFactory producerCF;
	private Connection prodConn;
	private Session prodSess;
	private MessageProducer prod;
	private Queue qu;
	
	private static int RC = 0;
	private static int totalMsgNumber = 30;
	private static String qname = "ARSENAL";
	private static String msgBody = "AJ_MSG";
		
	public static void main(String[] args) {
		String ip = "", prt = "";
		SimpleJmsQueueProducer queProd = new SimpleJmsQueueProducer();
		if (args.length==0)
		{
			System.out.println("\nTest aborted, test requires command line arguments!! (ip, port, qname, no. of msgs)");
			RC=ReturnCodes.ERROR_NO_COMMANDLINE_ARGS;
			System.exit(RC);
		}
		else if(args.length == 5)
		{
			ip = args[0];
			prt = args[1];
			qname  = args[2]; 
			totalMsgNumber=Integer.parseInt(args[3]);
			msgBody = args[4];
		}
		else
		{
			System.out.println("\nInvalid number of commandline arguments! Test Aborted");
			RC=ReturnCodes.ERROR_INVALID_NUMBER_OF_COMMANDLINE_ARGS;
			System.exit(RC);
		}
		
		queProd.createProducer(ip, prt);
		queProd.sendMessages();
		queProd.closeDown();
	}


	private void createProducer(String ip, String prt) {
		System.out.println("Started created a producer");
		try{
			producerCF = ImaJmsFactory.createConnectionFactory();
			((ImaProperties) producerCF).put("Server", ip);
	        ((ImaProperties) producerCF).put("Port", prt);

	        prodConn = producerCF.createConnection();
			String producerClientID = UUID.randomUUID().toString().substring(0, 10);;
			prodConn.setClientID(producerClientID);
			
			prodSess = prodConn.createSession(false, Session.AUTO_ACKNOWLEDGE);
			qu = prodSess.createQueue(qname);
			
			prod = prodSess.createProducer(qu);
			prod.setDeliveryMode(DeliveryMode.NON_PERSISTENT); 
						
		} 
		catch (JMSException e) {
			e.printStackTrace();
		}
		
	}

	private void sendMessages() {
		int i;
		
		TextMessage msg;
		for(i=1;i<=totalMsgNumber;i++)
		{			
			try{
				msg = prodSess.createTextMessage(msgBody+Integer.toString(i));
				msg.setJMSDeliveryMode(DeliveryMode.NON_PERSISTENT);
				prod.send(msg);
			}
			catch(JMSException e){
				e.printStackTrace();
			}
		}
		if(i==totalMsgNumber)
			System.out.println("ALL MESSAGES SENT");
	}
	

	private void closeDown() {
		System.out.println("Closing down everything");
		try {
			prodConn.close();
		} catch (JMSException e) {
			e.printStackTrace();
		}		
	}
}
