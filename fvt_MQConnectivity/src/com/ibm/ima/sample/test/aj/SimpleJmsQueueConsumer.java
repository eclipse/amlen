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

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
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
	

public class SimpleJmsQueueConsumer {
	
	private ConnectionFactory consumerCF;
	private Connection consConn;
	private Session consSess;
	private MessageConsumer cons;
	private Queue qu;
	
	private static int RC = 0;
	private static int totalMsgNumber = 30;
	private static String qname = "ARSENAL";
	private static String expectedMsg = "AJ_MSG";
		
	public static void main(String[] args) {
		String ip = "", prt = "";
		SimpleJmsQueueConsumer queCon = new SimpleJmsQueueConsumer();
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
			expectedMsg = args[4];
		}
		else
		{
			System.out.println("\nInvalid number of commandline arguments! Test Aborted");
			RC=ReturnCodes.ERROR_INVALID_NUMBER_OF_COMMANDLINE_ARGS;
			System.exit(RC);
		}
		
		queCon.createConsumer(ip, prt);
		queCon.receiveMsgs(totalMsgNumber);
		queCon.closeDown();
	}

	private void createConsumer(String ip, String prt) 
	{
		System.out.println("Creating a consumer");
		try {	
			consumerCF = ImaJmsFactory.createConnectionFactory();
			((ImaProperties) consumerCF).put("Server", ip);
			((ImaProperties) consumerCF).put("Port", prt);
			
			consConn = consumerCF.createConnection();
			String consumerClientID = "AJ_Consumer";
			consConn.setClientID(consumerClientID);
			consSess = consConn.createSession(false, Session.AUTO_ACKNOWLEDGE);
			qu = consSess.createQueue(qname);
			cons = consSess.createConsumer(qu);
			
			consConn.start();
			System.out.println("Subscribing MessageSight IP: "+ip+" and Port: "+prt+" with clientID: "+ consumerClientID);
		} 
		catch (JMSException e) {
			e.printStackTrace();
		}
	}
	
	private void receiveMsgs(int msgNumber) {
		int msgCount = 0;
		try {
			while (msgCount<totalMsgNumber)
			{
				Message msg;
				msg = cons.receive(3000);
				if (msg == null)
				{
					System.out.print(".");
					Thread.sleep(500);
				}
				else
				{
					msgCount ++;
					if( ((TextMessage)msg).getText().contains(expectedMsg))
					{
						System.out.print("\n" + msgCount+": "+((TextMessage)msg).getText());
					}
					else
					{
						System.out.println("Invalid Msg received. Msg: " + ((TextMessage)msg).getText());
						RC = ReturnCodes.ERROR_INVALID_MSG_RECEIVED;
					}
				}
			}
		}
		catch(JMSException e) {
			e.printStackTrace();
		} 
		catch (InterruptedException e) {
			e.printStackTrace();
		}
	}
	

	private void closeDown() {
		try{
			consConn.close();
		}
		catch(JMSException e) {
			e.printStackTrace();
		}
	}

}
