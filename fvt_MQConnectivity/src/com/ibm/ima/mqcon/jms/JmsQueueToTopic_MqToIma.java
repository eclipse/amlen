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
package com.ibm.ima.mqcon.jms;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Queue;
import javax.jms.Session;
import javax.jms.Topic;

import com.ibm.ima.mqcon.msgconversion.JmsToJms;
import com.ibm.ima.mqcon.msgconversion.ReturnCodes;
import com.ibm.ima.mqcon.utils.CompareMessageContents;
import com.ibm.ima.mqcon.utils.JmsMessage;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.ima.mqcon.utils.Trace;

/*** 
* Test checks that a messages are sent correctly
* across the bridge from ima queue to mq topic
* 
* 
* Subscriber = non durable and synchronous receive
* Session = non transacted and auto acknowledgement
* Message = all types
* Headers = set
*
*/
public class JmsQueueToTopic_MqToIma extends JmsToJms {
	
	private int messageNum=0;

public static void main(String[] args) {
		
		new Trace().traceAutomation();
		JmsQueueToTopic_MqToIma testClass = new JmsQueueToTopic_MqToIma();
		testClass.setNumberOfMessages(5);
		testClass.setNumberOfPublishers(1);
		testClass.setNumberOfSubscribers(1);
		testClass.setTestName("JmsQueueToTopic_MqToIma");
		
		// Get the hostname and port passed in as a parameter
		try
		{
			if (args.length==0)
			{
				if(Trace.traceOn())
				{
					Trace.trace("\nTest aborted, test requires command line arguments!! (IMA IP address, IMA port, MQ IP address, MQ port, MQ queue manager, IMA topic, MQ queue, timeout)");
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
			
			testClass.testMessageAck();
									
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}	
	}
	
	
	public void testMessageAck()
	{
		try
		{
			if(Trace.traceOn())
			{
				Trace.trace("Creating the default producer session for IMA and consumer session for MQ with CLIENT_ACKNOWLEDGEMENT");
			}

			Session producerSession = producerJMSSession.getSession(JmsSession.CLIENT_TYPE.MQ, mqhostname, mqport, mqqueuemanager, false, Session.CLIENT_ACKNOWLEDGE);
			Session consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, false, Session.CLIENT_ACKNOWLEDGE);
			
			Message msg1 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			Message msg2 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.BYTES);
			Message msg3 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.OBJECT);
			Message msg4 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.STREAM);
			Message msg5 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.MAP);
		
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message producer");
			}
			Queue mqDestination = producerSession.createQueue(mqDestinationName);
			MessageProducer producer = producerSession.createProducer(mqDestination);
			
			Topic imaDestination = consumerSession.createTopic(imadestinationName);
			MessageConsumer consumer = consumerSession.createConsumer(imaDestination);
			consumerJMSSession.startConnection();
			
			if(Trace.traceOn())
			{
				Trace.trace("Sending five messages to MQ");
			}
			producer.send(msg1);
			producer.send(msg2);
			producer.send(msg3);
			producer.send(msg4);
			producer.send(msg5);
		
						
			if(Trace.traceOn())
			{
				Trace.trace("Waiting for the five messages to arrive on IMA and check they are correct");
			}
			Message mqMessage1 = consumer.receive(timeout);
			Message mqMessage2 = consumer.receive(timeout);
			Message mqMessage3 = consumer.receive(timeout);
			Message mqMessage4 = consumer.receive(timeout);
			Message mqMessage5 = consumer.receive(timeout);
			
			
			
			checkMessage(msg1, mqMessage1, imadestinationName);
			checkMessage(msg2, mqMessage2, imadestinationName);
			checkMessage(msg3, mqMessage3, imadestinationName);
			checkMessage(msg4, mqMessage4, imadestinationName);
			checkMessage(msg5, mqMessage5, imadestinationName);
		
			
			// do nothing and wait until message received
			StaticUtilities.sleep(8000);
			
			if(messageNum == 5)
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
	
	
	private void checkMessage(Message expected, Message actual, String topicName)
	{
		
		messageNum++;
		
		try
		{
			if(actual == null)
			{
				if(Trace.traceOn())
				{
					Trace.trace("No message arrived at MQ within the specified timeout. This test failed");
				}
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				closeConnections();
				System.exit(RC);
			}
			else
			{
				
				if(CompareMessageContents.checkJMSMessageEquality(expected, actual, JmsMessage.imaTopic+topicName, JmsMessage.imaTopic))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message " + messageNum + " has passed equality checking for body");
					}
				
					
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


