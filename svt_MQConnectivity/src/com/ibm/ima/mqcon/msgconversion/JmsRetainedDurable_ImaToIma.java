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

import java.util.UUID;

import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.mqcon.utils.JmsMessage;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.Trace;

public class JmsRetainedDurable_ImaToIma extends JmsImaBaseClass
{
	public static void main(String[] args) 
	{
		retained = true;
		JmsRetainedDurable_ImaToIma testClass = new JmsRetainedDurable_ImaToIma();
		testClass.setup(args);
	}

	@Override
	public void runTest()
	{
		try
		{		
			Message msg1 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			msg1.setIntProperty("JMS_IBM_Retain", 1);
			if(Trace.traceOn())
			{
				Trace.trace("Sending the first retained persistent message to IMA");
			}
			jmsProducer.setDeliveryMode(DeliveryMode.PERSISTENT);
			jmsProducer.send(msg1);
			
			String clientID = UUID.randomUUID().toString().substring(0, 10).replace('-', '_');
			String subID = 	UUID.randomUUID().toString().substring(0, 10).replace('-', '_');
			consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, clientID);
			Topic imaDestination = consumerSession.createTopic(imaDestinationName);
			jmsConsumer = consumerSession.createDurableSubscriber(imaDestination, subID);
			consumerJMSSession.startConnection();
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the durable message consumer");
			}
			if(Trace.traceOn())
			{
				Trace.trace("Durable subscriber is attempting to get the message expecting to be marked retained");
			}
			Message jmsMessage = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage, ((TextMessage)msg1).getText(), 1);
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the non durable message consumer");
			}
			String clientID2 = UUID.randomUUID().toString().substring(0, 10).replace('-', '_');
			JmsSession consumerJMSSession2 = new JmsSession(); 
			Session consumerSession2 = consumerJMSSession2.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, clientID2);
			MessageConsumer jmsConsumer2 = consumerSession2.createConsumer(imaDestination);
			consumerJMSSession2.startConnection();
			jmsMessage = jmsConsumer2.receive(timeout);
			this.checkContents(jmsMessage, ((TextMessage)msg1).getText(), 1);
			
			if(Trace.traceOn())
			{
				Trace.trace("Disconnecting both clients");
			}
			jmsConsumer.close();
			consumerJMSSession.closeConnection();
			jmsConsumer2.close();
			consumerJMSSession2.closeConnection();
			
			msg1 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			msg1.setIntProperty("JMS_IBM_Retain", 1);
			if(Trace.traceOn())
			{
				Trace.trace("Sending the second retained persistent message to IMA");
			}
			jmsProducer.send(msg1);
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating both clients again");
			}
			consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, clientID);
			jmsConsumer = consumerSession.createDurableSubscriber(imaDestination, subID);
			consumerJMSSession.startConnection();
			
			consumerSession2 = consumerJMSSession2.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, clientID2);
			jmsConsumer2 = consumerSession2.createConsumer(imaDestination);
			consumerJMSSession2.startConnection();
			

			jmsMessage = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage, ((TextMessage)msg1).getText(), 0);
			
			jmsMessage = jmsConsumer2.receive(timeout);
			this.checkContents(jmsMessage, ((TextMessage)msg1).getText(), 1);
			
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

