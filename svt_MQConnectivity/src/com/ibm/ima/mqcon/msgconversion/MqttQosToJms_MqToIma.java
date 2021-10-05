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
import javax.jms.Message;

import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import com.ibm.ima.mqcon.utils.Trace;

/**
 * Test sends a MQTT message to an MQ topic
 * which is received by a JMS client listening
 * on an IMA topic.
 * 
 * It checks that the QOS is mapped correctly
 * 
 * Subscriber = JMS
 * Session = non transacted and auto acknowledgement
 * Message = MQTT
 * Headers = 
 *
 */
public class MqttQosToJms_MqToIma extends MqttToJmsMqToImaBaseClass {
	
	public static void main(String[] args) 
	{
		MqttQosToJms_MqToIma testClass = new MqttQosToJms_MqToIma();
		testClass.setup(args);
	}

	@Override
	public void runTest()
	{
		try
		{
			
			String body = UUID.randomUUID().toString().substring(0, 30);

			if(Trace.traceOn())
			{
				Trace.trace("Sending first message: " + body);
			}
			MqttMessage msg1 = new MqttMessage(body.getBytes());
			msg1.setQos(0);	
			
			MqttDeliveryToken token = topic.publish(msg1);
			token.waitForCompletion(5000);			
			Message jmsMessage1 = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage1, 0, body);
			
			body = UUID.randomUUID().toString().substring(0, 30);
			if(Trace.traceOn())
			{
				Trace.trace("Sending second message: " + body);
			}
			MqttMessage msg2 = new MqttMessage(body.getBytes());
			msg2.setQos(1);
			token = topic.publish(msg2);
			token.waitForCompletion(5000);			
			Message jmsMessage2 = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage2, 1, body);
			
			body = UUID.randomUUID().toString().substring(0, 30);
			if(Trace.traceOn())
			{
				Trace.trace("Sending third message: " + body);
			}
			MqttMessage msg3 = new MqttMessage(body.getBytes());
			msg3.setQos(2);
			token = topic.publish(msg3);
			token.waitForCompletion(5000);			
			Message jmsMessage3 = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage3, 2, body);
			
			
			// All passed 
			if(Trace.traceOn())
			{
				Trace.trace("All three messages passed");
			}
			RC = ReturnCodes.PASSED;
			System.out.println("Test.*Success!");
			this.closeConnections();
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
	
	
	
	
}
