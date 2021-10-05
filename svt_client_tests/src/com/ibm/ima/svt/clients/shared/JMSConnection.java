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
package com.ibm.ima.svt.clients.shared;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.Session;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaTopic;

/*
 * A JMS connection class.
 * 
 * Can create a connection with or without a client ID.
 * 
 * Will be used for creating shared consumers.
 */

public class JMSConnection {

	ConnectionFactory   factory;
    Connection          connection;
    Session             session;
    Topic  		       	topic;
    
    String				serverIP;
    String				port;
    int					messageCache;
	String				topicName;
    String				clientID;
    
    public JMSConnection(String serverIP, String port, int messageCache, String topicName) {
			this.serverIP = serverIP;
			this.port = port;
			this.messageCache = messageCache;
			this.topicName = topicName;
			createConnection();
	}
    
    public JMSConnection(String serverIP, String port, int messageCache, String topicName, String clientID) {
			this.serverIP = serverIP;
			this.port = port;
			this.messageCache = messageCache;
			this.topicName = topicName;
			this.clientID = clientID;
			createConnection();
	}
    
    public void createConnection(){
		try {
			factory = ImaJmsFactory.createConnectionFactory();
			ImaProperties props = (ImaProperties)factory;
	        ((ImaProperties)props).put("Server", serverIP);
	        ((ImaProperties)props).put("Port", port);
	        if(clientID!=null)
	        	((ImaProperties)props).put("ClientID", clientID);
	        ((ImaProperties)props).put("ClientMessageCache", messageCache);
	        connection = factory.createConnection();
	        session  = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
	        topic = session.createTopic(topicName);
	        ((ImaTopic)topic).put("ClientMessageCache", messageCache);
	        
	        ImaJmsException [] errstr = props.validate(ImaProperties.WARNINGS);
	        if (errstr == null) {
	        	//Ignore
	        } else {
	            // Display the validation errors
	            for (int i=0;  i<errstr.length; i++)
	                System.out.println(""+errstr[i]);
	        }
		} catch (JMSException e) {
			e.printStackTrace();
		}
    }
}
