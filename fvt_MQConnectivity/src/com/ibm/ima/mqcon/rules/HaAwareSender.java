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
package com.ibm.ima.mqcon.rules;

import java.util.UUID;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.MessageProducer;
import javax.jms.Queue;
import javax.jms.Session;
import javax.jms.TextMessage;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

public class HaAwareSender {

	private static ConnectionFactory connectionFactory = null;

	public static void main(String[] args) throws Exception
	{
		HaAwareSender send = new HaAwareSender();
		connectionFactory = ImaJmsFactory.createConnectionFactory();
		((ImaProperties) connectionFactory).put("Server", "9.3.177.39, 9.3.177.40");
		((ImaProperties) connectionFactory).put("Port", 16112);
		send.sendMessage();
	}

	public void sendMessage()
	{
		try
		{
			Connection connection = connectionFactory.createConnection();
			String clientID = UUID.randomUUID().toString().substring(0, 10);
			System.out.println("ClientID="+clientID);
			connection.setClientID(clientID);			
			Session session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
			Queue destination = session.createQueue("ISMQueue.To.MQTopic");
			MessageProducer producer = session.createProducer(destination);
			connection.start();
			int i=0;
			while(true)
			{
				String messagebody = ("Message number " + i);				
				TextMessage message = session.createTextMessage(messagebody);
				message.setJMSDeliveryMode(DeliveryMode.PERSISTENT);
				producer.send(message);	
				System.out.println("Sent message " + i);
				Thread.sleep(5000);
				i++;
				if(i==10)
				{
					break;
				}
			}
			producer.close();
			session.close();
			connection.close();
		}
		catch(Exception e)
		{
			e.printStackTrace();
			try
			{
			Thread.sleep(5000);
			}catch(Exception ef)
			{
				//do nothing
			}
			this.sendMessage();
		}
	}

	public void resetConnections()
	{
		this.sendMessage();
	}
}
