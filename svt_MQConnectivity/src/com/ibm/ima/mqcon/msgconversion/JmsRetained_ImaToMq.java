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
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.mqcon.utils.JmsMessage;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.Trace;

public class JmsRetained_ImaToMq extends JmsImaToMqBaseClass

 
{
	public static void main(String[] args) 
	{
		retained = true;
		JmsRetained_ImaToMq testClass = new JmsRetained_ImaToMq();
		testClass.setup(args);
	}

	@Override
	public void runTest()
	{
		try
		{
					
			msg = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			msg.setIntProperty("JMS_IBM_Retain", 1);			
			if(Trace.traceOn())
			{
				Trace.trace("Sending the retained message to IMA");
			}
			
			jmsProducer.send(msg);
			Thread.sleep(8000); // give the retained message chance to be sent over
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message consumer");
			}

			consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.MQ, mqhostname, mqport, mqqueuemanager);
			Topic mqDestination = consumerSession.createTopic(mqDestinationName);
			jmsConsumer = consumerSession.createConsumer(mqDestination);
			consumerJMSSession.startConnection();
			Message jmsMessage = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage, ((TextMessage)msg).getText(), 1);
			if(Trace.traceOn())
			{
				Trace.trace("Sending a second message whilst consumer still connected");
			}
			msg = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			msg.setIntProperty("JMS_IBM_Retain", 1);
			if(Trace.traceOn())
			{
				Trace.trace("Sending the retained message to IMA");
			}			
			jmsProducer.send(msg);
		
			jmsMessage = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage, ((TextMessage)msg).getText(), 0);
			
			if(Trace.traceOn())
			{
				Trace.trace("Now sending a non retained message which should be converted to retained by mqconnectivity for a new consumer");
			}
			// mapping will always send message as retained even if not so send in one that isnt
			msg = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			jmsProducer.send(msg);
			Thread.sleep(8000); // give the retained message chance to be sent over
			// Now create another consumer as we need to see the message as retained so need a new connection etc 
			consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.MQ, mqhostname, mqport, mqqueuemanager);
			jmsConsumer = consumerSession.createConsumer(mqDestination);
			consumerJMSSession.startConnection();
			jmsMessage = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage, ((TextMessage)msg).getText(), 1);
			
			
			// All passed 
			if(Trace.traceOn())
			{
				Trace.trace("Test passed");
			}
			RC = ReturnCodes.PASSED;
			System.out.println("Test.*Success!");
			closeConnections();
			clearRetainedMessages();
			System.exit(RC);
			
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
			clearRetainedMessages();
			System.exit(RC);
		}
		catch(Exception e)
		{
			if(RC == ReturnCodes.INCOMPLETE_TEST)
			{
				if(Trace.traceOn())
				{
					Trace.trace("An exception occurred whilst attempting the test. Exception=" + e.getMessage());
				}
				RC = ReturnCodes.GENERAL_EXCEPTION;
			}
			e.printStackTrace();			
			closeConnections();
			clearRetainedMessages();
			System.exit(RC);
		}
		
	}

	
	
}
