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
import javax.jms.Session;
import javax.jms.Topic;

import com.ibm.ima.mqcon.msgconversion.JmsToJms;
import com.ibm.ima.mqcon.msgconversion.ReturnCodes;
import com.ibm.ima.mqcon.utils.CompareMessageContents;
import com.ibm.ima.mqcon.utils.JmsMessage;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.Trace;

/*** Test sends a JMS Text message to IMA topic
* and is received from that same topic
* 
* It checks that the text sent in the message body
* is correctly preserved.
* 
* Subscriber = non durable and synchronous receive
* Session = non transacted and auto acknowledgement
* Message = Text and non persistent
* Headers = non specifically set
*
*/
public class JmsAcks_ImaToMq extends JmsToJms{

	

	public static void main(String[] args) {
		
		new Trace().traceAutomation();
		JmsAcks_ImaToMq testClass = new JmsAcks_ImaToMq();
		testClass.setNumberOfMessages(3);
		testClass.setNumberOfPublishers(1);
		testClass.setNumberOfSubscribers(1);
		testClass.setTestName("JmsAcks_ImaToMq");
		
		// Get the hostname and port passed in as a parameter
		try
		{
			if (args.length==0)
			{
				if(Trace.traceOn())
				{
					Trace.trace("Test aborted, test requires command line arguments!! (IMA IP address, IMA port, MQ IP address, MQ port, MQ queue manager, IMA topic, MQ topic, timeout)");
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
					Trace.trace("Invalid Commandline arguments! System Exit.");
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

			Session producerSession = producerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, false, Session.CLIENT_ACKNOWLEDGE);
			Session consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.MQ, mqhostname, mqport, mqqueuemanager, false, Session.CLIENT_ACKNOWLEDGE);
			
			Message msg1 = JmsMessage.createMessageBody(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			Message msg2 = JmsMessage.createMessageBody(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			Message msg3 = JmsMessage.createMessageBody(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message producer");
			}
			Topic imaDestination = producerSession.createTopic(imadestinationName);
			MessageProducer producer = producerSession.createProducer(imaDestination);
			
			Topic mqDestination = consumerSession.createTopic(mqDestinationName);
			MessageConsumer consumer = consumerSession.createConsumer(mqDestination);
			consumerJMSSession.startConnection();
			
			if(Trace.traceOn())
			{
				Trace.trace("Sending three messages to IMA");
			}
			producer.send(msg1);
			producer.send(msg2);
			producer.send(msg3);
			
			if(Trace.traceOn())
			{
				Trace.trace("Waiting for the three messages to arrive on MQ and check they are correct");
			}
			Message mqMessage1 = consumer.receive(timeout);
			Message mqMessage2 = consumer.receive(timeout);
			Message mqMessage3 = consumer.receive(timeout);
			
			if(mqMessage1 == null)
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
			
				if(CompareMessageContents.checkJMSMessagebodyEquality(msg1, mqMessage1))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message 1 has passed equality checking for body");
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
			
			if(mqMessage2 == null)
			{
				if(Trace.traceOn())
				{
					Trace.trace("No second message arrived at MQ within the specified timeout. This test failed");
				}
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				closeConnections();
				System.exit(RC);
			}
			else
			{
			
				if(CompareMessageContents.checkJMSMessagebodyEquality(msg2, mqMessage2))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message 2 has passed equality checking for body");
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
			
			if(mqMessage3 == null)
			{
				if(Trace.traceOn())
				{
					Trace.trace("No message 3 arrived at MQ within the specified timeout. This test failed");
				}
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				closeConnections();
				System.exit(RC);
			}
			else
			{
			
				if(CompareMessageContents.checkJMSMessagebodyEquality(msg3, mqMessage3))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message 3 has passed equality checking for body");
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
				
		

			if(Trace.traceOn())
			{
				Trace.trace("Now recover the session and check the producer sends all three messages again");
			}
			consumerSession.recover();
			
			
			
			mqMessage1 = consumer.receive(timeout);
			mqMessage2 = consumer.receive(timeout);
			mqMessage3 = consumer.receive(timeout);
			
			if(mqMessage1 == null)
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
			
				if(CompareMessageContents.checkJMSMessagebodyEquality(msg1, mqMessage1))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message 1 has passed equality checking for body");
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
			
			if(mqMessage2 == null)
			{
				if(Trace.traceOn())
				{
					Trace.trace("No second message arrived at MQ within the specified timeout. This test failed");
				}
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				closeConnections();
				System.exit(RC);
			}
			else
			{
			
				if(CompareMessageContents.checkJMSMessagebodyEquality(msg2, mqMessage2))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message 2 has passed equality checking for body");
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
			
			if(mqMessage3 == null)
			{
				if(Trace.traceOn())
				{
					Trace.trace("No message 3 arrived at MQ within the specified timeout. This test failed");
				}
				
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				closeConnections();
				System.exit(RC);
			}
			else
			{
			
				if(CompareMessageContents.checkJMSMessagebodyEquality(msg3, mqMessage3))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message 3 has passed equality checking for body so we are going to pass the test");
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

