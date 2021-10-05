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

import java.util.Enumeration;

import javax.jms.JMSException;
import javax.naming.RefAddr;
import javax.naming.Reference;

import junit.framework.TestCase;

import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaQueue;
import com.ibm.ima.jms.impl.ImaTopic;

public class ImaDestinationTest extends TestCase {
	
    public ImaDestinationTest() {
        super("IMA Destination test");
    }
    
    public void testIsmDestinationConstructor() throws Exception {
    	
    	String name = "TopicA";
    	ImaTopic dest = new ImaTopic(name);
    	
    	assertTrue(dest!=null);
    	assertEquals(dest.getTopicName(), name);
    	assertEquals(dest.getString("Name"),name);
    	assertEquals(""+dest.get("Name"),name);  
    	
    	name = "QueueA";
    	ImaQueue qdest = new ImaQueue(name);
    	assertTrue(qdest != null);
    	assertEquals(qdest.getQueueName(), name);
    	assertEquals(qdest.getString("Name"),name);
    	assertEquals(""+qdest.get("Name"),name);  
    	
    }
    
    public void testIsmDestinationConstructor_withNullString() throws Exception {
    	
    	String dname = null;
    	@SuppressWarnings("unused")
        String name = null;
    	ImaTopic dest = new ImaTopic(dname);
    	
    	assertTrue(dest!=null);
    	try{
    		name = dest.getTopicName();
    		assertTrue(false);
    	}catch(JMSException jmse){
    		/*Expect the exception*/
    	}
    	
    	ImaQueue qdest = new ImaQueue(dname);
    	try{
    		name = qdest.getQueueName();
    		assertTrue(false);
    	}catch(JMSException jmse){
    		/*Expect the exception*/
    	}
    	assertEquals(qdest.getString("Name"),dname);
    	assertEquals(qdest.get("Name"),dname);  
    	
    		
    }

    
    /*
     * Test some properties
     */
    public void testgetCurrentProperties() throws Exception {
    	
    	String name = "TopicA";
    	ImaTopic dest = new ImaTopic(name);
    	
    	assertTrue(dest!=null);
    	assertEquals(name, dest.getTopicName());
    	assertEquals(name, dest.getString("Name"));
    	assertEquals(name, ""+dest.get("Name"));  
    	
    	name = "QueueA";
    	ImaQueue qdest = new ImaQueue(name);
    	
    	assertTrue(qdest!=null);
    	assertEquals(name, qdest.getQueueName());
    	assertEquals(name, qdest.getString("Name"));
    	assertEquals(name, ""+qdest.get("Name"));  
    	
    	ImaProperties props = qdest.getCurrentProperties();
    	assertEquals(name, props.getString("Name"));
    	
    	
    }

    public void testgetReference() throws Exception {
    	
    	String name = "TopicA";
    	ImaTopic dest = new ImaTopic(name);
    	
    	assertTrue(dest!=null);
    	assertEquals(name, dest.getTopicName());
    	assertEquals(name, dest.getString("Name"));
    	
    	Reference ref = dest.getReference();
    	
    	assertEquals(ref.size(), 1);  
    	assertEquals(ref.getClassName(),"com.ibm.ima.jms.impl.ImaTopic");
    	assertEquals(ref.getFactoryClassName(),"com.ibm.ima.jms.impl.ImaTopic");
    	
    	Enumeration<RefAddr> eRefAddr = ref.getAll();
    	
    	RefAddr refAddr = eRefAddr.nextElement();
    	assertEquals(refAddr.getType(), "Name");  
    	assertEquals(refAddr.getContent(),name);
    }

    public void testgetObjectInstance() throws Exception {
    	
    	String name = "QueueA";
    	ImaQueue dest = new ImaQueue(name);
    	
    	assertTrue(dest!=null);
    	assertEquals(dest.getQueueName(), name);
    	assertEquals(dest.getString("Name"),name);
    	assertEquals(""+dest.get("Name"),name);  
    	
    	Reference ref = dest.getReference();
    	
    	assertEquals(ref.size(),1);  
    	assertEquals(ref.getClassName(),"com.ibm.ima.jms.impl.ImaQueue");
    	assertEquals(ref.getFactoryClassName(),"com.ibm.ima.jms.impl.ImaQueue");
    	
    	Enumeration<RefAddr> eRefAddr = ref.getAll();
    	
    	RefAddr refAddr = eRefAddr.nextElement();
    	assertEquals(refAddr.getType(),"Name");  
    	assertEquals(refAddr.getContent(),name);
    	
    	ImaQueue dest2 = (ImaQueue) dest.getObjectInstance(ref, null, null, null);
    	assertTrue(dest2!=null);
    	assertEquals(dest2.getQueueName(), name);
    	assertEquals(dest2.getString("Name"),name);
    	assertEquals(""+dest2.get("Name"),name);  
    	
    	ImaProperties props = dest2.getCurrentProperties();
    	assertEquals(props.getString("Name"), name);
    }

    /*
     * Main test 
     */
    public static void main(String args[]) {
        try {
            new ImaDestinationTest().testIsmDestinationConstructor();
            new ImaDestinationTest().testIsmDestinationConstructor_withNullString();
            new ImaDestinationTest().testgetCurrentProperties();
            new ImaDestinationTest().testgetReference();
            new ImaDestinationTest().testgetObjectInstance();
           
        } catch (Exception e) {
            e.printStackTrace();
        } 
    }
}
