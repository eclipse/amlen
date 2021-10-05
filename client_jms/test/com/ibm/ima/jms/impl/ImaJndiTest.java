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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.util.Hashtable;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Queue;
import javax.jms.Topic;
import javax.naming.Context;
import javax.naming.InitialContext;

import junit.framework.TestCase;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaPropertiesImpl;
import com.ibm.ima.jms.impl.ImaQueue;
import com.ibm.ima.jms.impl.ImaTopic;

/**
 * Test putting the IBM Messaging Appliance JMS client admin objects into JNDI and make sure serializable works.
 */
@SuppressWarnings("unchecked")
public class ImaJndiTest extends TestCase {

    /*
     * Test the ability to bind the two IBM Messaging Appliance JMS client admin objects into fscontext.
     * This does a referenceable conversion.
     */
    public void testBind() throws JMSException {
        ConnectionFactory fact;
        @SuppressWarnings("unused")
        ConnectionFactory fact2;
        Topic  topic;
        Queue  queue;

        Topic topic2 = null;
        Queue queue2 = null;
        
        Connection conn;
        ImaProperties props;
        InitialContext ctx = null;
        
        Hashtable env = new Hashtable();
        
        /*
         * Create an initial context
         */
        env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.fscontext.RefFSContextFactory");
        env.put(Context.PROVIDER_URL, "file:.");
        try {
            ctx = new InitialContext(env);
        } catch (Exception e) {
            e.printStackTrace(System.err);
            assertTrue(false);
            return;
        }
        
        fact = ImaJmsFactory.createConnectionFactory();
        props = (ImaProperties)fact;
        props.put("Name", "jms");
        props.put("Port", "31000");
        props.put("LogLevel", 4);
        props.put("EventListener", "Connection");
        try {
            ctx.rebind("jms", fact);
        } catch (Exception e) {
            e.printStackTrace(System.err);
            assertTrue(false);
            return;
        }
        
        topic = (Topic)ImaJmsFactory.createTopic("sam");
        queue = (Queue)ImaJmsFactory.createQueue("sue");
        props = (ImaProperties)topic;
        props.put("junk", "123");
        props.put("ijunk", 3);
        props.put("djunk", 1.23);
        props = (ImaProperties)queue;
        props.put("junk", "456");
        props.put("ijunk", 4);
        props.put("djunk", 3.14159);
        try {
            ctx.rebind("topic1", topic);
            ctx.rebind("queue1", queue);
        } catch (Exception e) {
            e.printStackTrace(System.err);
            assertTrue(false);
            return;
        }
        
    
        try {
            fact2 = (ConnectionFactory)ctx.lookup("jms");
            conn = new ImaConnection(true);
            ((ImaConnection)conn).props.putAll(((ImaPropertiesImpl)props).props);
            ((ImaConnection)conn).client = ImaTestClient.createTestClient(conn);
            topic2 = (Topic)ctx.lookup("topic1");
            queue2 = (Queue)ctx.lookup("queue1");
            assertEquals("sam", topic2.getTopicName());
            assertEquals("123", ((ImaTopic)topic2).get("junk"));
            assertEquals(3, ((ImaTopic)topic2).get("ijunk"));
            assertEquals(1.23, ((ImaTopic)topic2).get("djunk"));
            assertEquals("sue", queue2.getQueueName());
            assertEquals("456", ((ImaQueue)queue2).get("junk"));
            assertEquals(4, ((ImaQueue)queue2).get("ijunk"));
            assertEquals(3.14159, ((ImaQueue)queue2).get("djunk"));
            conn.close();
        } catch (Exception e) {
            e.printStackTrace(System.err);
            assertTrue(false);
            return;
        }
        

    }
    
    
    /*
     * Simple test to serialize the two IBM Messaging Appliance JMS client admin objects
     */
    public void testSerialize() throws Exception {
        ImaProperties props;
        ImaProperties props2;
        ConnectionFactory fact2;
        
        /*
         * Construct a ConnectionFactory
         */
        ConnectionFactory fact = ImaJmsFactory.createConnectionFactory();
        props = (ImaProperties)fact;
        props.put("NetworkInterface", "127.0.0.1");
        props.put("Name", "jms");
        props.put("Port", "31000");
        props.put("MulticastLoop", "1");
        props.put("LogLevel", 4);
        props.put("LogEventListener", "this");
        props.put("EventListener", "Connection");
        
        /* 
         * Serialize the object
         */
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        ObjectOutputStream oos = new ObjectOutputStream(baos);
        oos.writeObject(fact);
        byte [] body = baos.toByteArray();
        
        /*
         * Deserialize the object
         */
        ByteArrayInputStream bais = new ByteArrayInputStream(body);
        ObjectInputStream ois = new ObjectInputStream(bais);
        Serializable obj = (Serializable) ois.readObject();
        fact2 = (ConnectionFactory)obj;
        props2 = (ImaProperties)fact2;
        assertEquals("jms", (String)props2.get("Name"));
        assertEquals(4, props2.getInt("LogLevel", 22));
        assertEquals("4", props2.getString("LogLevel"));
        assertEquals("1", props2.getString("MulticastLoop"));
        /*
         * Construct a ConnectionFactory
         */
        Destination dest2;
        Destination dest = ImaJmsFactory.createTopic("fred");
        props = (ImaProperties)dest;
        
        /* 
         * Serialize the object
         */
        baos = new ByteArrayOutputStream();
        oos = new ObjectOutputStream(baos);
        oos.writeObject(dest);
        body = baos.toByteArray();
        
        /*
         * Deserialize the object
         */
        bais = new ByteArrayInputStream(body);
        ois = new ObjectInputStream(bais);
        obj = (Serializable) ois.readObject();
        dest2 = (Destination)obj;
        props2 = (ImaProperties)dest2;
    }

    /*
     * Test that the sample from the javadoc compiles correctly
     */
    public static void javadoc() throws JMSException {
       // Create a connection factory
        ConnectionFactory cf = ImaJmsFactory.createConnectionFactory();
        // Set the desired properties
        ImaProperties props = (ImaProperties)cf;
        props.put("Application", "MyApp");
        // Validate the properties.  A null return means that the properties are valid.
        ImaJmsException [] errstr = props.validate(ImaProperties.WARNINGS);
        if (errstr == null) {
            // Open the initial context
            InitialContext ctx;
       
            Hashtable env = new Hashtable();
            env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.fscontext.RefFSContextFactory");
            env.put(Context.PROVIDER_URL, "file:.");
            try {
                ctx = new InitialContext(env);
                // Bind the connection factory to a name
                ctx.rebind("ConnName", cf);
            } catch (Exception e) {
                System.out.println("Unable to open an initial context: " + e);
            }
        } else {
            // Display the validation errors
            for (int i=0; i<errstr.length; i++)
                System.out.println(""+errstr[i]);
        }
    }
}
