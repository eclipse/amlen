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

import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.ima.mqcon.utils.Trace;

/**
 * Test sends MQTT messages to an IMA topic
 * which is received by a MQTT client listening
 * on an IMA topic.
 * 
 * It checks that the QOS is mapped and or overriden correctly etc
 * 
 * Subscriber = MQTT
 * Message = MQTT
 *
 */
public class Mqtt_ImaToIma extends MqttImaBaseClass {

	private String msg1Body = null;
	private String msg2Body = null;
	private String msg3Body = null;
	private int messageNum = 0;
	
	public static void main(String[] args) 
	{
		Mqtt_ImaToIma testClass = new Mqtt_ImaToIma();
		testClass.setup(args);
	}
	
	@Override
	public void runTest()
	{
		try
		{
			
				
			msg1Body = UUID.randomUUID().toString().substring(0, 30);
			MqttMessage msg1 = new MqttMessage(msg1Body.getBytes());
			msg1.setQos(2);	
			
			MqttDeliveryToken token = topic.publish(msg1);
			token.waitForCompletion(5000);				
		
			msg2Body = UUID.randomUUID().toString().substring(0, 30);
			if(Trace.traceOn())
			{
				Trace.trace("Sending second message: " + msg2Body + " with QOS=1");
			}
			MqttMessage msg2 = new MqttMessage(msg2Body.getBytes());
			msg2.setQos(1);	
			token = topic.publish(msg2);
			token.waitForCompletion(5000);	
			
			msg3Body = UUID.randomUUID().toString().substring(0, 30);
			if(Trace.traceOn())
			{
				Trace.trace("Sending third message: " + msg3Body + " with QOS=0");
			}
			MqttMessage msg3 = new MqttMessage(msg3Body.getBytes());
			msg3.setQos(0);	
			token = topic.publish(msg3);
			token.waitForCompletion(5000);	
			
				
			
			// do nothing and wait until message received
			StaticUtilities.sleep(8000);
			
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
					Trace.trace("Not all of the messages were returned in this test");
				}
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				closeConnections();
				System.exit(RC);
			}
			
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
			System.exit(RC);
		}
	}
	
	private synchronized void checkContents(MqttMessage actual)
	{
		String actualBody = new String(actual.getPayload());
		if(actualBody.equals(msg1Body))
		{
			if(actual.getQos() == 2)
			{
				if(Trace.traceOn())
				{
					Trace.trace("Message 1 passed the QOS equality check. QOS=2");
				}
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("Expected QOS was 2 but actual QOS was " + actual.getQos());
				}
				RC = ReturnCodes.INCORRECT_QOS;
				closeConnections();
				System.exit(RC);
			}
		}
		else if(actualBody.equals(msg2Body))
		{
			if(actual.getQos() == 1)
			{
				if(Trace.traceOn())
				{
					Trace.trace("Message 2 passed the QOS equality check. QOS=1");
				}
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("Expected QOS was 1 but actual QOS was " + actual.getQos());
				}
				RC = ReturnCodes.INCORRECT_QOS;
				closeConnections();
				System.exit(RC);
			}
		}
		else if(actualBody.equals(msg3Body))
		{
			if(actual.getQos() == 0)
			{
				if(Trace.traceOn())
				{
					Trace.trace("Message 3 passed the QOS equality check. QOS=0");
				}
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("Expected QOS was 0 but actual QOS was " + actual.getQos());
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
				Trace.trace("Message was not expected");
			}
			RC = ReturnCodes.INCORRECT_MESSAGE;
			closeConnections();
			System.exit(RC);
		}
			
						
		
	}
	

	@Override
	public void callback(MqttMessage msg) {
		
		
		messageNum++;
		
		this.checkContents(msg);
		
		if(messageNum == 3)
		{
			// all completed
			completed = true;
		}
		
	}

}
