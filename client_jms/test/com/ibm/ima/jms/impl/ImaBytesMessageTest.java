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

import javax.jms.BytesMessage;
import javax.jms.Connection;
import javax.jms.JMSException;
import javax.jms.MessageNotReadableException;
import javax.jms.MessageNotWriteableException;
import javax.jms.Session;

import com.ibm.ima.jms.ImaJmsObject;
import com.ibm.ima.jms.impl.ImaConnection;

import junit.framework.TestCase;

/*
 * Test the IMABytesMessage implementation
 *
 */
public class ImaBytesMessageTest extends TestCase {
	static boolean verbose = false;

    /*
     * 
     */      
    public ImaBytesMessageTest() {
        super("IMA JMS Client BytesMessage test");
    }
    
    /*
     * Test bytes message (common to all message types)
     */
    public void testBytes() throws Exception {
        Connection conn = new ImaConnection(true);
        ((ImaConnection)conn).client = ImaTestClient.createTestClient(conn);
        Session sess = conn.createSession(false, 0);
        
        BytesMessage bmsg = sess.createBytesMessage();
        byte [] b1 = new byte[600];
        for (int i=0; i<b1.length; i++) {
            b1[i] = (byte)i;
        }    
        
        /*
         * Do each of the writes 
         */
        bmsg.writeBoolean(true);
        bmsg.writeByte((byte)55);
        bmsg.writeChar('A');
        bmsg.writeDouble(3.1);
        bmsg.writeFloat(-5.0f);
        bmsg.writeInt(100);
        bmsg.writeLong(123456789012L);
        bmsg.writeShort((short)-55);
        bmsg.writeUTF("This is a test");
        bmsg.writeByte((byte)-1);
        bmsg.writeShort((short)-1);
        bmsg.writeObject(false);
        bmsg.writeObject((byte)56);
        bmsg.writeObject((char)'B');
        bmsg.writeObject((double)3.2);
        bmsg.writeObject((float)-6.0f);
        bmsg.writeObject(101);
        bmsg.writeObject(123456789013L);
        bmsg.writeObject((short)-56);
        bmsg.writeObject("This is a test2");
        bmsg.writeBytes(b1);
        bmsg.writeBytes(b1, 1, 2);
        bmsg.writeObject((double)3.3);
        
        try {
            bmsg.getBodyLength();
            fail("getBodyLength");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        
        if (verbose)
            System.out.println(ImaJmsObject.toString(bmsg, "*b"));
        
        /*
         * Reset and do all of the reads
         */
        bmsg.reset();
        assertEquals(bmsg.getBodyLength(), 706);
        
        assertEquals(true,          bmsg.readBoolean());
        assertEquals((byte)55,      bmsg.readByte());
        assertEquals('A',           bmsg.readChar());
        assertEquals(3.1,           bmsg.readDouble());
        assertEquals(-5.00f,        bmsg.readFloat());
        assertEquals(100,           bmsg.readInt());
        assertEquals(123456789012L, bmsg.readLong());
        assertEquals((short)(-55),  bmsg.readShort());
        assertEquals("This is a test", bmsg.readUTF());
        assertEquals(0xff,          bmsg.readUnsignedByte());
        assertEquals(0xffff,        bmsg.readUnsignedShort());
        assertEquals(false,         bmsg.readBoolean());
        assertEquals((byte)56,      bmsg.readByte());
        assertEquals('B',           bmsg.readChar());
        assertEquals(3.2,           bmsg.readDouble());
        assertEquals(-6.0f,         bmsg.readFloat());
        assertEquals(101,           bmsg.readInt());
        assertEquals(123456789013L, bmsg.readLong());
        assertEquals((short)(-56),  bmsg.readShort());
        assertEquals("This is a test2", bmsg.readUTF());
        assertEquals(b1.length,     bmsg.readBytes(b1));
        assertEquals(3,             b1[3]);
        byte [] b2 = new byte[2];
        assertEquals(2,             bmsg.readBytes(b2));
        assertEquals(1,             b2[0]);
        assertEquals(3.3,           bmsg.readDouble());
        assertEquals(-1,            bmsg.readBytes(new byte[0]));
        assertEquals(-1,            bmsg.readBytes(new byte[4]));
        
        
        /*
         * Reset and read everything as bytes
         */
        bmsg.reset();
        int bodylen = (int)bmsg.getBodyLength();
        assertEquals(bmsg.readBytes(b1, 4), 4);
        assertEquals(bmsg.readBytes(b1), b1.length);
        assertEquals(bmsg.readBytes(b1), bodylen-b1.length-4);
        assertEquals(bmsg.readBytes(b1), -1);
        
        /*
         * Clear the body and do some more writes, reset, and reads
         */
        bmsg.clearBody();
        bmsg.reset();
        assertEquals(-1, bmsg.readBytes(new byte[0]));
        
        bmsg.clearBody();
        try {
            bmsg.getBodyLength();
            fail("getBodyLength");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        bmsg.writeBoolean(true);
        bmsg.writeByte((byte)55);
        bmsg.writeChar('A');
        bmsg.writeBytes(b1);
        bmsg.writeDouble(3.1);
        bmsg.reset();
        assertEquals(bmsg.getBodyLength(), b1.length+12);
        
        assertEquals(true,         bmsg.readBoolean());
        assertEquals((byte)55,     bmsg.readByte());
        assertEquals('A',          bmsg.readChar());
        assertEquals(b1.length,   bmsg.readBytes(b1));
        assertEquals(3.1,          bmsg.readDouble(), 3.1);
        
        /* 
         * Test writeable errors
         */
        bmsg.reset();
        try {
            bmsg.writeByte((byte)33);
            fail("writeByte");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            bmsg.writeBoolean(true);
            fail("writeBoolean");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            bmsg.writeChar('C');
            fail("writeChar");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            bmsg.writeDouble(3.5);
            fail("writeDouble");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            bmsg.writeFloat(7.0f);
            fail("writeFloat");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            bmsg.writeInt(103);
            fail("writeInt");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            bmsg.writeLong(123L);
            fail("writeLong");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            bmsg.writeShort((short)1234);
            fail("writeShort");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            bmsg.writeUTF("and again...");
            fail("writeUTF");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            bmsg.writeObject(Integer.valueOf(106));
            fail("writeObject");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            bmsg.writeObject(null);
            fail("writeObject null");
        } catch (Exception e) {
            assertTrue(e instanceof NullPointerException);
        }
        bmsg.clearBody();
        
        /*
         * Test readable errors
         */
        try {
            bmsg.readByte();
            fail("readByte");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            bmsg.readBoolean();
            fail("readBoolean");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            bmsg.readBytes(b1);
            fail("readBytes");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            bmsg.readChar();
            fail("readChar");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            bmsg.readDouble();
            fail("readDouble");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            bmsg.readInt();
            fail("readInt");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            bmsg.readLong();
            fail("readLong");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            bmsg.readFloat();
            fail("readFloat");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            bmsg.readShort();
            fail("readShort");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            bmsg.readUTF();
            fail("readUTF");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        
        conn.close();
    }    

    /*
     * Main test 
     */
    public void main(String args[]) {
        try {
            new ImaBytesMessageTest().testBytes();
        } catch (Exception e) {
            e.printStackTrace();
        } 
    }

}
