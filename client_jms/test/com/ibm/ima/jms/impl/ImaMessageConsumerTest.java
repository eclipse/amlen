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

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.Session;
import javax.jms.Topic;
import javax.naming.Reference;

import junit.framework.TestCase;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaConnectionFactory;
import com.ibm.ima.jms.impl.ImaMessageConsumer;
import com.ibm.ima.jms.impl.ImaMessageProducer;
import com.ibm.ima.jms.impl.ImaSession;
import com.ibm.ima.jms.impl.ImaTopic;

/*
 * Implement test for the message producer and consumer.
 * 
 * The primary point of these tests is to test that we
 */
public class ImaMessageConsumerTest extends TestCase {
    public ImaMessageConsumerTest() {
        super("IMA JMS Client message consumer test");
    }
    ConnectionFactory fact; 
    Connection conn;
    Topic topic;
    Session sess;
    ImaMessageConsumer msgcon; 
    ImaMessageProducer msgpro;
    
    /* 
     * @see junit.framework.TestCase#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
        fact = ImaJmsFactory.createConnectionFactory(); 
        ((ImaProperties)fact).put("Port", "1234");
        conn = fact.createConnection();
        topic = ImaJmsFactory.createTopic("fred");
        ((ImaProperties)topic).put("Destination", "rmm://239.1.2.3:1234");
        sess = conn.createSession(false, 0);
        msgcon = new ImaMessageConsumer((ImaSession)sess, topic, null, false, false, null, 0);
    }

    /* 
     * @see junit.framework.TestCase#tearDown()
     */
    protected void tearDown() throws Exception {
        super.tearDown();
    }
    /*
     * Test bytes message (common to all message types)
     */
    public void testConsume() throws Exception {

        assertTrue(true);    
        
    }
    
    /*
     * Test reference
     */
    public void testReference() throws Exception {
        Reference ref = ((ImaConnectionFactory)fact).getReference();
        @SuppressWarnings("unused")
        ConnectionFactory fact2 = (ConnectionFactory)((ImaConnectionFactory)fact).getObjectInstance(ref, null, null, null);
        
        ref = ((ImaTopic)topic).getReference();
        @SuppressWarnings("unused")
        Destination dest2 = (Destination)((ImaTopic)topic).getObjectInstance(ref, null, null, null);
    }
    
    /*
     * Main test 
     */
    public void main(String args[]) {
        try {
            new ImaMessageConsumerTest().testConsume();
        } catch (Exception e) {
            e.printStackTrace();
        } 
    }

    

    
}
