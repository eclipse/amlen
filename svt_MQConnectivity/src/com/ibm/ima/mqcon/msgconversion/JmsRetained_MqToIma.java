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

public class JmsRetained_MqToIma extends JmsMqToImaBaseClass
{
	public static void main(String[] args) 
	{
		retained = true;
		JmsRetained_MqToIma testClass = new JmsRetained_MqToIma();
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
				Trace.trace("Sending the retained message to MQ");
			}
			
			jmsProducer.send(msg);
			Thread.sleep(4000); // give the retained message chance to be sent over
			if(Trace.traceOn())
			{
				Trace.trace("Starting the connection");
			}
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message consumer");
			}
			consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null);
			Topic imaDestination = consumerSession.createTopic(imaDestinationName);
			jmsConsumer = consumerSession.createConsumer(imaDestination);
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
				Trace.trace("Sending the retained message to MQ");
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
			Thread.sleep(5000); // give the retained message chance to be sent over
			// Now create another consumer as we need to see the message as retained so need a new connection etc 
			consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null);
			jmsConsumer = consumerSession.createConsumer(imaDestination);
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
