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

import javax.jms.JMSException;
import javax.jms.Session;
import javax.jms.Topic;

import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.Trace;

public abstract class JmsImaBaseClass extends TestHelper{

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
			if(!transacted)
			{
				producerSession = producerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null);
			}
			else
			{
				producerSession = producerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null, true, Session.AUTO_ACKNOWLEDGE);
			}
			
			if(!retained)
			{
				if(Trace.traceOn())
				{
					Trace.trace("Creating the IMA consumer");
				}
				consumerSession = consumerJMSSession.getSession(JmsSession.CLIENT_TYPE.IMA, imahostname, imaport, null);
			}
			
			
			
			if(Trace.traceOn())
			{
				Trace.trace("Creating the message producer");
			}
			Topic imaDestination = producerSession.createTopic(imaDestinationName);
			jmsProducer = producerSession.createProducer(imaDestination);
			
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
			consumerJMSSession.closeConnection();
		}
		catch(Exception e)
		{
			// do nothing
		}
	}
	
	

}
