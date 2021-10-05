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

import javax.jms.BytesMessage;
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import com.ibm.ima.mqcon.utils.CompareMessageContents;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.MqttConnection;
import com.ibm.ima.mqcon.utils.Trace;
import com.ibm.ima.mqcon.utils.JmsSession.CLIENT_TYPE;
import com.ibm.ima.test.cli.mq.MQConfig;

public abstract class TestHelper{
	
	protected static String imahostname = null;
	protected static int imaport = 0;
	protected static String imausername = "admin";
	protected static String imapassword = "admin";
	protected static String mqhostname = null;
	protected static int mqport = 0;
	protected static String mquser = null;
	protected static String mqpassword = null;
	protected static String mqlocation = null;
	protected static String imaDestinationName = null;
	protected static String mqDestinationName = null;
	protected static int timeout = 1;
	protected static int RC = ReturnCodes.INCOMPLETE_TEST;
	protected static String mqqueuemanager = null;
	protected JmsSession producerJMSSession = new JmsSession();
	protected JmsSession consumerJMSSession = new JmsSession();
	protected Session producerSession = null;
	protected Session consumerSession = null;
	protected MqttConnection mqttProducerConnection = null;
	protected MqttConnection mqttConsumerConnection = null;
	protected boolean completed = false;
	protected MessageProducer jmsProducer = null;
	protected MessageConsumer jmsConsumer = null;
	protected MqttClient consumer = null;
	protected MqttClient producer = null;
	protected Message msg = null;
	protected static boolean retained = false;
	protected int retainedFlag = 0;
	protected String body = null;
	protected static boolean transacted = false;

	
	public abstract void closeConnections();
	
	public abstract void runTest();
	
