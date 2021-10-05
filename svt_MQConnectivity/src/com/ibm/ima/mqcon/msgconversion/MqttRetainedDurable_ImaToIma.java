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

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import com.ibm.ima.mqcon.utils.AsynchronousMqttListener;
import com.ibm.ima.mqcon.utils.MqttConnection;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.ima.mqcon.utils.Trace;

public class MqttRetainedDurable_ImaToIma extends MqttImaBaseClass
{
	public static void main(String[] args) 
	{
		retained = true;
		MqttRetainedDurable_ImaToIma testClass = new MqttRetainedDurable_ImaToIma();
		testClass.setup(args);
	}

	@Override
	public void runTest()
	{
		try
		{
			retainedFlag = 1;
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
			if(Trace.traceOn())
			{
				Trace.trace("Creating the durable message consumer");
			}
			String clientID = UUID.randomUUID().toString().substring(0, 10).replace('-', '_');
			mqttConsumerConnection = new MqttConnection(imahostname, imaport, clientID, false, 2);
			consumer = mqttConsumerConnection.createClient(new AsynchronousMqttListener(this));
			consumer.subscribe(imaDestinationName, 2);
			// do nothing and wait until message received
			StaticUtilities.sleep(8000);
			if(completed == true)
			{
				// carry on
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
			if(Trace.traceOn())
			{
				Trace.trace("Creating the non durable message consumer");
			}
			String clientID2 = UUID.randomUUID().toString().substring(0, 10).replace('-', '_');
			MqttConnection mqttConsumerConnection2 = new MqttConnection(imahostname, imaport, clientID2, true, 2);
			MqttClient consumer2 = mqttConsumerConnection2.createClient(new AsynchronousMqttListener(this));
			consumer2.subscribe(imaDestinationName, 2);
			// do nothing and wait until message received
			StaticUtilities.sleep(8000);
			if(completed == true)
			{
				// carry on
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
			
			
			if(Trace.traceOn())
			{
				Trace.trace("Disconnecting both clients");
			}
			consumer.disconnect();
			consumer2.disconnect();
			
			
			if(Trace.traceOn())
			{
				Trace.trace("Sending another retained message");
			}
	
			body = UUID.randomUUID().toString().substring(0, 30);
			msg1 = new MqttMessage(body.getBytes());
			msg1.setQos(0);	
			msg1.setRetained(true);
			token = topic.publish(msg1);
			token.waitForCompletion(5000);
			retainedFlag = 0;
			
			if(Trace.traceOn())
			{
				Trace.trace("Reconnecting the durable client");
			}
			MqttConnectOptions conOptions = new MqttConnectOptions();
			conOptions.setCleanSession(false);
			consumer.setCallback(new AsynchronousMqttListener(this));
			consumer.connect(conOptions);
					
			StaticUtilities.sleep(8000);
			if(completed == true)
			{
				// carry on
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
			// do nothing and wait until message received
			retainedFlag = 1;
			if(Trace.traceOn())
			{
				Trace.trace("Reconnecting the non durable client");
			}

			conOptions.setCleanSession(true);
			consumer2.setCallback(new AsynchronousMqttListener(this));
			consumer2.connect(conOptions);
			consumer2.subscribe(imaDestinationName, 2);
		
			StaticUtilities.sleep(8000);
			if(completed == true)
			{
				consumer2.disconnect();
				if(Trace.traceOn())
				{
					Trace.trace("Test passed");
				}
				RC = ReturnCodes.PASSED;
				System.out.println("Test.*Success!");
				consumer.unsubscribe(imaDestinationName);
				this.closeConnections();
				this.clearRetainedIMAMessage();
				System.exit(RC);
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("No message was returned in this test");
				}
				RC = ReturnCodes.MESSAGE_NOT_ARRIVED;
				this.closeConnections();
				this.clearRetainedIMAMessage();
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

