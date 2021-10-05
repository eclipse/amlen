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

import java.util.Hashtable;

import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.naming.Context;
import javax.naming.NamingException;
import javax.naming.directory.InitialDirContext;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.msg.client.jms.JmsConnectionFactory;
import com.ibm.msg.client.jms.JmsConstants;
import com.ibm.msg.client.jms.JmsFactoryFactory;
import com.ibm.msg.client.wmq.WMQConstants;



public class JmsConnection {

	private ConnectionFactory cf = null;
	
	/**
	 * @param jndi - location of file JNDI
	 * @param connectionFactoryName - to look up CF name from file JNDI
	 * @throws NamingException
	 */
	public JmsConnection(String jndi, String connectionFactoryName) throws NamingException
	{
		Hashtable<String, String> env = new Hashtable<String, String>();
		env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.fscontext.RefFSContextFactory");
		env.put(Context.PROVIDER_URL, jndi);	
		Context context = new InitialDirContext(env);
		cf = (ConnectionFactory) context.lookup(connectionFactoryName);
	}	
	
	/**
	 * 
	 * @param ipaddress of mq server
	 * @param port of mq server
	 * @param queueManager - name of qmgr
	 * @param channel - channel to use to connect
	 * @throws JMSException
	 */
	public JmsConnection(String ipaddress, int port, String queueManager, String channel) throws JMSException
	{
		JmsFactoryFactory jmsff = JmsFactoryFactory.getInstance(JmsConstants.WMQ_PROVIDER);
		JmsConnectionFactory connectionFactory = jmsff.createConnectionFactory();
		connectionFactory.setIntProperty(WMQConstants.WMQ_CONNECTION_MODE, WMQConstants.WMQ_CM_CLIENT);
		connectionFactory.setStringProperty(WMQConstants.WMQ_HOST_NAME, ipaddress);
		connectionFactory.setIntProperty(WMQConstants.WMQ_PORT, port);
		connectionFactory.setStringProperty(WMQConstants.WMQ_QUEUE_MANAGER, queueManager);
		connectionFactory.setStringProperty(WMQConstants.WMQ_CHANNEL, channel);		
		cf = connectionFactory;
		
	}
	
	/**
	 * 
	 * @param ipaddress of IMS server
	 * @param port of IMS server
	 * @throws JMSException
	 */
	public JmsConnection(String ipaddress, int port) throws JMSException
	{
		ConnectionFactory connectionFactory = ImaJmsFactory.createConnectionFactory();
		((ImaProperties) connectionFactory).put("Server", ipaddress);
		((ImaProperties) connectionFactory).put("Port", port);
		cf = connectionFactory;
	}
	
	/**
	 * 
	 * @return the connection factory
	 * @throws NamingException
	 */
	public ConnectionFactory getConnectionFactory() throws NamingException
	{
		return cf;
	}

}
