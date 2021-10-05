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

import java.nio.ByteBuffer;
import java.text.DecimalFormat;

import javax.jms.BytesMessage;
import javax.jms.JMSException;
import javax.jms.Message;

/**
 * Implement the BytesMessage for the IBM MessageSight JMS client.
 */
public class ImaBytesMessage extends ImaMessage implements BytesMessage {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    /*
     * Constructor for an IMA bytes message 
     */
    public ImaBytesMessage(ImaSession session) {
        super(session, MTYPE_BytesMessage);
        body = ByteBuffer.allocate(256);
    }
    
    /*
     * Constructor for a cloned foreign message
     */
    public ImaBytesMessage(BytesMessage msg, ImaSession session) throws JMSException {
        super(session, MTYPE_BytesMessage, (Message)msg);
        msg.reset();
        int len = (int)msg.getBodyLength();

        /*
         * Copy the bytes of the message
         */
        byte[] ba = new byte[len];
        if (len > 0)
            msg.readBytes(ba);
        body = ByteBuffer.wrap(ba);
        body.position(len);
        msg.reset();
    }
    
    /*
     * Constructor for a cloned received message
     */
    public ImaBytesMessage(ImaMessage msg) {
        super(msg);
    }
    
    /* 
     * Clear the body.
     * This will allow the body to be written, but not read.
     * 
     * @see javax.jms.Message#clearBody()
     */
    public void clearBody() throws JMSException {
        super.clearBody();        
    }
    
    /* 
     * Read a boolean from the body.
     * 
     * @see javax.jms.BytesMessage#getBodyLength()
     */
    public long getBodyLength() throws JMSException {
        checkReadable();
        return body.limit();
    }

    /* 
     * Read a boolean from the message body.
     * 
     * @see javax.jms.BytesMessage#readBoolean()
     */
    public boolean readBoolean() throws JMSException {
        byte b = readByte();
        return b != 0;
    }

    /* 
     * Read a byte from the message body.
     * 
     * @see javax.jms.BytesMessage#readByte()
     */
    public byte readByte() throws JMSException {
        checkReadBuffer(1);
        return body.get();
    }

    /* 
     * Get bytes using the entire byte array.
     * 
     * @see javax.jms.BytesMessage#readBytes(byte[])
     */
    public int readBytes(byte[] value) throws JMSException {
        return readBytes(value, value.length);
    }

    /* 
     * Get bytes using a specified length of the byte array.
     * 
     * @see javax.jms.BytesMessage#readBytes(byte[], int)
     */
    public int readBytes(byte[] value, int len) throws JMSException {
        int avail = body.remaining();
        if (avail == 0) {
            checkReadable();
            return -1;
        }    
        if (avail >= len)  
            avail = len;
        checkReadBuffer(avail);
        body.get(value, 0, avail);
        return avail;
    }

    /* 
     * Read a character from the message body.
     * 
     * @see javax.jms.BytesMessage#readChar()
     */
    public char readChar() throws JMSException {
        return (char)(readShort()&0xffff);
    }

    /* 
     * Read a double from the message body.
     * @see javax.jms.BytesMessage#readDouble()
     */
    public double readDouble() throws JMSException {
        checkReadBuffer(8);
        return body.getDouble();
    }

    /* 
     * Read a float from the message body.
     * 
     * @see javax.jms.BytesMessage#readFloat()
     */
    public float readFloat() throws JMSException {
        checkReadBuffer(4);
        return body.getFloat();
    }

    /* 
     * Read an integer from the message body.
     * 
     * @see javax.jms.BytesMessage#readInt()
     */
    public int readInt() throws JMSException {
        checkReadBuffer(4);
        return body.getInt();
    }

    /* 
     * Read a long from the message body.
     * 
     * @see javax.jms.BytesMessage#readLong()
     */
    public long readLong() throws JMSException {
        checkReadBuffer(8);
        return body.getLong();
    }

    /* 
     * Read a short from the message body.
     * 
     * @see javax.jms.BytesMessage#readShort()
     */
    public short readShort() throws JMSException {
        checkReadBuffer(2);
        return body.getShort();
    }

    /* 
     * Read a modified UTF-8 string from the message body.
     * 
     * @see javax.jms.BytesMessage#readUTF()
     */
    public String readUTF() throws JMSException {
    	body.mark();
        int len = readShort()&0xffff;
        checkReadBuffer(len);
        try {
            return ImaUtils.fromUTF8(body, len, false);
        } catch (JMSException e) {
            body.reset();
            throw e;
        }
    }
  

