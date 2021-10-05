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

import javax.jms.Topic;

import org.eclipse.paho.client.mqttv3.MqttException;

import com.ibm.ima.mqcon.utils.AsynchronousMqttListener;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.MqttConnection;
import com.ibm.ima.mqcon.utils.Trace;

public abstract class JmsToMqttMqToImaBaseClass extends TestHelper{

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
					Trace.trace("Test aborted, test requires command line arguments!! (IMA IP address, IMA port, MQ IP address, MQ port, MQ queue manager, IMA topic, MQ topic, timeout for not retained tests or)");
					Trace.trace("Test aborted, test requires command line arguments!! (IMA IP address, IMA port, MQ IP address, MQ port, MQ queue manager, MQ user, MQ password, MQ location, IMA topic, MQ topic, timeout for retained tests)");
				}
				RC = ReturnCodes.INCORRECT_PARAMS;
				System.exit(RC);
			}
			else if(args.length == 8)
			{
				imahostname = args[0];
				imaport = new Integer((String)args[1]);
				mqhostname = args[2];
				mqport = new Integer((String)args[3]);
				mqqueuemanager = args[4];
				imaDestinationName = args[5];
				mqDestinationName = args[6];
				timeout = new Integer((String)args[7]);
				
				if(Trace.traceOn())
				{
					Trace.trace("The arguments passed into test class are: " + imahostname + ", " + imaport + ", " + mqhostname + ", " + mqport + ", " + mqqueuemanager + ", " + imaDestinationName + ", " + mqDestinationName + ", " + timeout);
				}
			}
			else if(args.length == 10)
			{
				imahostname = args[0];
				imaport = new Integer((String)args[1]);
				mqhostname = args[2];
				mqport = new Integer((String)args[3]);
				mqqueuemanager = args[4];
				mquser = args[5];
				mqlocation = args[6];
				imaDestinationName = args[7];
				mqDestinationName = args[8];
				timeout = new Integer((String)args[9]);
				
				if(Trace.traceOn())
				{
					Trace.trace("The arguments passed into test class are: " + imahostname + ", " + imaport + ", " + mqhostname + ", " + mqport + ", " + mqqueuemanager + ", " + mquser + ", " + imaDestinationName + ", " + mqDestinationName + ", " + mqlocation +  ", " + timeout);
				}
			}
			else if(args.length == 11)
			{
				imahostname = args[0];
				imaport = new Integer((String)args[1]);
				mqhostname = args[2];
				mqport = new Integer((String)args[3]);
				mqqueuemanager = args[4];
				mquser = args[5];
				mqpassword = args[6];
				mqlocation = args[7];
				imaDestinationName = args[8];
				mqDestinationName = args[9];
				timeout = new Integer((String)args[10]);
				
				
				if(Trace.traceOn())
				{
					Trace.trace("The arguments passed into test class are: " + imahostname + ", " + imaport + ", " + mqhostname + ", " + mqport + ", " + mqqueuemanager + ", " + mquser + ", " + mqpassword+ ", " + imaDestinationName + ", " + mqDestinationName + ", " + mqlocation +  ", " + timeout);
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
				this.clearRetainedMessages();
			}
			this.setUpClients();
			this.runTest();
									
		}
		catch(Exception e)
		{
			e.printStackTrace();
			if(retained)
			{
				this.closeConnections();
				this.clearRetainedMessages();
			}
			System.exit(RC);
		}	
	}
	
	public void setUpClients() throws Exception
	{
		try
		{
			
			producerSession = producerJMSSession.getSession(JmsSession.CLIENT_TYPE.MQ, mqhostname, mqport, mqqueuemanager);
			
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message producer");
			}
			Topic mqDestination = producerSession.createTopic(mqDestinationName);
			jmsProducer = producerSession.createProducer(mqDestination);			
			
			if(!retained)
			{
				if(Trace.traceOn())
				{
					Trace.trace("Creating the message consumer and starting the connection");
				}
				String clientID = UUID.randomUUID().toString().substring(0, 10).replace('-', '_');
				mqttConsumerConnection = new MqttConnection(imahostname, imaport, clientID, true, 0);
				AsynchronousMqttListener listener = new AsynchronousMqttListener(this);
				consumer = mqttConsumerConnection.createClient(listener);
				consumer.subscribe(imaDestinationName);
			}
		}
		catch(MqttException me)
		{
			if(Trace.traceOn())
			{
				Trace.trace("An exception occurred whilst attempting the test. Exception=" + me.getMessage());
			}
			throw new Exception(me.getMessage());
		
		}
		catch(Exception e)
		{
			if(Trace.traceOn())
			{
				Trace.trace("An exception occurred whilst attempting the test. Exception=" + e.getMessage());
			}
			throw new Exception(e.getMessage());
		
		}
	}
	
	public void closeConnections()
	{
		try
		{
			producerJMSSession.closeConnection();
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			mqttConsumerConnection.destroyClient();
		}
		catch(Exception e)
		{
			// do nothing
		}
	}
	
	
	public void clearRetainedMessages()
	{
		this.clearRetainedMQMessage();
		this.clearRetainedIMAMessage();
	}
	
	

}
