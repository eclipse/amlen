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

import com.ibm.ima.mqcon.utils.AsynchronousMqttListener;
import com.ibm.ima.mqcon.utils.MqttConnection;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.ima.mqcon.utils.Trace;

public class MqttRetained_MqToImaDisabled extends MqttMqToImaBaseClass{

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		retained = true;
		MqttRetained_MqToImaDisabled testClass = new MqttRetained_MqToImaDisabled();
		testClass.setup(args);
	}
	
	
	@Override
	public void runTest()
	{
		try
		{
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message consumer");
			}
			String clientID2 = UUID.randomUUID().toString().substring(0, 10).replace('-', '_');
			mqttConsumerConnection = new MqttConnection(imahostname, imaport, clientID2, true, 2);
			consumer = mqttConsumerConnection.createClient(new AsynchronousMqttListener(this));
			consumer.subscribe(imaDestinationName, 2);
			// Although we are sending a retained message the mapping used is not a retained one and
			// thus we expect the message to MQ to also not be retained
			retainedFlag = 0;
			consumer.unsubscribe(imaDestinationName); // need to unsubscribe first
			body = UUID.randomUUID().toString().substring(0, 30);
			MqttMessage msg1 = new MqttMessage(body.getBytes());
			
			msg1.setQos(0);	
			msg1.setRetained(true);
			if(Trace.traceOn())
			{
				Trace.trace("Sending the retained message to IMA");
			}
			MqttDeliveryToken token = topic.publish(msg1);
			token.waitForCompletion(5000);
			consumer.subscribe(imaDestinationName);
			// do nothing and wait until message received
			StaticUtilities.sleep(8000);
			
			if(completed == true)
			{
				if(Trace.traceOn())
				{
					Trace.trace("We got the message and it was not retained. Now disconnecting the client to send again");
				}
				completed = false;
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
			
			consumer.unsubscribe(imaDestinationName);
			body = UUID.randomUUID().toString().substring(0, 30);
			msg1 = new MqttMessage(body.getBytes());
			System.out.println("body " + body);
			msg1.setQos(0);	
			msg1.setRetained(true);
			if(Trace.traceOn())
			{
				Trace.trace("Sending the second retained message to IMA");
			}
			token = topic.publish(msg1);
			token.waitForCompletion(5000);
			StaticUtilities.sleep(4000);
			consumer.subscribe(imaDestinationName);
			// now check to see that we didnt get a message
			StaticUtilities.sleep(4000);
			if(completed == false)
			{
				if(Trace.traceOn())
				{
					Trace.trace("We didn't get any retained message so all is well as mapping is disabled for retained");
				}
				if(Trace.traceOn())
				{
					Trace.trace("Test passed");
				}
				RC = ReturnCodes.PASSED;
				System.out.println("Test.*Success!");
				consumer.unsubscribe(mqDestinationName);
				closeConnections();
				clearRetainedMessages();
				System.exit(RC);
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("An unexpected message was returned in this test");
				}
				RC = ReturnCodes.INCORRECT_MESSAGE;
				closeConnections();
				clearRetainedMessages();
				System.exit(RC);
			}
		
			
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
