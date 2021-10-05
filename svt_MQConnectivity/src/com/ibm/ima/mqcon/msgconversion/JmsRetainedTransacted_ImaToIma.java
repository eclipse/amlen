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
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.mqcon.utils.JmsMessage;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.Trace;

public class JmsRetainedTransacted_ImaToIma extends JmsImaBaseClass
{
	public static void main(String[] args) 
	{
		retained = true;
		transacted = true;
		JmsRetainedTransacted_ImaToIma testClass = new JmsRetainedTransacted_ImaToIma();
		testClass.setup(args);
	}

	@Override
	public void runTest()
	{
		try
		{
			Session secondSession = new JmsSession().getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, true, Session.AUTO_ACKNOWLEDGE);		
			Topic imaDestination = secondSession.createTopic(imaDestinationName);
			MessageProducer jmsProducer2 = secondSession.createProducer(imaDestination);
			
			msg = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);	
			msg.setIntProperty("JMS_IBM_Retain", 1);
			if(Trace.traceOn())
			{
				Trace.trace("Sending the first retained message to IMA");
			}
			
			jmsProducer.send(msg);
			
			if(Trace.traceOn())
			{
				Trace.trace("Sending the seconds retained message to IMA by another producer");
			}
			
			msg = JmsMessage.createMessage(secondSession, JmsMessage.MESSAGE_TYPE.TEXT);
			msg.setIntProperty("JMS_IBM_Retain", 1);
			jmsProducer2.send(msg);
		
			if(Trace.traceOn())
			{
				Trace.trace("Committing the second message");
			}
			secondSession.commit();
			
			if(Trace.traceOn())
			{
				Trace.trace("Committing the first message");
			}
			producerSession.commit();
		
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message consumer");
			}
			consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null);
			imaDestination = consumerSession.createTopic(imaDestinationName);
			jmsConsumer = consumerSession.createConsumer(imaDestination);
			consumerJMSSession.startConnection();
			
			Message jmsMessage = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage, ((TextMessage)msg).getText(), 1);
			
			// All passed 
			if(Trace.traceOn())
			{
				Trace.trace("Test passed");
			}
			RC = ReturnCodes.PASSED;
			System.out.println("Test.*Success!");
			this.closeConnections();
			this.clearRetainedIMAMessage();
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
			this.closeConnections();
			this.clearRetainedIMAMessage();
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
			this.closeConnections();
			this.clearRetainedIMAMessage();
			System.exit(RC);
		}
		
	}

	
}

