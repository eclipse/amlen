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

/*** Test sends a JMS Bytes message to IMA topic
* and is received from that same topic
* 
* It checks that the bytes sent in the message body
* is correctly preserved.
* 
* Subscriber = non durable and synchronous receive
* Session = non transacted and auto acknowledgement
* Message = Text and non persistent
* Headers = non specifically set
*
*/
public class JmsBytes_ImaToIma extends JmsToJms{

	public static void main(String[] args) {
		
		new Trace().traceAutomation();
		JmsBytes_ImaToIma testClass = new JmsBytes_ImaToIma();
		testClass.setNumberOfMessages(1);
		testClass.setNumberOfPublishers(1);
		testClass.setNumberOfSubscribers(1);
		testClass.setTestName("JmsBytes_ImaToIma");
		
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
			
			testClass.sendJMSBytesMessage();
									
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}	
	}
	
	
	public void sendJMSBytesMessage()
	{
		try
		{
			if(Trace.traceOn())
			{
				Trace.trace("Creating the default producer session for IMA and consumer session for IMA");
			}
			Session producerSession = producerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null);
			Session consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null);
			
			Message msg = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.BYTES);
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message producer");
			}
			Topic imaDestination = producerSession.createTopic(imadestinationName);
			MessageProducer producer = producerSession.createProducer(imaDestination);
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message consumer and starting the connection");
			}
			Topic imaDestination2 = consumerSession.createTopic(imadestinationName);
			MessageConsumer consumer = consumerSession.createConsumer(imaDestination2);
			consumerJMSSession.startConnection();
			
			if(Trace.traceOn())
			{
				Trace.trace("Sending the message to IMA");
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
					// TODO change the RC back to 0 once we are happy with automation
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
