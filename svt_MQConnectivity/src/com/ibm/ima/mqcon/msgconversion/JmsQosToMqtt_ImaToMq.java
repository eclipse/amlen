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

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.Session;
import javax.jms.Topic;

import org.eclipse.paho.client.mqttv3.MqttMessage;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.mqcon.utils.CompareMessageContents;
import com.ibm.ima.mqcon.utils.JmsMessage;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.ima.mqcon.utils.Trace;

/**
 * Test sends a JMS text message to an IMA topic
 * which is received by an MQTT client listening
 * on an MQ topic.
 * 
 * It checks that the QOS is mapped correctly
 * 
 * Subscriber = MQTT
 * Session = non transacted and auto acknowledgement
 * Message = text and non persistent and persistent
 * Headers = Set
 *
 */
public class JmsQosToMqtt_ImaToMq extends JmsToMqttImaToMqBaseClass{
	
	private Message msg1 = null;
	private Message msg2 = null;
	private Message msg3 = null;
	private int messageNum = 0;
	
		
	public static void main(String[] args) 
	{
		JmsQosToMqtt_ImaToMq testClass = new JmsQosToMqtt_ImaToMq();
		testClass.setup(args);
	}
		
		
	@Override
	public void runTest()
	{
		try
		{
			if(Trace.traceOn())
			{
				Trace.trace("Creating the default producer session (ACK disabled) for IMA and consumer client for MQ");
			}
			
			// for this test I need to specify some properties on connection factory to get non persistent messages mapped
			// to QOS 0
			ConnectionFactory connectionFactory = ImaJmsFactory.createConnectionFactory();
			((ImaProperties) connectionFactory).put("Server", imahostname);
			((ImaProperties) connectionFactory).put("Port", imaport);
			((ImaProperties) connectionFactory).put("DisableACK", true);
			Connection connection = connectionFactory.createConnection();
			producerSession = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
			
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message producer");
			}
			Topic imaDestination = producerSession.createTopic(imaDestinationName);
			jmsProducer = producerSession.createProducer(imaDestination);
			
			
			msg1 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			jmsProducer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
			if(Trace.traceOn())
			{
				Trace.trace("Sending the message to IMA");
			}
			jmsProducer.send(msg1);
			
			msg2 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			jmsProducer.setDeliveryMode(DeliveryMode.PERSISTENT);
			if(Trace.traceOn())
			{
				Trace.trace("Sending the message to IMA");
			}
			jmsProducer.send(msg2);
			
			// Now send one without the disableAck
			producerSession = producerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null);
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message producer without the ACK disabled");
			}
			imaDestination = producerSession.createTopic(imaDestinationName);
			jmsProducer = producerSession.createProducer(imaDestination);
			msg3 = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			jmsProducer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
			jmsProducer.send(msg3);
			// do nothing and wait until message received
			StaticUtilities.sleep(20000);
			
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
				Trace.trace("An exception occurred whilst attempting the test. Exception=" + jmse.getMessage());
			}
			jmse.printStackTrace();
			RC = ReturnCodes.JMS_EXCEPTION;
			this.closeConnections();
			System.exit(RC);
		}
		
	}

	@Override
	public void callback(MqttMessage msg) {
		
		try
		{
			messageNum ++;
			
			if(messageNum == 1)
			{
				if(CompareMessageContents.checkJMSToMQTTEquality(msg1, msg))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message 1 has passed equality checking for body");
					}
					
					if(msg.getQos() == 0)
					{
						if(Trace.traceOn())
						{
							Trace.trace("Message 1 has passed the check for QOS mapping");
						}
					}
					else
					{
						if(Trace.traceOn())
						{
							Trace.trace("The expected QOS was 0 but the actual QOS was " + msg.getQos());
						}
						RC = ReturnCodes.INCORRECT_QOS;
						closeConnections();
						System.exit(RC);
					}
					
					
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
			else if(messageNum == 2)
			{
				if(CompareMessageContents.checkJMSToMQTTEquality(msg2, msg))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message 2 has passed equality checking for body");
					}
					
					if(msg.getQos() == 1)
					{
						if(Trace.traceOn())
						{
							Trace.trace("Message 2 has passed the check for QOS mapping");
						}
						
					}
					else
					{
						if(Trace.traceOn())
						{
							Trace.trace("The expected QOS was 1 but the actual QOS was " + msg.getQos());
						}
						RC = ReturnCodes.INCORRECT_QOS;
						closeConnections();
						System.exit(RC);
					}
					
					
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
			else if(messageNum == 3)
			{
				if(CompareMessageContents.checkJMSToMQTTEquality(msg3, msg))
				{
					if(Trace.traceOn())
					{
						Trace.trace("Message 3 has passed equality checking for body");
					}
					
					if(msg.getQos() == 1)
					{
						if(Trace.traceOn())
						{
							Trace.trace("Message 3 has passed the check for QOS mapping");
						}
						completed = true;
					}
					else
					{
						if(Trace.traceOn())
						{
							Trace.trace("The expected QOS was 1 but the actual QOS was " + msg.getQos());
						}
						RC = ReturnCodes.INCORRECT_QOS;
						closeConnections();
						System.exit(RC);
					}
					
					
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
		catch(Exception e)
		{
			if(Trace.traceOn())
			{
				Trace.trace("An exception occurred whilst attempting the test. Exception=" + e.getMessage());
			}
			e.printStackTrace();
			RC = ReturnCodes.GENERAL_EXCEPTION;
			this.closeConnections();
			System.exit(RC);
		}
	}

}
