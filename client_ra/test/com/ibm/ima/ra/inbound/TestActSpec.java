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
package com.ibm.ima.ra.inbound;

import javax.jms.Destination;
import javax.jms.Queue;
import javax.jms.Topic;
import javax.naming.InitialContext;
import javax.resource.spi.ActivationSpec;
import javax.resource.spi.ResourceAdapter;

import junit.framework.TestCase;

import com.ibm.ima.jms.impl.ImaQueue;
import com.ibm.ima.jms.impl.ImaTopic;
import com.ibm.ima.ra.ImaResourceAdapter;
//import com.ibm.ima.ra.testutils.MockBootstrapContext;

public class TestActSpec extends TestCase {
    
    static {
        System.setProperty("java.naming.factory.initial", "com.sun.jndi.fscontext.RefFSContextFactory");
        System.setProperty("java.naming.provider.url", "file:.");
    }
    
    public void setUp() {
    }
    
    public TestActSpec() {
        super("IMA JMS Resource Adapter ActSpec test");
    }
    
    public void testActSpec() throws Exception {
        assertEquals("javax.jms.Queue", ImaActivationSpec.QUEUE);
        assertEquals("javax.jms.Topic", ImaActivationSpec.TOPIC);
        assertEquals("Durable", ImaActivationSpec.DURABLE);
        assertEquals("NonDurable", ImaActivationSpec.NON_DURABLE);
        
        ActivationSpec actSpec = new ImaActivationSpec();
        
        // RA is null by default
        assertEquals(null, actSpec.getResourceAdapter());
        
        actSpec.setResourceAdapter(null);
        assertEquals(null, actSpec.getResourceAdapter());
        
        ResourceAdapter ra = new ImaResourceAdapter();
        actSpec.setResourceAdapter(ra);
        assertEquals(ra, actSpec.getResourceAdapter());
        
        ResourceAdapter ra2 = new ImaResourceAdapter();
        actSpec.setResourceAdapter(ra2);
        
        // Confirm resource adapter is unchanged after 2nd call to set
        assertEquals(ra, actSpec.getResourceAdapter());
        
        // Confirm resource adapter is unchanged
        actSpec.setResourceAdapter(null);
        assertEquals(ra, actSpec.getResourceAdapter());
        
        // MessageSight will not support ASF in 2013Q4
        assertFalse(((ImaActivationSpec)actSpec).isASF());
        
        // Confirm that setting destination type does not alter destination
        assertEquals(null, ((ImaActivationSpec)actSpec).getDestination());
        
        ((ImaActivationSpec)actSpec).setDestinationType("javax.jms.Topic");
        assertEquals(ImaActivationSpec.TOPIC, ((ImaActivationSpec)actSpec).getDestinationType());

// For validation, we no longer force correct values
// TODO: See if there's a way to see validation via unit tests
//        ((ImaActivationSpec)actSpec).setDestinationType(null);
//        assertEquals("javax.jms.Topic", ((ImaActivationSpec)actSpec).getDestinationType());
        
        ((ImaActivationSpec)actSpec).setDestination("MyTopic");
        assertEquals("MyTopic", ((ImaActivationSpec)actSpec).getDestination());
        
        ((ImaActivationSpec)actSpec).setDestination(null);
        assertEquals(null, ((ImaActivationSpec)actSpec).getDestination());
        
        // By default, subscriptionName is null
        assertEquals(null, ((ImaActivationSpec)actSpec).getSubscriptionName());
        
        ((ImaActivationSpec)actSpec).setSubscriptionName("subName");        
        assertEquals("subName", ((ImaActivationSpec)actSpec).getSubscriptionName());
        
        ((ImaActivationSpec)actSpec).setSubscriptionName(null);        
        assertEquals(null, ((ImaActivationSpec)actSpec).getSubscriptionName());
        
        // By default, subscriptionDurablity is NonDurable
        assertEquals(ImaActivationSpec.NON_DURABLE, ((ImaActivationSpec)actSpec).getSubscriptionDurability());

// For validation, we no longer force correct values
// TODO: See if there's a way to see validation via unit tests
//        ((ImaActivationSpec)actSpec).setSubscriptionDurability("invalidDurabilityValue");
//        assertEquals("NonDurable", ((ImaActivationSpec)actSpec).getSubscriptionDurability());
        
        ((ImaActivationSpec)actSpec).setSubscriptionDurability("Durable");
        assertEquals(ImaActivationSpec.DURABLE, ((ImaActivationSpec)actSpec).getSubscriptionDurability());
 
// For validation, we no longer force correct values
// TODO: See if there's a way to see validation via unit tests
//        ((ImaActivationSpec)actSpec).setSubscriptionDurability(null);
//        assertEquals("Durable", ((ImaActivationSpec)actSpec).getSubscriptionDurability());
        
        // By default, messageSelector is null
        assertEquals(null, ((ImaActivationSpec)actSpec).getMessageSelector());
        
        ((ImaActivationSpec)actSpec).setMessageSelector("msgProp = 'myprop'");
        assertEquals("msgProp = 'myprop'", ((ImaActivationSpec)actSpec).getMessageSelector());
        
        ((ImaActivationSpec)actSpec).setMessageSelector(null);
        assertEquals(null, ((ImaActivationSpec)actSpec).getMessageSelector());
        
        // TODO: Add code or remove this comment when final decision is made.
        // Intentionally omitting tests for nonASFRollbackEnabled.
        
        // By default, concurrentConsumers is 1
        assertEquals("1", ((ImaActivationSpec) actSpec).getConcurrentConsumers());
        
// For validation, we no longer force correct values
// TODO: See if there's a way to see validation via unit tests
//        ((ImaActivationSpec)actSpec).setConcurrentConsumers("-1");
//        // Confirm min value of 1 is enforced
//        assertEquals("1", ((ImaActivationSpec)actSpec).getConcurrentConsumers());
//        
//        ((ImaActivationSpec)actSpec).setConcurrentConsumers("11");
//        // Confirm max value of 10 is enforced
//        assertEquals("10", ((ImaActivationSpec)actSpec).getConcurrentConsumers());
        
        ((ImaActivationSpec) actSpec).setConcurrentConsumers("5");
        assertEquals("5", ((ImaActivationSpec) actSpec).getConcurrentConsumers());
        
        // Value setting is forced to invalid -99 so validation can report 
        ((ImaActivationSpec) actSpec).setConcurrentConsumers(null);
        assertEquals("0", ((ImaActivationSpec) actSpec).getConcurrentConsumers());
    }
    
