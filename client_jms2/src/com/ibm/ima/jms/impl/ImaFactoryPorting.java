/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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


package com.ibm.ima.jms.impl;

import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.Queue;
import javax.jms.QueueConnectionFactory;
import javax.jms.Topic;
import javax.jms.TopicConnectionFactory;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.sun.ts.lib.porting.TSJMSObjectsInterface;

/**
 * Create IMA JMS administered objects for the JMS TCK.
 * This class implements an interfaces in the JMS TCK which is available
 * either in the TCK jar file, or in porting.jar.
 */
public class ImaFactoryPorting implements TSJMSObjectsInterface {
    
    /*
     * Get a queue..
     * @see com.sun.ts.lib.porting.TSJMSObjectsInterface#getQueue(java.lang.String)
     */
	public Queue getQueue(java.lang.String name) throws java.lang.Exception {
		Destination dest = ImaJmsFactory.createQueue(name);

		return (Queue)dest;
	}
	
	
	/*
	 * Get a topic.
	 * @see com.sun.ts.lib.porting.TSJMSObjectsInterface#getTopic(java.lang.String)
	 */
	public Topic getTopic(java.lang.String name) throws java.lang.Exception {
		Destination dest = ImaJmsFactory.createTopic(name);

		return (Topic)dest;
	}
	
	/*
	 * Get a topic connection factory.
	 * @see com.sun.ts.lib.porting.TSJMSObjectsInterface#getTopicConnectionFactory(java.lang.String)
	 */
	public TopicConnectionFactory getTopicConnectionFactory(String name) throws java.lang.Exception {
	    return (TopicConnectionFactory)getConnectionFactory(name);
	}
	
	/*
	 * Get a queue connection factory.
	 * @see com.sun.ts.lib.porting.TSJMSObjectsInterface#getQueueConnectionFactory(java.lang.String)
	 */
	public QueueConnectionFactory getQueueConnectionFactory(java.lang.String name) throws java.lang.Exception {
		return (QueueConnectionFactory)getConnectionFactory(name);
	}
	
	/*
     * Get a queue connection factory.
     * @see com.sun.ts.lib.porting.TSJMSObjectsInterface#getQueueConnectionFactory(java.lang.String)
     */
    public ConnectionFactory getConnectionFactory(java.lang.String name) throws java.lang.Exception {
        ConnectionFactory connFact = ImaJmsFactory.createConnectionFactory();
        /* 
         * Set minimal properties for the connection.
         * The default properties include setting localhost as the network interface. 
         */
        ImaProperties props = (ImaProperties)connFact;
        String server = System.getProperty("IMAServer");
        String port   = System.getProperty("IMAPort");
        if (server != null)
            props.put("Server", server);
        else {
            props.put("Server", "127.0.0.1");
        }
        if (port == null)
            port = "16102";
        props.put("Port", port);
        if ("DURABLE_SUB_CONNECTION_FACTORY".equals(name)) {
            props.put("ClientID", "cts");
        }
        return connFact;
    }
}
