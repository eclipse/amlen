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
import java.util.HashMap;
import java.util.HashSet;

import javax.jms.Connection;
import javax.jms.MapMessage;
import javax.jms.MessageFormatException;
import javax.jms.Session;

import junit.framework.TestCase;


public class ImaMapMessageTest extends TestCase {
    /*
     * Test bytes message (common to all message types)
     */
    @SuppressWarnings("unchecked")
    public void testBytes() throws Exception {
    	Connection conn = new ImaConnection(true);
        ((ImaConnection)conn).client = ImaTestClient.createTestClient(conn);
        Session sess = conn.createSession(false, 0);
        
        byte [] b1 = new byte[30];
        for (int i=0; i<b1.length; i++) {
            b1[i] = (byte)i;
        }    
        
        MapMessage mmsg = sess.createMapMessage();
        mmsg.setObject("Boolean", true);
        mmsg.setObject("Byte", (byte)55);
        mmsg.setObject("Char", 'A');
        mmsg.setObject("Double", 3.1);
        mmsg.setObject("Float", -5.0f);
        mmsg.setObject("Int", 100);
        mmsg.setObject("Long", 123456789012L);
        mmsg.setObject("Short", (short)-55);
        mmsg.setObject("String", "This is a test");
        mmsg.setObject("Bytes1", b1);
        mmsg.setBytes("Bytes2", b1);
        mmsg.setBytes("Bytes3", b1, 1, 2);
        mmsg.setBoolean("Boolean2", false);
        mmsg.setByte("Byte2", (byte)2);
        mmsg.setChar("Char2", 'B');
        mmsg.setDouble("Double2", 0.0);
        mmsg.setFloat("Float2", 0.0f);
        mmsg.setInt("Int2", 10123456);
        mmsg.setInt("Int", 100);
        mmsg.setLong("Long2", 123456789013L);
        mmsg.setShort("Short2", (short)-56);
        mmsg.setString("String2", "This is another test");
        mmsg.setString("BoolStr", "false");
        mmsg.setString("NumStr", "33");
        mmsg.setString("FloatStr", "3.0");
        
        //System.out.println(ImaJmsObject.toString(mmsg, "*b"));
        
        Enumeration<?> en = mmsg.getMapNames();
        HashSet<String> set = new HashSet<String>();
        while (en.hasMoreElements()) {
            set.add((String)en.nextElement());
        }
        assertEquals(24, set.size());
        assertTrue(mmsg.itemExists("Long"));
        assertTrue(set.contains("Long"));
        assertTrue(mmsg.itemExists("NumStr"));
        assertTrue(set.contains("NumStr"));
        assertTrue(mmsg.itemExists("Float2"));
        assertTrue(set.contains("Float2"));
        assertTrue(mmsg.itemExists("Int"));
        assertTrue(set.contains("Int"));
        assertFalse(mmsg.itemExists("Int3"));
        assertFalse(set.contains("Int3"));
        
        assertEquals(b1, mmsg.getBytes("Bytes1"));
        assertEquals(b1, mmsg.getBytes("Bytes2"));
        assertEquals(2, mmsg.getBytes("Bytes3").length);
        
        assertEquals(true, mmsg.getBoolean("Boolean"));
        assertEquals("true", mmsg.getString("Boolean"));
        assertEquals(true, mmsg.getObject("Boolean"));
        assertEquals(false, mmsg.getObject("Boolean2"));
        
        assertEquals((byte)55, mmsg.getByte("Byte"));
        assertEquals("55",     mmsg.getString("Byte"));
        assertEquals(55,       mmsg.getInt("Byte"));
        assertEquals(55L,      mmsg.getLong("Byte"));
        assertEquals((byte)55, mmsg.getObject("Byte"));
        assertEquals((byte)2,  mmsg.getObject("Byte2"));
        
        assertEquals('A', mmsg.getChar("Char"));
        assertEquals('A', mmsg.getObject("Char"));
        assertEquals('B', mmsg.getChar("Char2"));
        
        assertEquals(3.1, mmsg.getDouble("Double"));
        assertEquals("3.1", mmsg.getString("Double"));
        assertEquals(3.1, mmsg.getObject("Double"));
        assertEquals(0.0, mmsg.getDouble("Double2"));
        
        assertEquals(-5.0f,  mmsg.getFloat("Float"));
        assertEquals(-5.0,   mmsg.getDouble("Float"));
        assertEquals("-5.0", mmsg.getString("Float"));
        assertEquals(-5.0f,  mmsg.getObject("Float"));
        assertEquals(0.0f,  mmsg.getFloat("Float2"));
        
        assertEquals(100,    mmsg.getInt("Int"));
        assertEquals("100",  mmsg.getString("Int"));
        assertEquals(100L,   mmsg.getLong("Int"));
        assertEquals(100,    mmsg.getObject("Int"));
        assertEquals(10123456,  mmsg.getInt("Int2"));
        
        assertEquals(123456789012L, mmsg.getLong("Long"));
        assertEquals(123456789012L, mmsg.getObject("Long"));
        assertEquals(123456789013L, mmsg.getLong("Long2"));
        
        assertEquals(33,         mmsg.getInt("NumStr"));
        assertEquals((byte)33,   mmsg.getByte("NumStr"));
        assertEquals((short)33,  mmsg.getShort("NumStr"));
        assertEquals(33L,        mmsg.getLong("NumStr"));
        assertEquals(false,      mmsg.getBoolean("BoolStr"));
        assertEquals(3.0f,       mmsg.getFloat("FloatStr"));
        assertEquals(3.0,        mmsg.getDouble("FloatStr"));
        
        /* Check unknown conversions */
        assertEquals(null,  mmsg.getObject("Unknown"));
        assertEquals(null,  mmsg.getString("Unknown"));
        assertEquals(null,  mmsg.getBytes("Unknown"));
        

        
        try {
            mmsg.getByte("Uknown");
            fail("get unknown as byte");
        } catch (Exception e) {
            assertTrue(e instanceof NumberFormatException);
        }
        try {
            mmsg.getBoolean("Uknown");
            fail("get unknown as boolean");
        } catch (Exception e) {
            assertTrue(e instanceof NumberFormatException);
        }
        try {
            mmsg.getDouble("Uknown");
            fail("get unknown as double");
        } catch (Exception e) {
            assertTrue(e instanceof NumberFormatException);
        }
        try {
            mmsg.getFloat("Uknown");
            fail("get unknown as float");
        } catch (Exception e) {
            assertTrue(e instanceof NumberFormatException);
        }
        try {
            mmsg.getInt("Uknown");
            fail("get unknown as int");
        } catch (Exception e) {
            assertTrue(e instanceof NumberFormatException);
        }    try {
            mmsg.getLong("Uknown");
            fail("get unknown as long");
        } catch (Exception e) {
            assertTrue(e instanceof NumberFormatException);
        }
       
        /* Test invalid objects */
        try {
            mmsg.setObject("HashMap", new HashMap());
            fail("set invalid object");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        /* Check invalid boolean conversions */
        try {
            mmsg.getByte("Boolean");
            fail("get boolean as byte");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
       
        try {
            mmsg.getChar("Boolean");
            fail("get boolean as char");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getShort("Boolean");
            fail("get boolean as short");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getInt("Boolean");
            fail("get boolean as int");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getLong("Boolean");
            fail("get boolean as long");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getFloat("Boolean");
            fail("get boolean as float");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getDouble("Boolean");
            fail("get boolean as double");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getBytes("Boolean");
            fail("get boolean as bytes");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        /* Check invalid byte conversions */
        try {
            mmsg.getByte("Long");
            fail("get long as byte");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
       
        try {
            mmsg.getChar("Long");
            fail("get long as char");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getShort("Long");
            fail("get long as short");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getInt("Long");
            fail("get long as char");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getBoolean("Long");
            fail("get long as long");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getFloat("Long");
            fail("get long as float");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getDouble("Long");
            fail("get long as double");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getBytes("Long");
            fail("get long as bytes");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }

        /* Check invalid int conversions */
        try {
            mmsg.getByte("Int");
            fail("get int as byte");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
       
        try {
            mmsg.getChar("Int");
            fail("get int as char");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getShort("Int");
            fail("get int as short");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getFloat("Int");
            fail("get int as float");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getDouble("Int");
            fail("get int as double");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getBoolean("Int");
            fail("get int as char");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getBytes("Int");
            fail("get int as bytes");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        

        /* Check invalid short conversions */
        try {
            mmsg.getByte("Short");
            fail("get short as byte");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getChar("Short");
            fail("get short as char");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getBoolean("Short");
            fail("get short as boolean");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getFloat("Short");
            fail("get short as float");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getDouble("Short");
            fail("get short as double");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getBytes("Short");
            fail("get short as bytes");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }


        /* Check invalid double conversions */
        try {
            mmsg.getByte("Double");
            fail("get double as byte");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getChar("Double");
            fail("get double as char");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getShort("Double");
            fail("get double as short");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getInt("Double");
            fail("get double as int");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getLong("Double");
            fail("get double as long");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getFloat("Double");
            fail("get double as float");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getBytes("Double");
            fail("get double as bytes");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        

        /* Check invalid double conversions */
        try {
            mmsg.getByte("Float");
            fail("get float as byte");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
       
        try {
            mmsg.getChar("Float");
            fail("get float as char");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getShort("Float");
            fail("get float as short");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getInt("Float");
            fail("get float as int");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getLong("Float");
            fail("get float as long");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        try {
            mmsg.getBytes("Float");
            fail("get float as bytes");
        } catch (Exception e) {
            assertTrue(e instanceof MessageFormatException);
        }
        
        conn.close();
    }   
    
   /*
    * Main test 
    */
   public static void main(String args[]) {
       try {
           new ImaBytesMessageTest().testBytes();
       } catch (Exception e) {
           e.printStackTrace();
       } 
   }
}
