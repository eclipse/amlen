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

import javax.jms.JMSException;

import org.eclipse.paho.client.mqttv3.MqttException;

import com.ibm.ima.mqcon.utils.AsynchronousMqttListener;
import com.ibm.ima.mqcon.utils.JmsMessage;
import com.ibm.ima.mqcon.utils.MqttConnection;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.ima.mqcon.utils.Trace;

public class JmsRetainedToMqtt_MqToIma  extends JmsToMqttMqToImaBaseClass
{

	public static void main(String[] args) 
	{
		retained = true;
		JmsRetainedToMqtt_MqToIma testClass = new JmsRetainedToMqtt_MqToIma();
		testClass.setup(args);
	}

	@Override
	public void runTest()
	{
		try
		{
			retainedFlag = 1;
			msg = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			msg.setIntProperty("JMS_IBM_Retain", 1);
			
			if(Trace.traceOn())
			{
				Trace.trace("Sending first message");
			}
			jmsProducer.send(msg);
			Thread.sleep(4000); // give the retained message chance to be sent over
			if(Trace.traceOn())
			{
				Trace.trace("Creating the consumer");
			}
			String clientID = UUID.randomUUID().toString().substring(0, 10).replace('-', '_');
			mqttConsumerConnection = new MqttConnection(imahostname, imaport, clientID, true, 0);
			AsynchronousMqttListener listener = new AsynchronousMqttListener(this);
			consumer = mqttConsumerConnection.createClient(listener);
			consumer.subscribe(imaDestinationName);
			
			// do nothing and wait until message received
			StaticUtilities.sleep(8000);
			if(completed == true)
			{
				// carry on
				completed = false;
				retainedFlag = 0;
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("Something was wrong with the expected message");
				}
				RC = ReturnCodes.INCORRECT_MESSAGE;
				this.closeConnections();
				this.clearRetainedIMAMessage();
				System.exit(RC);
			}
			if(Trace.traceOn())
			{
				Trace.trace("Sending a second message whilst consumer still connected");
			}
			msg = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			msg.setIntProperty("JMS_IBM_Retain", 1);
			jmsProducer.send(msg);
			// do nothing and wait until message received
			StaticUtilities.sleep(8000);
			if(completed == true)
			{
				// carry on
				completed = false;
				retainedFlag = 1;
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("Something was wrong with the expected message");
				}
				RC = ReturnCodes.INCORRECT_MESSAGE;
				this.closeConnections();
				this.clearRetainedIMAMessage();
				System.exit(RC);
			}
			if(Trace.traceOn())
			{
				Trace.trace("Now sending a non retained message which should be converted to retained by mqconnectivity for a new consumer");
			}
			// As mqconnectivity changes a non retained to retained check this works
			consumer.unsubscribe(imaDestinationName);
			msg = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.TEXT);
			jmsProducer.send(msg);
			StaticUtilities.sleep(4000);
			consumer.subscribe(imaDestinationName);
			StaticUtilities.sleep(4000);
			if(completed == true)
			{
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
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("No message was returned in this test");
				}
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				consumer.unsubscribe(imaDestinationName);
				closeConnections();
				clearRetainedMessages();
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
			clearRetainedMessages();
			System.exit(RC);
		}
		catch(MqttException mqtte)
		{
			if(Trace.traceOn())
			{
				Trace.trace("An exception occurred whilst attempting the test. Exception=" + mqtte.getMessage());
			}
			mqtte.printStackTrace();
			RC = ReturnCodes.MQTT_EXCEPTION;
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

