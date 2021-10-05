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

import com.ibm.ima.mqcon.msgconversion.ReturnCodes;
import com.ibm.ima.mqcon.utils.AsynchronousJmsConsumer;
import com.ibm.ima.mqcon.utils.AsynchronousJmsListener;
import com.ibm.ima.mqcon.utils.CompareMessageContents;
import com.ibm.ima.mqcon.utils.JmsMessage;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.ima.mqcon.utils.Trace;

/*** 
 * Test sends a JMS Text message to IMA topic
* and is received from that same topic but using
* an asynchronous listener, selectors and
* transactionality. 
* 
* It checks that the text sent in the message body
* is correctly preserved.
* 
* Subscriber = non durable and asynchronous receive
* Session = Transacted and auto acknowledgement
* Message = Text and non persistent
* Headers = set
*
*/
public class JmsAsynchronous_ImaToIma extends AsynchronousJmsConsumer {

	
	private static boolean completed = false;
	private static Message msg4 = null;
	

	public static void main(String[] args) {
		
		new Trace().traceAutomation();
		JmsAsynchronous_ImaToIma testClass = new JmsAsynchronous_ImaToIma();
		testClass.setNumberOfMessages(4);
		testClass.setNumberOfPublishers(1);
		testClass.setNumberOfSubscribers(1);
		testClass.setTestName("JmsAsynchronous_ImaToIma");
		
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
			
			testClass.testAsynchronousReceive();
									
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}	
	}
	
	
	public void testAsynchronousReceive()
	{
		try
		{
			if(Trace.traceOn())
			{
				Trace.trace("Creating the default producer session for IMA and consumer session for IMA");
			}
			consumerJMSSession = new JmsSession();
			producerJMSSession = new JmsSession();
			Session producerSession = producerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, true, Session.AUTO_ACKNOWLEDGE);
			Session consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, true, Session.AUTO_ACKNOWLEDGE);
			
			Message msg1 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			Message msg2 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			Message msg3 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			msg4 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			
			msg3.setStringProperty("_Food$", "Pie");
			msg3.setStringProperty("Drink", "Beer");
			msg4.setStringProperty("_Food$", "Chips");
			msg4.setStringProperty("Drink", "Beer");
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message producer");
			}
			Topic imaDestination = producerSession.createTopic(imadestinationName);
			MessageProducer producer = producerSession.createProducer(imaDestination);
			
			Topic imaDestination2 = consumerSession.createTopic(imadestinationName);
			MessageConsumer consumer = consumerSession.createConsumer(imaDestination2, "(_Food$ LIKE 'Ch_ps')");
			consumer.setMessageListener(new AsynchronousJmsListener(this));
			consumerJMSSession.startConnection();
			
			if(Trace.traceOn())
			{
				Trace.trace("Sending four messages to IMA");
			}
			producer.send(msg1);
			if(Trace.traceOn())
			{
				Trace.trace("First has been committed");
			}
			producerSession.commit();
			producer.send(msg2);
			if(Trace.traceOn())
			{
				Trace.trace("Second has been rolled back");
			}
			producerSession.rollback();
			producer.send(msg3);
			if(Trace.traceOn())
			{
				Trace.trace("Third has been committed");
			}
			producerSession.commit();
			producer.send(msg4);
			if(Trace.traceOn())
			{
				Trace.trace("Fourth has been committed");
			};
			producerSession.commit();

			msg2.setStringProperty("_Food$", "Chips");
			msg2.setStringProperty("Drink", "Beer");
			msg3.setStringProperty("_Food$", "Pie");
			msg3.setStringProperty("Drink", "Beer");
			msg4.setStringProperty("_Food$", "Chips");
			msg4.setStringProperty("Drink", "Beer");
			
			// do nothing and wait until message received
			StaticUtilities.sleep(5000);
			
			if(completed == true)
			{
				if(Trace.traceOn())
				{
					Trace.trace("Test passed");
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
					Trace.trace("No message was returned in this test");
				}
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				closeConnections();
				System.exit(RC);
			}
			
		}
		catch(JMSException jmse)
		{
			if(Trace.traceOn())
			{
				Trace.trace("An exception occurred whilse attempting the test. Exception=" + jmse.getMessage());
			}
			System.out.println("\nAn exception occurred whilst sending a message. Exception=" + jmse.getMessage());
			RC = ReturnCodes.JMS_EXCEPTION;
			closeConnections();
			System.exit(RC);
		}
	}


	public void callback(Message arg0) 
	{
		try
		{
		
			if(arg0.getStringProperty("_Food$").equals("Chips"))
			{
				if(Trace.traceOn())
				{
					Trace.trace("Message has passed the check for user property '_Food$'");
				}
				
				
				if(CompareMessageContents.checkJMSMessageEquality(msg4, arg0, JmsMessage.imaTopic+imadestinationName, null))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message has passed equality checking for body");
					}
					completed = true;
				}
				else
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message equality was incorrect");
					}
					RC = ReturnCodes.INCORRECT_MESSAGE;
					closeConnections();
					System.exit(RC);
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


