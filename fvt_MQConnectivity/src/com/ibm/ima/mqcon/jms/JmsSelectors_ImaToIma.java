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
* Headers = user property set
*
*/
public class JmsSelectors_ImaToIma extends JmsToJms{


	public static void main(String[] args) {
		
		new Trace().traceAutomation();
		JmsSelectors_ImaToIma testClass = new JmsSelectors_ImaToIma();
		testClass.setNumberOfMessages(2);
		testClass.setNumberOfPublishers(1);
		testClass.setNumberOfSubscribers(1);
		testClass.setTestName("JmsSelectors_ImaToIma");
		
		// Get the hostname and port passed in as a parameter
		try
		{
			if (args.length==0)
			{
				if(Trace.traceOn())
				{
					Trace.trace("\nTest aborted, test requires command line arguments!! (IMA IP address, IMA port, IMA topic, timeout)");
				}
				RC = ReturnCodes.INCORRECT_PARAMS;
				System.exit(RC);
			}
			else if(args.length == 4)
			{
				imahostname = args[0];
				imaport = new Integer((String)args[1]);
				imadestinationName = args[2];
				timeout = new Integer((String)args[3]);
				
				if(Trace.traceOn())
				{
					Trace.trace("The arguments passed into test class are: " + imahostname + ", " + imaport + ", "  + imadestinationName + ", " + timeout);
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
			
			testClass.testSelectors();
									
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}	
	}
	
	
	public void testSelectors()
	{
		try
		{
			if(Trace.traceOn())
			{
				Trace.trace("Creating the default producer session for IMA and consumer session for IMA");
			}
		
			Session producerSession = producerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null);
			Session consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null);
			
			Message msg = JmsMessage.createMessageBody(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			Message msg2 = JmsMessage.createMessageBody(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			
			// Set the user properties on the message
			if(Trace.traceOn())
			{
				Trace.trace("Setting _Food$=Pie & Drink=Beer on message1 and _Food$=Chips & Drink=Beer on message2");
			}
			msg.setStringProperty("_Food$", "Pie");
			msg.setStringProperty("Drink", "Beer");
			msg2.setStringProperty("_Food$", "Chips");
			msg2.setStringProperty("Drink", "Beer");
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message producer");
			}
			Topic imaDestination = producerSession.createTopic(imadestinationName);
			MessageProducer producer = producerSession.createProducer(imaDestination);
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message consumer with the selector (_Food$ LIKE 'Ch_ips') and starting the connection");
			}
			Topic imaDestination2 = consumerSession.createTopic(imadestinationName);
			MessageConsumer consumer = consumerSession.createConsumer(imaDestination2, "(_Food$ LIKE 'Ch_ps')");
			consumerJMSSession.startConnection();
			
			if(Trace.traceOn())
			{
				Trace.trace("Sending both messages to IMA");
			}
			producer.send(msg);
			producer.send(msg2);
			
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
				if(imaMessage.getStringProperty("_Food$").equals("Chips"))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message has passed the check for user property '_Food$'");
					}
					
					
					if(CompareMessageContents.checkJMSMessagebodyEquality(msg2, imaMessage))
					{
						if(Trace.traceOn())
						{
							Trace.trace("Message has passed equality checking for body");
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
					
					if(Trace.traceOn())
					{
						Trace.trace("Performing another synchronous check to make sure we don't get another message");
					}
					// Now just check we dont get the other message
					Message imaMessage2 = consumer.receive(timeout);
					if(imaMessage2 == null)
					{
						
						if(Trace.traceOn())
						{
							Trace.trace("We did't get the other message which is correct - test passed");
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
							Trace.trace("We received another message which was incorrect. Msg=" + imaMessage2);
						}
						RC = ReturnCodes.MESSAGE_RECEIVED_INCORRECTLY;
						closeConnections();
						System.exit(RC);
					}
				}
				else
				{
					if(Trace.traceOn())
					{
						Trace.trace("The wrong message was returned by the selector. User property for food was expected to be 'Chips' but the actual was " + imaMessage.getStringProperty("_Food$"));
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