    /* 
     * Read an unsigned byte from the message body.
     * 
     * @see javax.jms.BytesMessage#readUnsignedByte()
     */
    public int readUnsignedByte() throws JMSException {
        return (int)readByte()&0xff;
    }

    /* 
     * Read an unsigned short from the message body.
     * 
     * @see javax.jms.BytesMessage#readUnsignedShort()
     */
    public int readUnsignedShort() throws JMSException {
        return (int)readShort()&0xffff;
    }

    /* 
     * Reset the message body.
     * 
     * @see javax.jms.BytesMessage#reset()
     */
    public void reset() throws JMSException {
        if (!isReadonly)
            body.flip();
        else
        	body.position(0);
        isReadonly = true;
    }
    
    /*
     * Internal method called when content is initialized.
     * This is similar to reset, but only the body and bodylen are assumed to be set.
     * 
     * @see com.ibm.ima.jms.impl.ImaMessage#initContent()
     */
    public void initContent() throws JMSException {
        super.initContent();
        if (body != null)
        	body.position(0);
        isReadonly = true;
    }

    /* 
     * Write a boolean to the message body.
     * 
     * @see javax.jms.BytesMessage#writeBoolean(boolean)
     */
    public void writeBoolean(boolean value) throws JMSException {
        checkWriteBuffer(16);
        byte b = value ? (byte)1 : (byte)0;
        body.put(b);
    }

    /* 
     * Write a byte to the message body.
     * 
     * @see javax.jms.BytesMessage#writeByte(byte)
     */
    public void writeByte(byte value) throws JMSException {
       checkWriteBuffer(16);
       body.put(value);

    }

    /* 
     * Write an entire byte array to the message body.
     *  
     * @see javax.jms.BytesMessage#writeBytes(byte[])
     */
    public void writeBytes(byte[] value) throws JMSException {
        writeBytes(value, 0, value.length);
    }

    /* 
     * Write a portion of a byte array to the message body.
     * 
     * @see javax.jms.BytesMessage#writeBytes(byte[], int, int)
     */
    public void writeBytes(byte[] b, int pos, int len) throws JMSException {
        checkWriteBuffer(len);
        try {
        	body.put(b, pos, len);
        } catch (Exception e) {
        	ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0043", e, "A call to writeBytes() or writeUTF() on a BytesMessage failed due to an exception.");
    		ImaTrace.traceException(2, jex);
    		throw jex;
        }
    }

    /* 
     * Write a character to the message body.
     * 
     * @see javax.jms.BytesMessage#writeChar(char)
     */
    public void writeChar(char value) throws JMSException {
        writeShort((short)value);
    }

    /* 
     * Write a double to the message body.
     * 
     * @see javax.jms.BytesMessage#writeDouble(double)
     */
    public void writeDouble(double value) throws JMSException {
        checkWriteBuffer(16);
        body.putDouble(value);
    }

    /* 
     * Write a float to the message body.
     * 
     * @see javax.jms.BytesMessage#writeFloat(float)
     */
    public void writeFloat(float value) throws JMSException {
        checkWriteBuffer(16);
        body.putFloat(value);
    }

    /* 
     * Write an integer to the message body.
     * 
     * @see javax.jms.BytesMessage#writeInt(int)
     */
    public void writeInt(int value) throws JMSException {
        checkWriteBuffer(16);
        body.putInt(value);
    }

    /* 
     * Write a long to the message body.
     * 
     * @see javax.jms.BytesMessage#writeLong(long)
     */
    public void writeLong(long value) throws JMSException {
        checkWriteBuffer(16);
        body.putLong(value);
    }