    public void testGetDestObjNoJNDI() throws Exception {
        ActivationSpec actSpec = new ImaActivationSpec();
        ((ImaActivationSpec)actSpec).setDestination("MyQueue");
        ((ImaActivationSpec)actSpec).setDestinationType(ImaActivationSpec.QUEUE);
        
        // First get the default destination object
        Destination dest = ((ImaActivationSpec)actSpec).getDestinationObject();
        assertTrue(dest instanceof Queue);

        assertEquals("MyQueue", ((Queue)dest).getQueueName());
        
        ((ImaActivationSpec)actSpec).setDestinationType(ImaActivationSpec.TOPIC);
        ((ImaActivationSpec)actSpec).setDestination("MyTopic");
        
        dest = ((ImaActivationSpec)actSpec).getDestinationObject();
        assertTrue(dest instanceof Topic);
        assertEquals("MyTopic", ((Topic)dest).getTopicName());       
    }
    
    public void testGetDestObjUseJNDI() throws Exception {    
        createJndiObjs();
        
        ActivationSpec actSpec = new ImaActivationSpec();
        
        // First get the queue JNDI destination object
        ((ImaActivationSpec)actSpec).setDestinationLookup("JndiQueue");
        Destination dest = ((ImaActivationSpec)actSpec).getDestinationObject();
        assertTrue(dest instanceof Queue);
        assertEquals("JndiMyQueue", ((Queue)dest).getQueueName());
        
        ((ImaActivationSpec)actSpec).setDestinationType(ImaActivationSpec.TOPIC);
        ((ImaActivationSpec)actSpec).setDestination("MyTopic");
        
        // Then get the topic JNDI destination object
        ((ImaActivationSpec)actSpec).setDestinationLookup("JndiTopic");
        dest = ((ImaActivationSpec)actSpec).getDestinationObject();
        assertTrue(dest instanceof Topic);
        assertEquals("JndiMyTopic", ((Topic)dest).getTopicName());
    }
    
    /* Test helper methods */
    private void createJndiObjs() throws Exception{
        InitialContext ctx = new InitialContext();
        
        Queue testQueue = new ImaQueue("JndiMyQueue");
        Topic testTopic = new ImaTopic("JndiMyTopic");
        
        ctx.rebind("JndiQueue", testQueue);
        ctx.rebind("JndiTopic", testTopic);
    }
}
