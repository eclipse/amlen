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
import javax.jms.JMSException;
import javax.jms.MessageNotReadableException;
import javax.jms.MessageNotWriteableException;
import javax.jms.Session;
import javax.jms.StreamMessage;

import com.ibm.ima.jms.impl.ImaConnection;

import junit.framework.TestCase;


public class ImaStreamMessageTest extends TestCase {

    /**
     * IBM Messaging Appliance JMS client stream message text
     */
    public ImaStreamMessageTest() {
        super("IBM Messaging Appliance JMS client stream message test");
    }

    /*
     * Test bytes message (common to all message types)
     */
    public void testStream() throws Exception {
        Connection conn = new ImaConnection(true);
        ((ImaConnection)conn).client = ImaTestClient.createTestClient(conn);
        Session sess = conn.createSession(false, 0);
        
        StreamMessage smsg = sess.createStreamMessage();
        byte [] b1 = new byte[600];
        for (int i=0; i<b1.length; i++) {
            b1[i] = (byte)i;
        }    
        
        /*
         * Do each of the writes 
         */
        smsg.writeBoolean(true);
        smsg.writeByte((byte)55);
        smsg.writeChar('A');
        smsg.writeDouble(3.1);
        smsg.writeFloat(-5.0f);
        smsg.writeInt(100);
        smsg.writeLong(123456789012L);
        smsg.writeShort((short)-55);
        smsg.writeString("This is a test");
        smsg.writeByte((byte)-1);
        smsg.writeShort((short)-1);
        smsg.writeBytes(b1, 1, 2);
        smsg.writeObject(false);
        smsg.writeObject((byte)56);
        smsg.writeObject((char)'B');
        smsg.writeObject((double)3.2);
        smsg.writeObject((float)-6.0f);
        smsg.writeObject(101);
        smsg.writeBytes(b1, 3, 4);
        smsg.writeObject(123456789013L);
        smsg.writeObject((short)-56);
        smsg.writeObject("This is a test2");
        smsg.writeBytes(b1);
        smsg.writeObject((double)3.3);
        smsg.reset();
        
        assertEquals(true,          smsg.readBoolean());
        assertEquals((byte)55,      smsg.readByte());
        assertEquals('A',           smsg.readChar());
        assertEquals(3.1,           smsg.readDouble());
        assertEquals(-5.00f,        smsg.readFloat());
        assertEquals(100,           smsg.readInt());
        assertEquals(123456789012L, smsg.readLong());
        assertEquals((short)(-55),  smsg.readShort());
        assertEquals("This is a test", smsg.readString());
        assertEquals(0xff,          smsg.readByte()&0xff);
        assertEquals(0xffff,        smsg.readShort()&0xffff);
        assertEquals(2,             smsg.readBytes(new byte[5]));
        assertEquals(false,         smsg.readBoolean());
        assertEquals((byte)56,      smsg.readByte());
        assertEquals('B',           smsg.readChar());
        assertEquals(3.2,           smsg.readDouble());
        assertEquals(-6.0f,         smsg.readFloat());
        assertEquals(101,           smsg.readInt());
        assertEquals(4,             ((byte [])smsg.readObject()).length);
        assertEquals(123456789013L, smsg.readLong());
        assertEquals((short)(-56),  smsg.readShort());
        assertEquals("This is a test2", smsg.readString());
        assertEquals(b1.length,     smsg.readBytes(b1));
        assertEquals(0,             smsg.readBytes(new byte[0]));
        assertEquals(-1,            smsg.readBytes(new byte[3]));
        assertEquals(3.3,           smsg.readDouble());
        
        /*
         * Write enough values to ensure an extension
         */
        smsg.clearBody();
        for (int i=0; i<500; i++) {
            smsg.writeShort((short)(256+i));
        }
        
        /*
         * Clear the body and do some more writes, reset, and reads
         */
        smsg.clearBody();
        smsg.writeBoolean(true);
        smsg.writeByte((byte)55);
        smsg.writeChar('A');
        smsg.writeBytes(b1);
        smsg.writeDouble(3.1);
        smsg.writeBytes(new byte[0]);
        smsg.writeBytes(null);
        smsg.reset();
        
        assertEquals(true,         smsg.readBoolean());
        assertEquals((byte)55,     smsg.readByte());
        assertEquals('A',          smsg.readChar());
        assertEquals(b1.length,    smsg.readBytes(b1));
        assertEquals(-1,           smsg.readBytes(new byte[1]));
        assertEquals(3.1,          smsg.readDouble(), 3.1);
        assertEquals(0,            smsg.readBytes(new byte[5]));
        assertEquals(-1,           smsg.readBytes(new byte[90]));
        
        /* 
         * Test writeable errors
         */
        smsg.reset();
        try {
            smsg.writeByte((byte)33);
            fail("writeByte");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            smsg.writeBoolean(true);
            fail("writeBoolean");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            smsg.writeChar('C');
            fail("writeChar");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            smsg.writeDouble(3.5);
            fail("writeDouble");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            smsg.writeFloat(7.0f);
            fail("writeFloat");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            smsg.writeInt(103);
            fail("writeInt");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            smsg.writeLong(123L);
            fail("writeLong");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            smsg.writeShort((short)1234);
            fail("writeShort");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            smsg.writeString("and again...");
            fail("writeString");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            smsg.writeObject(Integer.valueOf(106));
            fail("writeObject");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            smsg.writeObject(null);
            fail("writeObject null");
        } catch (Exception e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        smsg.clearBody();
        
        /*
         * Test readable errors
         */
        try {
            smsg.readByte();
            fail("readByte");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            smsg.readBoolean();
            fail("readBoolean");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            smsg.readBytes(b1);
            fail("readBytes");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            smsg.readChar();
            fail("readChar");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            smsg.readDouble();
            fail("readDouble");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            smsg.readInt();
            fail("readInt");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            smsg.readLong();
            fail("readLong");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            smsg.readFloat();
            fail("readFloat");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            smsg.readShort();
            fail("readShort");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            smsg.readString();
            fail("readString");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        
        conn.close();
    }    
    

    /*
     * Main test 
     */
    public static void main(String args[]) {
        try {
            new ImaStreamMessageTest().testStream();
        } catch (Exception e) {
            e.printStackTrace();
        } 
    }
}