	public void callback(MqttMessage mqttmsg) {
		
		try
		{
			if(msg!= null)
			{
				if(CompareMessageContents.checkJMSToMQTTEquality(msg, mqttmsg))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message has passed equality checking for body");
					}
					if(retained)
					{
						if(retainedFlag == 1)
						{	
							if(mqttmsg.isRetained())
							{
								if(Trace.traceOn())
								{
									Trace.trace("Message has passed the check for retained=true");
								}
								completed = true;
							}
							else
							{
								if(Trace.traceOn())
								{
									Trace.trace("Message retained property was incorrect. Expected message to be retained");
								}
								RC = ReturnCodes.INCORRECT_RETAINED_PROP;
								
								throw new RuntimeException("Message retained property was incorrect. Expected message to be retained");
							
							}
						}
						else
						{
							if(! mqttmsg.isRetained())
							{
								if(Trace.traceOn())
								{
									Trace.trace("Message has passed the check for retained=false");
								}
								completed = true;
							}
							else
							{
								if(Trace.traceOn())
								{
									Trace.trace("Message retained property was incorrect. Expected message to be not marked as retained");
								}
								RC = ReturnCodes.INCORRECT_RETAINED_PROP;
								throw new RuntimeException("Message retained property was incorrect. Expected message to be not marked as retained");
							}
						}
					}
					else
					{
						completed = true;
					}
				}
				else	
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message equality was incorrect");
					}
					RC = ReturnCodes.INCORRECT_MESSAGE;
					throw new RuntimeException("Message equality was incorrect");
				}
			}
			else
			{
				String actualBody = new String(mqttmsg.getPayload());
				if(actualBody.equals(body))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message has passed equality checking for body");
					}
					if(retained)
					{
						if(retainedFlag == 1)
						{	
							if(mqttmsg.isRetained())
							{
								if(Trace.traceOn())
								{
									Trace.trace("Message has passed the check for retained=true");
								}
								completed = true;
							}
							else
							{
								if(Trace.traceOn())
								{
									Trace.trace("Message retained property was incorrect. Expected message to be retained");
								}
								RC = ReturnCodes.INCORRECT_RETAINED_PROP;
								throw new RuntimeException("Message retained property was incorrect. Expected message to be retained");
							}
						}
						else
						{
							if(! mqttmsg.isRetained())
							{
								if(Trace.traceOn())
								{
									Trace.trace("Message has passed the check for retained=false");
								}
								completed = true;
							}
							else
							{
								if(Trace.traceOn())
								{
									Trace.trace("Message retained property was incorrect. Expected message to be not marked as retained");
								}
								RC = ReturnCodes.INCORRECT_RETAINED_PROP;
								throw new RuntimeException("Message retained property was incorrect. Expected message to be not marked as retained");
							}
						}
					}
					else
					{
						completed = true;
					}
				}
				else	
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message equality was incorrect. Expected " + body + " but got " + actualBody);
					}
					RC = ReturnCodes.INCORRECT_MESSAGE;
					throw new RuntimeException("Message equality was incorrect");
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
				throw new RuntimeException("An exception occurred whilst attempting the test. Exception=" + jmse.getMessage());
			}
			
		}

	
	
	protected void checkContents(Message jmsMessage, int qos, String expectedBody)
	{
		try
		{
			if(jmsMessage == null)
			{

				if(Trace.traceOn())
				{
					Trace.trace("No message was received");
				}
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				throw new RuntimeException("Expected message but none arrived ");
			}
			String actualBody = null;
			if(jmsMessage instanceof TextMessage)
			{
				actualBody = ((TextMessage)jmsMessage).getText();
			}
			else if(jmsMessage instanceof BytesMessage)
			{
				int TEXT_LENGTH = new Long(((BytesMessage)jmsMessage).getBodyLength()).intValue();
				byte[] textBytes = new byte[TEXT_LENGTH];
				((BytesMessage)jmsMessage).readBytes(textBytes, TEXT_LENGTH);
				actualBody = new String(textBytes);	
			}
			
			if(expectedBody.equals(actualBody))
			{
				if(Trace.traceOn())
				{
					Trace.trace("Message equality was correct");
				}
				
				
				if(qos == 0)
				{
					if(jmsMessage.getJMSDeliveryMode() == DeliveryMode.NON_PERSISTENT)
					{
						if(Trace.traceOn())
						{
							Trace.trace("Expected delivery mode was correct");
						}
					}
					else
					{
						if(Trace.traceOn())
						{
							Trace.trace("Expected DeliveryMode.NON_PERSISTENT but got " + jmsMessage.getJMSDeliveryMode());
						}
						RC = ReturnCodes.INCORRECT_QOS;
						throw new RuntimeException("Expected DeliveryMode.NON_PERSISTENT but got " + jmsMessage.getJMSDeliveryMode());
					}
				}
				else if((qos == 1 || qos == 2))
				{
					if((jmsMessage.getJMSDeliveryMode() == DeliveryMode.PERSISTENT))
					{
						if(Trace.traceOn())
						{
							Trace.trace("Expected delivery mode was correct");
						}
					}
					else
					{
						if(Trace.traceOn())
						{
							Trace.trace("Expected DeliveryMode.PERSISTENT but got " + jmsMessage.getJMSDeliveryMode());
						}
						RC = ReturnCodes.INCORRECT_QOS;
						throw new RuntimeException("Expected DeliveryMode.PERSISTENT but got " + jmsMessage.getJMSDeliveryMode());
					
					}
				}
				
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("Message equality was incorrect ");
				}
				RC = ReturnCodes.INCORRECT_MESSAGE;
				throw new RuntimeException("Message equality was incorrect");
			}
			
		}
		catch(JMSException jmse)
		{
			if(Trace.traceOn())
			{
				Trace.trace("An exception occurred whilse attempting the test. Exception=" + jmse.getMessage());
			}
			System.out.println("An exception occurred whilst sending a message. Exception=" + jmse.getMessage());
			RC = ReturnCodes.JMS_EXCEPTION;
			throw new RuntimeException("An exception occurred whilse attempting the test. Exception=" + jmse.getMessage());
		}
		
	}
	
	protected void checkContents(Message jmsMessage, String expectedBody, int expectedRetained)
	{
		try
		{
			if(jmsMessage == null)
			{

				if(Trace.traceOn())
				{
					Trace.trace("No message was received");
				}
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				throw new RuntimeException("No message was received");
			}
			String actualBody = null;
			if(jmsMessage instanceof TextMessage)
			{
				actualBody = ((TextMessage)jmsMessage).getText();
			}
			else if(jmsMessage instanceof BytesMessage)
			{
				int TEXT_LENGTH = new Long(((BytesMessage)jmsMessage).getBodyLength()).intValue();
				byte[] textBytes = new byte[TEXT_LENGTH];
				((BytesMessage)jmsMessage).readBytes(textBytes, TEXT_LENGTH);
				actualBody = new String(textBytes);	
			}
			
			
			if(expectedBody.equals(actualBody))
			{
				if(Trace.traceOn())
				{
					Trace.trace("Message equality was correct");
				}
				
				if(retained)
				{
					try
					{
						if(jmsMessage.getIntProperty("JMS_IBM_Retain") == expectedRetained)
						{
							if(Trace.traceOn())
							{
								Trace.trace("Message retained property was correctly set. Expected " + expectedRetained + " and got " + jmsMessage.getIntProperty("JMS_IBM_Retain"));
							}
						}
						else
						{
							if(Trace.traceOn())
							{
								Trace.trace("Message retained property was incorrect. Expected " + expectedRetained + " but got " + jmsMessage.getIntProperty("JMS_IBM_Retain"));
							}
							RC = ReturnCodes.INCORRECT_RETAINED_PROP;
							throw new RuntimeException("Message retained property was incorrect. Expected " + expectedRetained + " but got " + jmsMessage.getIntProperty("JMS_IBM_Retain"));
						}
					}
					catch(NumberFormatException nfe)
					{
						// in mq we get a null if not retained
						if(expectedRetained == 0 && consumerJMSSession.getClientType().equals(CLIENT_TYPE.MQ))
						{
							if(Trace.traceOn())
							{
								Trace.trace("Message retained property was correctly set. Expected " + expectedRetained + " and got 0");
							}
						}
						else
						{
							if(Trace.traceOn())
							{
								Trace.trace("Message retained property was incorrect. Expected " + expectedRetained + " but got a null value");
							}
							RC = ReturnCodes.INCORRECT_RETAINED_PROP;
							throw new RuntimeException("Message retained property was incorrect. Expected " + expectedRetained + " but got a null value");
						}
					}
				}
				
				
			
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("Message equality was incorrect");
				}
				RC = ReturnCodes.INCORRECT_MESSAGE;
				throw new RuntimeException("Message equality was incorrect");
			}
			
		}
		catch(JMSException jmse)
		{
			if(Trace.traceOn())
			{
				Trace.trace("An exception occurred whilse attempting the test. Exception=" + jmse.getMessage());
			}
			System.out.println("An exception occurred whilst sending a message. Exception=" + jmse.getMessage());
			RC = ReturnCodes.JMS_EXCEPTION;
			throw new RuntimeException("An exception occurred whilse attempting the test. Exception=" + jmse.getMessage());
		}
		
	}
	
	
	protected void clearRetainedIMAMessage()
	{
		if(Trace.traceOn())
		{
			Trace.trace("Sending a null mqtt message to IMA to clear up any retained messages. - Adding more debug");
		}
		try
		{
			String clientID = UUID.randomUUID().toString().substring(0, 10).replace('-', '_');
			mqttProducerConnection = new MqttConnection(imahostname, imaport, clientID, true, 0);
			mqttProducerConnection.setPublisher();
			if(Trace.traceOn())
			{
				Trace.trace("Producer connection to send a message to clear the retained message created.");
			}	
			MqttClient client = mqttProducerConnection.getClient();
			MqttMessage msg1 = new MqttMessage();
			msg1.setRetained(true);
			if(Trace.traceOn())
			{
				Trace.trace("Message is set as retained");
			}	
			MqttTopic topic =  client.getTopic(imaDestinationName);
			MqttDeliveryToken token = topic.publish(msg1);
			if(Trace.traceOn())
			{
				Trace.trace("Waiting for message token to arrive.");
			}	
			token.waitForCompletion(5000);
			if(Trace.traceOn())
			{
				Trace.trace("Published message to clear successfully");
			}			
		}
		catch(Exception e)
		{
			if(Trace.traceOn())
			{
				Trace.trace("Error clearing retained message");
			}	
			e.printStackTrace();
		}
	}
	

	
	protected void clearRetainedMQMessage()
	{
		if(Trace.traceOn())
		{
			Trace.trace("Calling MQ to clear up any retained messages");
		}
		MQConfig config = null;
		try
		{
			if(mqhostname!= null && mquser!=null && mqlocation!=null)
			{
				config = new MQConfig(mqhostname, mquser, mqpassword, mqlocation);
				config.connectToServer();
				if(config.hasRetainedMessages(mqDestinationName, mqqueuemanager))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Found a retained messages so going to clear it");
					}
					config.clearRetained(mqDestinationName, mqqueuemanager);
				}
				else
				{
					if(Trace.traceOn())
					{
						Trace.trace("No retained messages were found");
					}
				}
				config.disconnectServer();
				
			}
		}
		catch(Exception e)
		{
			if(Trace.traceOn())
			{
				Trace.trace("An exception occurred whilst trying to clear the MQ retained messages. This may mean the next test fails");
			}
			e.printStackTrace();
			try
			{
				config.disconnectServer();
			}
			catch(Exception ef)
			{
				// do nothing
			}
		}
	}

}
