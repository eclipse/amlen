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
import javax.jms.Topic;

import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.MqttConnection;
import com.ibm.ima.mqcon.utils.Trace;

public abstract class MqttToJmsImaBaseClass extends TestHelper{
	
	protected MqttTopic topic = null;

	public void setup(String args[])
	{
		
		new Trace().traceAutomation();
		
		// Get the hostname and port passed in as a parameter
		try
		{
			if (args.length==0)
			{
				if(Trace.traceOn())
				{
					Trace.trace("\nTest aborted, test requires command line arguments!! (ima IP address, ima port, ima topic, timeout)");
				}
				RC = ReturnCodes.INCORRECT_PARAMS;
				System.exit(RC);
			}
			else if(args.length == 4)
			{
				imahostname = args[0];
				imaport = new Integer((String)args[1]);
				imaDestinationName = args[2];
				timeout = new Integer((String)args[3]);
				
				if(Trace.traceOn())
				{
					Trace.trace("The arguments passed into test class are: " + imahostname + ", " + imaport + ", "  + imaDestinationName + ", " + timeout);
				}
			}
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("\nInvalid Commandline arguments! System Exit.");
				}
				RC = ReturnCodes.INCORRECT_PARAMS;
				System.exit(RC);
			}
			if(retained)
			{
				this.clearRetainedIMAMessage();
			}
			this.setUpClients();
			this.runTest();
									
		}
		catch(Exception e)
		{
			e.printStackTrace();
			System.exit(RC);
		}	
	}
	
	public void setUpClients()
	{
		try
		{
							
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message producer");
			}
			String clientID = UUID.randomUUID().toString().substring(0, 10).replace('-', '_');
			mqttProducerConnection = new MqttConnection(imahostname, imaport, clientID, true, 0);
			
			if(!retained)
			{
				if(Trace.traceOn())
				{
					Trace.trace("Creating the message consumer and starting the connection");
				}
				consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null);
				Topic imaDestination2 = consumerSession.createTopic(imaDestinationName);
				jmsConsumer = consumerSession.createConsumer(imaDestination2);
				consumerJMSSession.startConnection();
			}
			
			mqttProducerConnection.setPublisher();
			producer = mqttProducerConnection.getClient();
			topic =  producer.getTopic(imaDestinationName);
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
		
	}
	
	public void closeConnections()
	{
		try
		{
			consumerJMSSession.closeConnection();
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			mqttProducerConnection.destroyClient();
		}
		catch(Exception e)
		{
			// do nothing
		}
	}
	
	

}
