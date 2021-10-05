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
package com.ibm.ima.mqcon.boundaries;

import java.util.Random;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Queue;
import javax.jms.Session;
import javax.jms.TextMessage;

import com.ibm.ima.mqcon.msgconversion.JmsToJms;
import com.ibm.ima.mqcon.msgconversion.ReturnCodes;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.Trace;

/*** 
* Test checks that large messages are sent and received to and from an IMA Queue
* Example parameters - 9.20.123.102 16142 AshleyQueue1 60000 100 1
*/
public class LargeMessageTestJmsQueueToQueue_ImaToIma extends JmsToJms {
	
	private int messageNum=0;
	private int passedMessageCount=0;
	private int failedMessageCount=0;
	private static int noOfMessages = 1;
	private static int sizeOfMessageMB = 1;

public static void main(String[] args) {
		
		new Trace().traceAutomation();
		LargeMessageTestJmsQueueToQueue_ImaToIma testClass = new LargeMessageTestJmsQueueToQueue_ImaToIma();
		
		testClass.setNumberOfMessages(5);
		
		testClass.setNumberOfPublishers(1);
		testClass.setNumberOfSubscribers(1);
		
		testClass.setTestName("JmsQueueToQueue_ImaToIma");
		
		
		// Get the hostname and port passed in as a parameter
		try
		{
			if (args.length==0)
			{
				if(Trace.traceOn())
				{
					Trace.trace("\nTest aborted, test requires command line arguments!! (IMA IP address, IMA port, IMA queue, timeout, number of messages and size of message in mb)");
				}
				RC = ReturnCodes.INCORRECT_PARAMS;
				System.exit(RC);
			}
			else if(args.length == 6)
			{
				imahostname = args[0];
				imaport = new Integer((String)args[1]);
				imadestinationName = args[2];
				timeout = new Integer((String)args[3]);
				noOfMessages = new Integer((String)args[4]);
				sizeOfMessageMB = new Integer((String)args[5]);
				
				if(Trace.traceOn())
				{
					Trace.trace("The arguments passed into test class are: " + imahostname + ", " + imaport + ", " + imadestinationName + ", " + timeout + ", " + noOfMessages + ", " + sizeOfMessageMB);
				}
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("\nInvalid Commandline arguments! System Exit.");
				}
				RC = ReturnCodes.INCORRECT_PARAMS;
				System.exit(RC);
			}
			
			testClass.testMessageAck();
									
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}	
	}
	
	
	public void testMessageAck() throws InterruptedException
	{
		try
		{
			if(Trace.traceOn())
			{
				Trace.trace("Creating the default producer session for IMA and consumer session for IMA with CLIENT_ACKNOWLEDGEMENT");
			}

			Session producerSession = producerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, false, Session.AUTO_ACKNOWLEDGE);
			Session consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, false, Session.AUTO_ACKNOWLEDGE);

			
			Message largeMessage = createLargeMessage(producerSession, sizeOfMessageMB, 0);
			System.out.println(largeMessage);
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message producer");
			}
			Queue imaDestination = producerSession.createQueue(imadestinationName);
			
			MessageProducer producer = producerSession.createProducer(imaDestination);
			MessageConsumer consumer = consumerSession.createConsumer(imaDestination);
			
			consumerJMSSession.startConnection();
			
			if(Trace.traceOn())
			{
				Trace.trace("Sending large messages to IMA");
			}

			//Send the required amount of large JMS messages
			for(int i=1; i<=noOfMessages; i++){
				producer.send(largeMessage);
				System.out.println("Message " + (i) + " sent.");
			}
			
			System.out.println("All messages sent!");
			System.out.println("Starting receive!");
			TextMessage localTxtMsg = (TextMessage)largeMessage;
			
			//Receive all of the messages and compare their contents
//			for(int i=0; i<noOfMessages; i++){
//				Message imaMessage = consumer.receive(timeout);
//				messageNum++;
//				TextMessage receivedTxtMsg = (TextMessage)imaMessage;
//				if (localTxtMsg.getText().compareTo(receivedTxtMsg.getText()) == 0){
//					System.out.println("Message " + messageNum + "- \tBody of messages were the same.");
//					passedMessageCount++;
//				} else {
//					System.out.println("Message " + messageNum + "- \tBodies were not the same.");
//					failedMessageCount++;
//				}
//			}

			if (failedMessageCount > 0){
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				System.out.println("Test.*Fail!");
				closeConnections();
				System.exit(RC);
			}
			
			if (passedMessageCount != noOfMessages){
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				System.out.println("Test.*Fail!");
				closeConnections();
				System.exit(RC);
			}
			
			if(messageNum == noOfMessages)
			{
				RC = ReturnCodes.PASSED;
				System.out.println("Test.*Success!");
				closeConnections();
				System.exit(RC);
			}
		}
		catch(JMSException jmse)
		{
			if(Trace.traceOn())
			{
				Trace.trace("An exception occurred whilst attempting the test. Exception=" + jmse.getMessage());
			}
			jmse.printStackTrace();
			RC = ReturnCodes.JMS_EXCEPTION;
			closeConnections();
			System.exit(RC);
		}
	}
	
	public static Message createLargeMessage(Session session, int size, int extraBits) throws JMSException{
		Message msg = null;
		//Create random message of specified size
		Random randomGenerator = new Random();
		StringBuilder messageBody = new StringBuilder();
		long messageSizeBytes = size * 1024 * 1024 + extraBits;
		for(int x = 0;x<messageSizeBytes;x++){
			int randomInt = randomGenerator.nextInt(10);
			messageBody.append(randomInt);
		}
		msg = session.createTextMessage(messageBody.toString());
		if(Trace.traceOn())
		{
			Trace.trace("JMS Text Message generated that is " + size + "mb.");
		}
		return msg;
	}
	
	public static String createLargeMessage(int size, int extraBits) throws JMSException{
		//Create random message of specified size
		Random randomGenerator = new Random();
		StringBuilder messageBody = new StringBuilder();
		long messageSizeBytes = size * 1024 * 1024 + extraBits;
		for(int x = 0;x<messageSizeBytes;x++){
			int randomInt = randomGenerator.nextInt(10);
			messageBody.append(randomInt);
		}
		if(Trace.traceOn())
		{
			Trace.trace("JMS Text Message generated that is " + size + "mb.");
		}
		
		return messageBody.toString();
	}
}
