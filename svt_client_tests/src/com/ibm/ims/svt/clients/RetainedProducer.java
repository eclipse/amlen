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
package com.ibm.ims.svt.clients;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Set;
import java.util.UUID;

import javax.jms.Message;
import javax.jms.MessageProducer;
import javax.jms.Session;

public class RetainedProducer {
	
	private static RetainedProducer instance = null;
	private static int retainedNumber = 0;

	protected RetainedProducer() 
	{
		     
	}
		   
	public static RetainedProducer getInstance() 
	{
		      
		if(instance == null) 
		{
		     instance = new RetainedProducer();
		}
		      
		return instance;
		   
	}
	
	
	public synchronized static Message sendRetainedMessage(MessageProducer producer, Session session, String clientID, HashMap<String, String> properties) throws Exception
	{
		try
		{
			Message msg = session.createTextMessage(retainedNumber + "_RETAINED_" + UUID.randomUUID().toString().substring(0, 20) + "." + clientID);
			msg.setIntProperty("JMS_IBM_Retain", 1);
			Set<String> keys = properties.keySet();
			Iterator<String> keyItor = keys.iterator();
			while(keyItor.hasNext())
			{
				String key = keyItor.next();
				msg.setStringProperty(key, properties.get(key));
			}
			producer.send(msg);
			retainedNumber++;
			return msg;
		}
		catch(Exception e)
		{
			
			throw e;
		}
	}
	

}
