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
import java.util.HashSet;

import javax.jms.BytesMessage;
import javax.jms.Connection;
import javax.jms.Message;
import javax.jms.MessageFormatException;
import javax.jms.Session;
import javax.jms.StreamMessage;

import com.ibm.ima.jms.ImaJmsObject;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaMessage;
import com.ibm.ima.jms.impl.ImaMessageFormatException;

import junit.framework.TestCase;

/**
 * JUnit test for messages
 *
 */
public class ImaMessageTest extends TestCase {

    /**
     * @param name
     */
    public ImaMessageTest() {
        super("Test the IBM Messaging Appliance JMS client message support");
    }

    /* 
     * @see junit.framework.TestCase#setUp()
     */
    protected void setUp() throws Exception {
        super.setUp();
    }

    /* 
     * @see junit.framework.TestCase#tearDown()
     */
    protected void tearDown() throws Exception {
        super.tearDown();
    }
    
    /*
     * Tes properties (common to all message types)
     */
    public void testProperties() throws Exception {
    	Connection conn = new ImaConnection(true);
        ((ImaConnection)conn).client = ImaTestClient.createTestClient(conn);
        Session sess = conn.createSession(false, 0);
        
        BytesMessage bmsg = sess.createBytesMessage();
        
        assertEquals(false, bmsg.getJMSRedelivered());
        bmsg.setJMSDeliveryMode(ImaMessage.DEFAULT_DELIVERY_MODE);
        assertEquals(ImaMessage.DEFAULT_DELIVERY_MODE, bmsg.getJMSDeliveryMode());
        assertEquals(ImaMessage.DEFAULT_PRIORITY, bmsg.getJMSPriority());
        assertEquals(ImaMessage.DEFAULT_TIME_TO_LIVE, bmsg.getJMSExpiration());
        
        bmsg.setStringProperty("fred", "sam");
        bmsg.setBooleanProperty("bool", true);
        assertEquals(false, bmsg.propertyExists("sam"));
        bmsg.setIntProperty("sam", 3);
        assertEquals(true, bmsg.propertyExists("sam"));
        bmsg.setObjectProperty("sam", 3);
        bmsg.setByteProperty("bytep", (byte)-3);
        bmsg.setStringProperty("numval", "314");
        bmsg.setDoubleProperty("tim", 3.14);
        bmsg.setLongProperty("longp", 1234567890123L);
        bmsg.setFloatProperty("kurt", 3.0f);
        bmsg.setShortProperty("shrt", (short)55);
        
        assertTrue(ImaJmsObject.toString(bmsg,"*").indexOf("JMS_IBM_Retain") <= 0); 
        bmsg.setIntProperty("JMS_IBM_Retain", 1);
        assertEquals(1, bmsg.getIntProperty("JMS_IBM_Retain"));
        assertEquals(1L, bmsg.getLongProperty("JMS_IBM_Retain"));
        assertEquals("1", bmsg.getStringProperty("JMS_IBM_Retain"));
        assertEquals(true, bmsg.propertyExists("JMS_IBM_Retain"));
        assertEquals(false, bmsg.propertyExists("JMS_IBM_ACK_SQN"));
        assertEquals(false, bmsg.propertyExists(null));
        System.out.println(ImaJmsObject.toString(bmsg,"*"));
        assertTrue(ImaJmsObject.toString(bmsg,"*").indexOf("JMS_IBM_Retain") >= 0); 
        
        
        bmsg.setBooleanProperty("JMS_IBM_Retain", true);
        assertEquals(0, bmsg.getIntProperty("JMS_IBM_Retain"));
        
        Enumeration<?> en = bmsg.getPropertyNames();
        HashSet<String> set = new HashSet<String>();
        while (en.hasMoreElements()) {
            set.add((String) en.nextElement());
        }
        assertEquals(11, set.size());
        assertTrue(set.contains("sam"));
        assertTrue(set.contains("numval"));
        assertTrue(set.contains("shrt"));
        assertFalse(set.contains("kur"));
        
        /* Check float conversions */
        assertTrue(bmsg.getObjectProperty("kurt") instanceof Float);
        assertEquals(3.0f,   bmsg.getFloatProperty("kurt"));
        assertEquals(3.0,    bmsg.getDoubleProperty("kurt"));
        assertEquals("3.0",  bmsg.getStringProperty("kurt"));
        
        /* Check double conversions */
        assertTrue(bmsg.getObjectProperty("tim") instanceof Double);
        assertEquals(3.14, bmsg.getDoubleProperty("tim"));
        assertEquals("3.14", bmsg.getStringProperty("tim"));
        
        /* Check int conversions */
        assertTrue(bmsg.getObjectProperty("sam") instanceof Integer);
        assertEquals("3", bmsg.getStringProperty("sam"));
        assertEquals(3, bmsg.getIntProperty("sam"));
        assertEquals(3, bmsg.getLongProperty("sam"));
        
        /* Check string property conversions */
        assertTrue(bmsg.getObjectProperty("numval") instanceof String);
        assertEquals(314, bmsg.getIntProperty("numval"));
        assertEquals(314L, bmsg.getLongProperty("numval"));
        assertEquals("314", bmsg.getStringProperty("numval"));
        
        /* Check byte property conversions */
        assertTrue(bmsg.getObjectProperty("bytep") instanceof Byte);
        assertEquals(-3,   bmsg.getByteProperty("bytep"));
        assertEquals(-3,   bmsg.getIntProperty("bytep"));
        assertEquals(-3,   bmsg.getLongProperty("bytep"));
        assertEquals("-3", bmsg.getStringProperty("bytep"));
        
        /* Check short property conversions */
        assertTrue(bmsg.getObjectProperty("shrt") instanceof Short);
        assertEquals(55,    bmsg.getIntProperty("shrt"));
        assertEquals(55,    bmsg.getLongProperty("shrt"));
        assertEquals("55",  bmsg.getStringProperty("shrt"));
        
        /* Check long property conversions */
        assertTrue(bmsg.getObjectProperty("longp") instanceof Long);
        assertEquals(1234567890123L,  bmsg.getLongProperty("longp"));
        assertEquals("1234567890123", bmsg.getStringProperty("longp"));
        
        /* Check boolean property conversions */
        assertTrue(bmsg.getObjectProperty("bool") instanceof Boolean);
        assertEquals("true", bmsg.getStringProperty("bool"));
        
        bmsg.setIntProperty("JMS_IBM_Retain", 1);
        
        assertEquals(1, bmsg.getIntProperty("JMS_IBM_Retain"));
        assertEquals(1L, bmsg.getLongProperty("JMS_IBM_Retain"));
        assertEquals("1", bmsg.getStringProperty("JMS_IBM_Retain"));
        
        bmsg.setByteProperty("JMS_IBM_Retain", (byte)1);
        assertEquals(1, bmsg.getIntProperty("JMS_IBM_Retain"));
        
        bmsg.setShortProperty("JMS_IBM_Retain", (short)1);
        assertEquals(1, bmsg.getIntProperty("JMS_IBM_Retain"));
        
        bmsg.setDoubleProperty("JMS_IBM_Retain", 1.0);
        assertEquals(0, bmsg.getIntProperty("JMS_IBM_Retain"));
        
        try {
        	bmsg.setObjectProperty("connect", conn);
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        /* Check invalid boolean conversions */
        try {
            bmsg.getByteProperty("bool");
            fail("get boolean as byte");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        try {
            bmsg.getIntProperty("bool");
            fail("get boolean as int");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        try {
            bmsg.getLongProperty("bool");
            fail("get boolean as long");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        try {
            bmsg.getFloatProperty("bool");
            fail("get boolean as float");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        try {
            bmsg.getDoubleProperty("bool");
            fail("get boolean as double");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        
        /* Test invalid byte conversions */
        try {
            bmsg.getBooleanProperty("bytep");
            fail("get byte as boolean");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            bmsg.getFloatProperty("bytep");
            fail("get byte as float");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        try {
            bmsg.getDoubleProperty("bytep");
            fail("get byte as double");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        /* Test invalid int conversions */
        try {
            bmsg.getBooleanProperty("sam");
            fail("get int as boolean");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            bmsg.getFloatProperty("sam");
            fail("get int as float");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            bmsg.getDoubleProperty("sam");
            fail("get int as double");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            bmsg.getByteProperty("sam");
            fail("get int as byte");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            bmsg.getShortProperty("sam");
            fail("get int as short");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        /* Test invalid long conversions */
        try {
            bmsg.getBooleanProperty("longp");
            fail("get long as boolean");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            bmsg.getFloatProperty("longp");
            fail("get long as float");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            bmsg.getDoubleProperty("longp");
            fail("get long as double");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            bmsg.getByteProperty("longp");
            fail("get long as byte");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            bmsg.getShortProperty("longp");
            fail("get long as short");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        /* Test invalid short conversions */
        try {
            bmsg.getBooleanProperty("shrt");
            fail("get short as boolean");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            bmsg.getFloatProperty("shrt");
            fail("get short as float");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            bmsg.getDoubleProperty("shrt");
            fail("get short as double");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            bmsg.getByteProperty("shrt");
            fail("get short as byte");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        try {
        	bmsg.setIntProperty("\ud840\udc0b", 1);
        	assertEquals(1, bmsg.getIntProperty("\ud840\udc0b"));
        } catch (Exception e) {
        	assertTrue(false);
        }
        try {
        	bmsg.setIntProperty("\ud840\udc00\ud840\udc01\ud840\udc02", 1);
        	assertEquals(1, bmsg.getIntProperty("\ud840\udc00\ud840\udc01\ud840\udc02"));
        } catch (Exception e) {
        	assertTrue(false);
        }
        try {
        	bmsg.setIntProperty("\ud869\udeb2", 1);
        	assertEquals(1, bmsg.getIntProperty("\ud869\udeb2"));
        } catch (Exception e) {
        	assertTrue(false);
        }
        try {
        	bmsg.setIntProperty("\uD868\uDD90", 1);
        	assertEquals(1, bmsg.getIntProperty("\uD868\uDD90"));
        } catch (Exception e) {
        	System.err.println("isJavaIndentifier 2a190 = " + Character.isJavaIdentifierPart(0x2a190));
        	e.printStackTrace(System.err);
        	assertTrue(false);
        }
        // TODO: Change this if/when we start to use 1.7 for production builds
        // These tests only work for Java 1.7 so don't run them with 1.6
        if (!(System.getProperty("java.version")).startsWith("1.6")) {
            try {
            	bmsg.setIntProperty("\u0979", 1);
            	assertEquals(1, bmsg.getIntProperty("\u0979"));
            } catch (Exception e) {
            	System.err.println("isJavaIndentifier 0979 = " + Character.isJavaIdentifierPart(0x0979));
            	e.printStackTrace(System.err);
            	assertTrue(false);
            }
            try {
            	bmsg.setIntProperty("\uD869\uDF00", 1);
            	assertEquals(1, bmsg.getIntProperty("\uD869\uDF00"));
            } catch (Exception e) {
            	System.err.println("isJavaIndentifier 2A700 = " + Character.isJavaIdentifierPart(0x2a700));
            	e.printStackTrace(System.err);
            	assertTrue(false);
            }
            try {
            	bmsg.setIntProperty("\uD86D\uDF40\uD86e\udc1d\ud86d\udf34", 1);
            	assertEquals(1, bmsg.getIntProperty("\uD86D\uDF40\uD86e\udc1d\ud86d\udf34"));
            } catch (Exception e) {
            	System.err.println("isJavaIndentifier 2B740 = " + Character.isJavaIdentifierPart(0x2B740));
            	e.printStackTrace(System.err);
            	assertTrue(false);
            }
        }

        try {
        	bmsg.setIntProperty("\ud840\udc00", 1);
        	assertEquals(1, bmsg.getIntProperty("\ud840\udc00"));
        } catch (Exception e) {
        	fail("Non-baseplane character in property name");
        }
        try {
        	bmsg.setIntProperty("\udc00\ud840", 1);
        	assertEquals(1, bmsg.getIntProperty("\udc00\ud840"));
            fail("Invalid surrogate in property name");
        } catch (Exception e) {
        	assertTrue(e instanceof ImaMessageFormatException);
        }
        
        try {
            byte [] cid = new byte[2];
            bmsg.setJMSCorrelationIDAsBytes(cid);
            fail("set correlationID as bytes");
        } catch (Exception e) {
            assertTrue(e instanceof UnsupportedOperationException);
        }
        try {
            @SuppressWarnings("unused")
            byte [] cid = bmsg.getJMSCorrelationIDAsBytes();
            fail("get correlationID as bytes");
        } catch (Exception e) {
            assertTrue(e instanceof UnsupportedOperationException);
        }

        /*
         * Stream message
         */
        StreamMessage smsg = sess.createStreamMessage();
        smsg.setIntProperty("JMS_IBM_Retain", 0);
        
        assertEquals(0, smsg.getIntProperty("JMS_IBM_Retain"));
        assertEquals(0L, smsg.getLongProperty("JMS_IBM_Retain"));
        assertEquals("0", smsg.getStringProperty("JMS_IBM_Retain"));
        try {
            smsg.setStringProperty("abc def", "3");
            fail("Property 'abc def' is not a valid name.");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        
        Message msg = sess.createMessage();
        assertTrue(msg != null);
        
        conn.close();
    }
   
    
    /*
     * Main test 
     */
    public static void main(String args[]) {
        try {
            new ImaMessageTest().testProperties();
        } catch (Exception e) {
            e.printStackTrace();
        } 
    }

}
