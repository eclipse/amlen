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

import com.ibm.ima.mqcon.utils.JmsMessage;
import com.ibm.ima.mqcon.utils.StaticUtilities;
import com.ibm.ima.mqcon.utils.Trace;

/**
 * Test sends a JMS Base message to an ima topic
 * which is received by an MQTT client listening
 * on an ima topic.
 * 
 * It checks that the text sent in the message body
 * is correctly preserved.
 * 
 * Subscriber = MQTT
 * Session = non transacted and auto acknowledgement
 * Message = Bytes and non persistent
 * Headers = Set
 *
 */
public class JmsBaseToMqtt_ImaToIma extends JmsToMqttImaBaseClass{
	
	public static void main(String[] args) 
	{
		JmsBaseToMqtt_ImaToIma testClass = new JmsBaseToMqtt_ImaToIma();
		testClass.setup(args);
	}
	
	@Override
	public void runTest()
	{
		try
		{
			msg = JmsMessage.createMessage(producerSession, JmsMessage.MESSAGE_TYPE.BASE);
			if(Trace.traceOn())
			{
				Trace.trace("Sending the message to ima");
			}
			jmsProducer.send(msg);
			
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
