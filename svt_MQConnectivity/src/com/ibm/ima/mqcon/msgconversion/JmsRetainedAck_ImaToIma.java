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
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.mqcon.utils.JmsMessage;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.Trace;
import com.ibm.ima.test.cli.imaserver.ImaServerStatus;

public class JmsRetainedAck_ImaToIma extends JmsImaBaseClass
{
	public static void main(String[] args) 
	{
		retained = true;
		JmsRetainedAck_ImaToIma testClass = new JmsRetainedAck_ImaToIma();
		testClass.setup(args);
	}

	@Override
	public void runTest()
	{
		ImaServerStatus server = null;
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
			consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, clientID, false, Session.CLIENT_ACKNOWLEDGE);
			Topic imaDestination = consumerSession.createTopic(imaDestinationName);
			jmsConsumer = consumerSession.createDurableSubscriber(imaDestination, subID);
			consumerJMSSession.startConnection();
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message consumer with client ack");
			}
			Message jmsMessage = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage, ((TextMessage)msg1).getText(), 1);
			
			
			Message msg2 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			msg2.setIntProperty("JMS_IBM_Retain", 1);
			
			if(Trace.traceOn())
			{
				Trace.trace("Sending the second retained persistent message to IMA");
			}
			
			jmsProducer.send(msg2);
			
			jmsMessage = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage, ((TextMessage)msg2).getText(), 0);
			
			Message msg3 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			msg3.setIntProperty("JMS_IBM_Retain", 1);

			if(Trace.traceOn())
			{
				Trace.trace("Sending the third retained non persistent message to IMA");
			}
			jmsProducer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
			jmsProducer.send(msg3);
			
			jmsMessage = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage, ((TextMessage)msg3).getText(), 0);
			
			
			if(Trace.traceOn())
			{
				Trace.trace("Now we will stop the server");
			}
			
			server = new ImaServerStatus(imahostname, imausername, imapassword);
			if(Trace.traceOn())
			{
				Trace.trace("Created connection to appliance at "+imahostname+ " user "+imausername+" password "+imapassword);
			}
			server.connectToServer();
			server.stopForceServer();
			
			if(Trace.traceOn())
			{
				Trace.trace("Now we will start the server");
			}
			server.startServer(120000);
			if(Trace.traceOn())
			{
				Trace.trace("Server status is " + server.showStatus());
			}
			
			if(Trace.traceOn())
			{
				Trace.trace("Now we will reconnect our client");
			}
			consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, clientID, false, Session.CLIENT_ACKNOWLEDGE);
			jmsConsumer = consumerSession.createDurableSubscriber(imaDestination, subID);
			consumerJMSSession.startConnection();		
			
			jmsMessage = jmsConsumer.receive(timeout/3);
			this.checkContents(jmsMessage, ((TextMessage)msg1).getText(), 1);
			
			Message ackMessage = jmsConsumer.receive(timeout/3);
			this.checkContents(ackMessage, ((TextMessage)msg2).getText(), 0);
			
			jmsMessage = jmsConsumer.receive(timeout/3);
			if(jmsMessage == null)
			{
				// we are done
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("A message was return which was unexpected");
				}
				RC = ReturnCodes.INCORRECT_MESSAGE;
				this.closeConnections();
				this.clearRetainedIMAMessage();
				System.exit(RC);
			}
			
			if(Trace.traceOn())
			{
				Trace.trace("The messages will be acked");
				
			}
			
			ackMessage.acknowledge();
			
			if(Trace.traceOn())
			{
				Trace.trace("Now we will stop the server");
			}
			server.connectToServer();
			server.stopForceServer();
			if(Trace.traceOn())
			{
				Trace.trace("Now we will start the server");
			}
			server.startServer(120000);
			if(Trace.traceOn())
			{
				Trace.trace("Server status is " + server.showStatus());
			}
			
			
			if(Trace.traceOn())
			{
				Trace.trace("Now we will reconnect our client");
			}
			consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, clientID, false, Session.CLIENT_ACKNOWLEDGE);
			jmsConsumer = consumerSession.createDurableSubscriber(imaDestination, subID);
			consumerJMSSession.startConnection();
			
			jmsMessage = jmsConsumer.receive(timeout/3);
			if(jmsMessage == null)
			{
				// we are done
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("A message was return which was unexpected");
				}
				RC = ReturnCodes.INCORRECT_MESSAGE;
				try
				{
					server.disconnectServer();
				}
				catch(Exception e)
				{
					// do nothing
				}
				this.closeConnections();
				this.clearRetainedIMAMessage();
				System.exit(RC);
			}
			
			// All passed 
			if(Trace.traceOn())
			{
				Trace.trace("Test passed");
			}
			RC = ReturnCodes.PASSED;
			System.out.println("Test.*Success!");
			try
			{
				server.disconnectServer();
			}
			catch(Exception e)
			{
				// do nothing
			}
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
			try
			{
				server.disconnectServer();
			}
			catch(Exception e)
			{
				// do nothing
			}
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
			try
			{
				server.disconnectServer();
			}
			catch(Exception es)
			{
				// do nothing
			}
			this.closeConnections();
			this.clearRetainedIMAMessage();
			System.exit(RC);
		}
		
	}

	
}
