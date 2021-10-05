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
package com.ibm.ima.mqcon.msgconversion;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.Topic;

import com.ibm.ima.mqcon.utils.CompareMessageContents;
import com.ibm.ima.mqcon.utils.JmsMessage;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.Trace;

/**
 * Test sends a JMS Map message to an MQ topic
 * which is mapped to an IMA Topic.
 * 
 * It checks that the text sent in the message body
 * is correctly preserved.
 * 
 * Subscriber = non durable and synchronous receive
 * Session = non transacted and auto acknowledgement
 * Message = Text and non persistent
 * Headers = non specifically set *
 *
 */
public class JmsMap_MqToIma extends JmsToJms{

	
	public static void main(String[] args) {
		
		new Trace().traceAutomation();
		JmsMap_MqToIma testClass = new JmsMap_MqToIma();
		testClass.setNumberOfMessages(1);
		testClass.setNumberOfPublishers(1);
		testClass.setNumberOfSubscribers(1);
		testClass.setTestName("JmsMap_MqToIma");
		
		// Get the hostname and port passed in as a parameter
		try
		{
			if (args.length==0)
			{
				if(Trace.traceOn())
				{
					Trace.trace("\nTest aborted, test requires command line arguments!! (IMA IP address, IMA port, MQ IP address, MQ port, MQ queue manager, IMA topic, MQ topic, timeout)");
				}
				RC = ReturnCodes.INCORRECT_PARAMS;
				System.exit(RC);
			}
			else if(args.length == 8)
			{
				imahostname = args[0];
				imaport = new Integer((String)args[1]);
				mqhostname = args[2];
				mqport = new Integer((String)args[3]);
				mqqueuemanager = args[4];
				imadestinationName = args[5];
				mqDestinationName = args[6];
				timeout = new Integer((String)args[7]);
				
				if(Trace.traceOn())
				{
					Trace.trace("The arguments passed into test class are: " + imahostname + ", " + imaport + ", " + mqhostname + ", " + mqport + ", " + mqqueuemanager + ", " + imadestinationName + ", " + mqDestinationName + ", " + timeout);
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
			
			testClass.sendJMSMapMessage();
									
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}	
	}
	
	
	public void sendJMSMapMessage()
	{
		try
		{
			if(Trace.traceOn())
			{
				Trace.trace("Creating the default producer session for MQ and consumer session for IMA");
			}
			
			Session producerSession = producerJMSSession.getSession(JmsSession.CLIENT_TYPE.MQ, mqhostname, mqport, mqqueuemanager);
			Session consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null);
			
			Message msg = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.MAP);
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message producer");
			}
			Topic mqDestination = producerSession.createTopic(mqDestinationName);
			MessageProducer producer = producerSession.createProducer(mqDestination);
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message consumer and starting the connection");
			}
			Topic imaDestination = consumerSession.createTopic(imadestinationName);
			MessageConsumer consumer = consumerSession.createConsumer(imaDestination);
			consumerJMSSession.startConnection();
			
			if(Trace.traceOn())
			{
				Trace.trace("Sending the message to MQ");
			}
			producer.send(msg);
			
			if(Trace.traceOn())
			{
				Trace.trace("Waiting for the message to arrive on IMA");
			}
			Message imaMessage = consumer.receive(timeout);
			
			if(imaMessage == null)
			{
				if(Trace.traceOn())
				{
					Trace.trace("No message arrived at IMA within the specified timeout. This test failed");
				}
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				closeConnections();
				System.exit(RC);
			}
			else
			{
				if(CompareMessageContents.checkJMSMessagebodyEquality(msg, imaMessage))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message has passed");
					}
					RC = ReturnCodes.PASSED;
					System.out.println("Test.*Success!");
					closeConnections();
					System.exit(RC);
				}
				else
				{
					if(Trace.traceOn())
					{
						Trace.trace("The message type or contents were not the same. Text failed");
					}
					RC = ReturnCodes.INCORRECT_MESSAGE;
					closeConnections();
					System.exit(RC);
				}
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
	
	

}

