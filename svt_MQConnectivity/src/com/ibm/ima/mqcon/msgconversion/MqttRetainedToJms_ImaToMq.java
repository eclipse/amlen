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
import javax.jms.Topic;

import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.Trace;

public class MqttRetainedToJms_ImaToMq extends MqttToJmsImaToMqBaseClass{

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		
		retained = true;
		MqttRetainedToJms_ImaToMq testClass = new MqttRetainedToJms_ImaToMq();
		testClass.setup(args);
	}
	
	
	@Override
	public void runTest()
	{
		try
		{
			String body = UUID.randomUUID().toString().substring(0, 30);
			MqttMessage msg1 = new MqttMessage(body.getBytes());
			System.out.println("body " + body);
			msg1.setQos(0);	
			msg1.setRetained(true);
			if(Trace.traceOn())
			{
				Trace.trace("Sending the retained message to IMA");
			}
			MqttDeliveryToken token = topic.publish(msg1);
			token.waitForCompletion(5000);
			Thread.sleep(4000); // give the retained message chance to be sent over
			Topic mqDestination = consumerSession.createTopic(mqDestinationName);
			jmsConsumer = consumerSession.createConsumer(mqDestination);
			consumerJMSSession.startConnection();
			Message jmsMessage = jmsConsumer.receive(timeout);
			checkContents(jmsMessage, body, 1);
			if(Trace.traceOn())
			{
				Trace.trace("Sending a second message whilst consumer still connected");
			}
			body = UUID.randomUUID().toString().substring(0, 30);
			msg1 = new MqttMessage(body.getBytes());
			msg1.setQos(0);	
			msg1.setRetained(true);
			token = topic.publish(msg1);
			token.waitForCompletion(5000);
			jmsMessage = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage, body, 0);
			
			if(Trace.traceOn())
			{
				Trace.trace("Now sending a non retained message which should be converted to retained by mqconnectivity for a new consumer");
			}
			body = UUID.randomUUID().toString().substring(0, 30);
			msg1 = new MqttMessage(body.getBytes());
			msg1.setQos(0);	
			token = topic.publish(msg1);
			token.waitForCompletion(5000);
			// Check that we still see retained even for a non retained message as mqconnectivity converts it
			mqDestination = consumerSession.createTopic(mqDestinationName);
			consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.MQ, mqhostname, mqport, mqqueuemanager);
			jmsConsumer = consumerSession.createConsumer(mqDestination);
			consumerJMSSession.startConnection();
			jmsMessage = jmsConsumer.receive(timeout);
			this.checkContents(jmsMessage, body, 1);
			
			
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
		catch(MqttException mqtte)
		{
			if(Trace.traceOn())
			{
				Trace.trace("An exception occurred whilst attempting the test. Exception=" + mqtte.getMessage());
			}
			System.out.println("\nAn exception occurred whilst sending a message. Exception=" + mqtte.getMessage());
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