    /* 
     * Write an object to the message body.
     * 
     * @see javax.jms.BytesMessage#writeObject(java.lang.Object)
     */
    public void writeObject(Object value) throws JMSException {
        if (value instanceof Integer)
            writeInt(((Integer)value).intValue());
        else if (value instanceof byte[])
            writeBytes((byte[])value);
        else if (value instanceof Long)
            writeLong(((Long)value).longValue());
        else if (value instanceof Short)
            writeShort(((Short)value).shortValue());
        else if (value instanceof Boolean)
            writeBoolean(((Boolean)value).booleanValue());
        else if (value instanceof Byte)
            writeByte(((Byte)value).byteValue());
        else if (value instanceof Character)
            writeChar(((Character)value).charValue());
        else if (value instanceof Double)
            writeDouble(((Double)value).doubleValue());
        else if (value instanceof Float)
            writeFloat(((Float)value).floatValue());
        else if (value instanceof String)
            writeUTF(((String)value));
        else {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0034", "A method call to set an object on a message failed because the input object type ({0}) is not supported.  The object types that are supported are Boolean, Byte, byte[], Character, Double, Float, Integer, Long, Short, and String.", value.getClass());
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }

    /* 
     * Write a short to the message body.
     * 
     * @see javax.jms.BytesMessage#writeShort(short)
     */
    public void writeShort(short value) throws JMSException {
        checkWriteBuffer(16);
        body.putShort(value);
    }

    /* 
     * Write a string using a modified UTF-8 encoding.
     * 
     * @see javax.jms.BytesMessage#writeUTF(java.lang.String)
     */
    public void writeUTF(String str) throws JMSException {
        int utflen = ImaUtils.sizeModifiedUTF8(str);
        if (utflen > 65535) {
        	ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0033", "A call writeUTF() on a BytesMessage failed because the input string is too long.  The maximum length is 65535 bytes.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        
        
        byte[] ba = tempByte.get();
        if (ba.length < utflen+2) {
        	ba = new byte[utflen+2];
        	tempByte.set(ba);
        }
        ba[0] = (byte) ((utflen >>> 8) & 0xFF);
        ba[1] = (byte) ((utflen >>> 0) & 0xFF);
        ImaUtils.makeModifiedUTF8(str, utflen, ba, 2);
        writeBytes(ba,0,utflen+2);
    }

    /*
     * Check that the write buffer is long enough
     */
    private void checkWriteBuffer(int len) throws JMSException {
        checkWriteable();
        body = ImaUtils.ensureBuffer(body,len);
    } 
    
    
    /*
     * Check that the read buffer is long enough
     */
    private void checkReadBuffer(int len) throws JMSException {
        checkReadable();      
        if ((body == null) || (body.remaining() < len)) {
            int remaining = (body == null) ? 0 : body.remaining();
        	ImaMessageEOFException jex =  new ImaMessageEOFException("CWLNC0073", "An attempt to read content from a BytesMessage failed because the end of the message was reached. Attempted to read {0} bytes but {1} bytes were left.", 
        	        len, remaining);
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }    
    
	static char [] hexdigit = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	
	/*
	 * Put out the characters
	 */
	int putchars(char [] line, int outpos, int inpos, int len) {
	    int       i;
	    line[outpos++] = ' ';
	    line[outpos++] = '[';
	    for (i=0; i<len; i++) {
	    	char ch = (char)(body.get(i+inpos)&0xff);
	        if (ch < 0x20 || ch >= 0x7f)
	            ch = '.';
	        line[outpos++] = ch;
	    }
	    line[outpos++] = ']';
	    line[outpos++] = '\n';
	    return outpos;
	}
	
	/*
	 * @see com.ibm.ima.jms.impl.ImaMessage#formatBody()
	 */
	public String formatBody() {
		return formatBody(1024, "     ");
	}
	
    /*
     * Format the body of the message and allow for maxsize and line offset to be specified
     */
    public String formatBody(int maxlen, String startline) {
     	char line [] = new char[128];
    	int  inpos = 0;
    	int  outpos = 0;
    	int  mod;
    	if (maxlen > body.limit())
    		maxlen = body.limit();
    	StringBuffer ret = new StringBuffer();
    	if (maxlen > body.limit())
    		maxlen = body.limit();
        if (maxlen < 32) {
            mod = 0;
            outpos = 0;
            for (inpos=0; inpos<maxlen; inpos++) {
                line[outpos++] = hexdigit[(body.get(inpos)>>4)&0x0f];
                line[outpos++] = hexdigit[body.get(inpos)&0x0f];
                if (mod++ == 3) {
                    line[outpos++] = ' ';
                    mod = 0;
                }
            }
            outpos = putchars(line, outpos, 0, maxlen);
            ret.append(line, 0, outpos);
        } else {
        	DecimalFormat nf = new DecimalFormat("00000': '");
            while (inpos < maxlen) {
            	outpos = 0;
            	if (inpos > 0)
            		ret.append(startline);
                ret.append(nf.format(inpos));
                mod = 0;
                for (int i=0; i<32; i++, inpos++) {
                    if (inpos >= maxlen) {
                        line[outpos++] = ' ';
                        line[outpos++] = ' ';
                    } else {
                        line[outpos++] = hexdigit[(body.get(inpos)>>4)&0x0f];
                        line[outpos++] = hexdigit[body.get(inpos)&0x0f];
                    }
                    if (mod++ == 3) {
                        line[outpos++] = ' '; 
                        mod = 0;
                    }
                }
                outpos = putchars(line, outpos, inpos-32, inpos<=maxlen ? 32 : maxlen-inpos+32);
                ret.append(line, 0, outpos);
            }
        }
    	return ret.toString();
    }
    
}
