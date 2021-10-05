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
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageEOFException;
import javax.jms.StreamMessage;

/**
 * Implement the StreamMessage interface for the IBM MessageSight JMS client.
 *
 * This class uses the stream methods provided in the ImaMessage class to process a stream message.
 * These encode the stream into a slightly compressed format which uses very small object introduces and
 * skips the leading zero bytes of integers.
 */
public class ImaStreamMessage extends ImaMessage implements StreamMessage {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private ByteBuffer savebytes = null;        /* Save the byte array over multiple readBytes                 */

    /*
     * Constructor for a StreamMessage.
     */
    public ImaStreamMessage(ImaSession session) {
        super(session, MTYPE_StreamMessage);
        isReadonly  = false;
        if (body == null)
            body = ByteBuffer.allocate(512);
        body.put((byte)0x9E);
    }

    
    /*
     * Constructor for a StreamMessage from another StreamMessage.
     */
    public ImaStreamMessage(StreamMessage msg, ImaSession session) throws JMSException {
        super(session, MTYPE_StreamMessage, (Message)msg);
        isReadonly = false;
        checkWriteable();
        body.put((byte)0x9E);
        msg.reset();
        try {
            for (;;) {
                writeObject(msg.readObject());
            }
        } catch (MessageEOFException e) {}
        msg.reset();
    }
    
    
    /*
     * Constructor for a cloned received message
     */
    public ImaStreamMessage(ImaMessage msg) {
        super(msg);
        body.position(1);
    }

    
    /*
     * Reset the message.
     * @see javax.jms.StreamMessage#reset()
     */
    public void reset() throws JMSException {
        if (!isReadonly) {
            body.flip();
        } else {
        	body.position(0);
        }
        if (body.limit()<1 || body.get() != (byte)0x9E) {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0210", "A call to reset() or to initContent() on a StreamMessage failed because the message does not contain stream data. This can occur if a message received from MessageSight is incorrectly cast to StreamMessage.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        isReadonly = true;
    }
    
    /*
     * Initialize the content. 
     * This checks to see if the content is in fact stream data.
     * @see com.ibm.ima.jms.impl.ImaMessage#initContent()
     */
    void initContent() throws JMSException {
        super.initContent();
        if (body == null || body.limit()<1 || body.get() != (byte)0x9E) {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0210", "A call to reset() or to initContent() on a StreamMessage failed because the message does not contain stream data. This can occur if a message received from MessageSight is incorrectly cast to StreamMessage.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        isReadonly = true;
    }

    
    /*
     * Clear the body.
     * @see javax.jms.Message#clearBody()
     */
    public void clearBody() throws JMSException {
        super.clearBody();
        checkWriteable();
        body.put((byte)0x9E);
    }


    /*
     * Read a boolean value.
     *
     * @see javax.jms.StreamMessage#readBoolean()
     */
    public boolean readBoolean() throws JMSException {
    	body.mark();
        int otype = getObjectType();
        S_Type type = FieldTypes[otype];
        switch (type) {
        case Boolean:
            return Boolean.valueOf((otype&1)==1);
        case String:
        case StrLen:
        case Null:
        	body.mark();
            try {
                 return Boolean.valueOf(ImaUtils.getStringValue(body,otype));
            } catch (RuntimeException re) {
                body.reset();
                throw re;
            }
            catch (JMSException jex) {
                body.reset();
                throw jex;
            }
        default:
            body.reset();
            ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0061", "A method call to retrieve a Boolean from a Message object failed because the value is not Boolean and cannot be successfully converted to Boolean.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }


    /*
     * Read a byte value.
     *
     * @see javax.jms.StreamMessage#readByte()
     */
    public byte readByte() throws JMSException {
    	body.mark();
        int otype = getObjectType();
        S_Type type = FieldTypes[otype];
        switch (type) {
        case Byte:
        case UByte:
            return ImaUtils.getByteValue(body,otype);
        case String:
        case StrLen:
        case Null:
            try {
                 return Byte.valueOf(ImaUtils.getStringValue(body,otype));
            } catch (RuntimeException re) {
                body.reset();
                throw re;
            }
            catch (JMSException jex) {
                body.reset();
                throw jex;
            }
        default:
            body.reset();
            ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0062", "A method call to retrieve a Byte from a Message object failed because the value is not of type Byte and cannot be successfully converted to Byte.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }


    /*
     * Override the method in ImaMessage to also check if we are within a bytes read
     */
    int getObjectType() throws JMSException {
        checkReadable();
        if (savebytes != null) {
        	// TODO: Need new error code for this.
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0080", "A read operation on a StreamMessage object failed because there were bytes remaining to be read after a previous call to readBytes().");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        if (body.remaining() == 0) {
        	ImaMessageEOFException jex = new ImaMessageEOFException("CWLNC0060", "A failure occurred when reading data received from MessageSight because the end of the message content was reached. This typically means that all of the data was read and no further reading is required.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        return body.get()&0xff;
    }


    /*
     * Read a byte array.
     *
     * @see javax.jms.StreamMessage#readBytes(byte[])
     */
    public int readBytes(byte[] value) throws JMSException {
    	checkReadable();
        body.mark();
        if (value == null) {
        	ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0076", "A call to readBytes() on a StreamMessage failed because the byte array argument is null.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        ByteBuffer bb;
        if (savebytes != null) {
            bb = savebytes;
            savebytes = null;
        } else {
            int otype = ImaUtils.getObjectType(body);
            S_Type type = FieldTypes[otype];
            switch (type) {
            case BArray:
                bb = ImaUtils.getByteBufferValue(body,otype);
                break;
            case User:
                Object obj = ImaUtils.getUserValue(body,otype);
                if (!(obj instanceof ByteBuffer)) {
                    body.reset();
                    ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0070", "A call to getBytes() or readBytes() on a Message object failed because the value is not of type byte array (byte[]) and cannot be successfully converted to a byte array.");
                    ImaTrace.traceException(2, jex);
                    throw jex;
                }    
                bb = (ByteBuffer)obj;
                break;
            case Null:
                return -1;
            default:
                body.reset();
                ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0070", "A call to getBytes() or readBytes() on a Message object failed because the value is not of type byte array (byte[]) and cannot be successfully converted to a byte array.");
                ImaTrace.traceException(2, jex);
                throw jex;
            }
        }

        int retlen;
        if (value.length > (bb.remaining())) {
            retlen = bb.remaining();
            if (retlen == 0) {
                if (bb.limit() > 0)
                    retlen = -1;
            } else {
                bb.get(value,0,retlen);
            }
        } else {
            bb.get(value);
            savebytes = bb;
            retlen = value.length;
        }
        return retlen;
    }


    /*
     * Read a character.
     *
     * @see javax.jms.StreamMessage#readChar()
     */
    public char readChar() throws JMSException {
    	body.mark();
        int otype = getObjectType();
        S_Type type = FieldTypes[otype];
        switch (type) {
        case Char:
        case UShort:
            return ImaUtils.getCharValue(body,otype);
        case Null:
        	ImaNullPointerException nex = new ImaNullPointerException("CWLNC0081", "A call to getChar() on a MapMessage or to readChar() on a StreamMessage failed because the map element {0} contains a null object or the stream object is null.","");
            ImaTrace.traceException(2, nex);
            throw nex;
        default:
            body.reset();
            ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0069", "A call to getChar() or readChar() on a Message object failed because the value is not of type Character and cannot be successfully converted to Character.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }


    /*
     * Read a double.
     *
     * @see javax.jms.StreamMessage#readDouble()
     */
    public double readDouble() throws JMSException {
    	body.mark();
        int otype = getObjectType();
        S_Type type = FieldTypes[otype];
        switch (type) {
        case Double:
            return ImaUtils.getDoubleValue(body,otype);
        case String:
        case StrLen:
        case Null:
        	body.mark();
            try {
                 return Double.valueOf(ImaUtils.getStringValue(body,otype));
            } catch (RuntimeException re) {
            	body.reset();
                throw re;
            }
            catch (JMSException jex) {
                body.reset();
                throw jex;
            }
        case Float:
            return (double)ImaUtils.getFloatValue(body,otype);
        default:
            body.reset();
            ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0063", "A method call to retrieve a Double from a Message object failed because the value is not of type Double and cannot be successfully converted to Double.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }


    /*
     * Read a float.
     *
     * @see javax.jms.StreamMessage#readFloat()
     */
    public float readFloat() throws JMSException {
    	body.mark();
        int otype = getObjectType();
        S_Type type = FieldTypes[otype];
        switch (type) {
        case Float:
            return ImaUtils.getFloatValue(body,otype);
        case String:
        case StrLen:
        case Null:
        	body.mark();
            try {
                 return Float.valueOf(ImaUtils.getStringValue(body,otype));
            } catch (RuntimeException re) {
            	body.reset();
                throw re;
            }
            catch (JMSException jex) {
                body.reset();
                throw jex;
            }
        default:
            body.reset();
            ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0065", "A method call to retrieve a Float from a Message object failed because the value is not of type Float and cannot be successfully converted to Float.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }


    /*
     * Read an int.
     *
     * @see javax.jms.StreamMessage#readInt()
     */
    public int readInt() throws JMSException {
    	body.mark();
        int otype = getObjectType();
        S_Type type = FieldTypes[otype];
        switch (type) {
        case Int:
            return ImaUtils.getIntValue(body,otype);
        case String:
        case StrLen:
        case Null:
        	body.mark();
            try {
                 return Integer.valueOf(ImaUtils.getStringValue(body,otype));
            } catch (RuntimeException re) {
                body.reset();
                throw re;
            }
            catch (JMSException jex) {
                body.reset();
                throw jex;
            }
        case Byte:
            return (int)ImaUtils.getByteValue(body,otype);
        case UByte:
            return (int)ImaUtils.getByteValue(body,otype)&0xff;
        case Short:
            return (int)ImaUtils.getShortValue(body,otype);
        case UShort:
            return (int)ImaUtils.getShortValue(body,otype)&0xffff;
        default:
            body.reset();
            ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0064", "A method call to retrieve an Integer from a Message object failed because the value is not of type Integer and cannot be successfully converted to Integer.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }


    /*
     * Read a long.
     *
     * @see javax.jms.StreamMessage#readLong()
     */
    public long readLong() throws JMSException {
    	body.mark();
        int otype = getObjectType();
        S_Type type = FieldTypes[otype];
        switch (type) {
        case Long:
        case ULong:
            return ImaUtils.getLongValue(body,otype);
        case Time:
            return otype==S_Time ? 0 : ImaUtils.getLongValue(body,8);
        case Int:
            return ImaUtils.getIntValue(body,otype);
        case String:
        case StrLen:
        case Null:
        	body.mark();
            try {
                 return Long.valueOf(ImaUtils.getStringValue(body,otype));
            } catch (RuntimeException re) {
                body.reset();
                throw re;
            }
            catch (JMSException jex) {
                body.reset();
                throw jex;
            }
        case Byte:
            return (long)ImaUtils.getByteValue(body,otype);
        case UByte:
            return (long)ImaUtils.getByteValue(body,otype)&0xff;
        case Short:
            return (long)ImaUtils.getShortValue(body,otype);
        case UShort:
            return (long)ImaUtils.getShortValue(body,otype)&0xffff;
        default:
            body.reset();
            ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0066", "A method call to retrieve a Long from a Message object failed because the value is not of type Long and cannot be successfully converted to Long.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }


    /*
     * Read an object.
     * @see javax.jms.StreamMessage#readObject()
     */
    public Object readObject() throws JMSException {
        checkReadable();
        return ImaUtils.getObjectValue(body);
    }

    /*
     * Read a short.
     *
     * @see javax.jms.StreamMessage#readShort()
     */
    public short readShort() throws JMSException {
    	body.mark();
        int otype = getObjectType();
        S_Type type = FieldTypes[otype];
        switch (type) {
        case String:
        case StrLen:
        case Null:
        	body.mark();
            try {
                 return Short.valueOf(ImaUtils.getStringValue(body,otype));
            } catch (RuntimeException re) {
                body.reset();
                throw re;
            }
            catch (JMSException jex) {
                body.reset();
                throw jex;
            }
        case Byte:
            return (short)ImaUtils.getByteValue(body,otype);
        case UByte:
            return (short)(ImaUtils.getByteValue(body,otype)&0xff);
        case UShort:
        case Short:
            return (short)ImaUtils.getShortValue(body,otype);
        default:
            body.reset();
            ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0067", "A method call to retrieve a Short from a Message object failed because the value is not of type Short and cannot be successfully converted to Short.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }


    /*
     * Read a string.
     *
     * @see javax.jms.StreamMessage#readString()
     */
    public String readString() throws JMSException {
    	body.mark();
        int otype = getObjectType();
        S_Type type = FieldTypes[otype];
        String s = null;
        switch (type) {
        case String:
        case StrLen:
        case Null:       s = ImaUtils.getStringValue(body,otype);            break;
        case Int:        s = ""+ImaUtils.getIntValue(body,otype);            break;
        case UInt:       s = ""+((long)ImaUtils.getIntValue(body,otype)&0xffffffff);   break;
        case Boolean:    s = ""+((otype&1) == 1);              break;
        case Long:       s = ""+ImaUtils.getLongValue(body,otype);           break;
        case Byte:       s = ""+ImaUtils.getByteValue(body,otype);           break;
        case UByte:      s = ""+(ImaUtils.getByteValue(body,otype)&0xff);    break;
        case Char:       s = ""+Character.valueOf((char)(ImaUtils.getShortValue(body,otype)&0xffff));  break;
        case Short:      s = ""+ImaUtils.getShortValue(body,otype);          break;
        case UShort:     s = ""+(ImaUtils.getShortValue(body,otype)&0xffff); break;
        case Float:      s = ""+ImaUtils.getFloatValue(body,otype);          break;
        case Double:     s = ""+ImaUtils.getDoubleValue(body,otype);         break;
        case Time:
            long timestamp = (otype==S_Time) ? 0 : ImaUtils.getLongValue(body,8);
            if (timestamp != 0) {
                DateFormat iso8601 = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ");
                String ts = iso8601.format(new Date(timestamp/1000000));
                if (ts.substring(24).equals("0000"))
                    s = ts.substring(0, 22)+"Z";
                else
                    s = ts.substring(0, 26)+':'+ts.substring(26);
                break;
            } else {
                s = "0";
            }
            break;
        case User:
            Object obj = ImaUtils.getUserValue(body,otype);
            if (obj instanceof String)
                s = (String)obj;
            else
                otype |= 0x100;    /* Byte array gives an error */
            break;
        default:
            otype |= 0x100;
            break;
        }
        if (otype >= 0x100) {
            body.reset();
            ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0068", "A call to getString() or readString() on a Message object failed because the value is not of type String and cannot be successfully converted to String.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        return s;
    }
    

    /*
     * Write a boolean.
     *
     * @see javax.jms.StreamMessage#writeBoolean(boolean)
     */
    public void writeBoolean(boolean value) throws JMSException {
        checkWriteable();
        body = ImaUtils.putBooleanValue(body, value);
    }


    /*
     * Write a byte.
     *
     * @see javax.jms.StreamMessage#writeByte(byte)
     */
    public void writeByte(byte value) throws JMSException {
        checkWriteable();
        body = ImaUtils.putByteValue(body,value);
    }


    /*
     * Write a byte array.
     *
     * @see javax.jms.StreamMessage#writeBytes(byte[])
     */
    public void writeBytes(byte[] value) throws JMSException {
        checkWriteable();
        if (value == null) {
            body = ImaUtils.putNullValue(body);
        } else {
        	body = ImaUtils.putByteArrayValue(body,value, 0, value.length);
        }
    }


    /*
     * Write a byte array with position and length.
     *
     * @see javax.jms.StreamMessage#writeBytes(byte[], int, int)
     */
    public void writeBytes(byte[] value, int pos, int len) throws JMSException {
        checkWriteable();
        if (value == null)
        	body = ImaUtils.putNullValue(body);
        else
        	body = ImaUtils.putByteArrayValue(body, value, pos, len);
    }


    /*
     * Write a character.
     *
     * @see javax.jms.StreamMessage#writeChar(char)
     */
    public void writeChar(char value) throws JMSException {
        checkWriteable();
        body = ImaUtils.putCharValue(body, value);
    }

    /*
     * Write a double.
     *
     * @see javax.jms.StreamMessage#writeDouble(double)
     */
    public void writeDouble(double value) throws JMSException {
        checkWriteable();
        body = ImaUtils.putDoubleValue(body, value);
    }


    /*
     * Write a float.
     *
     * @see javax.jms.StreamMessage#writeFloat(float)
     */
    public void writeFloat(float value) throws JMSException {
        checkWriteable();
        body = ImaUtils.putFloatValue(body, value);
    }


    /*
     * Write an int.
     *
     * @see javax.jms.StreamMessage#writeInt(int)
     */
    public void writeInt(int value) throws JMSException {
        checkWriteable();
        body = ImaUtils.putIntValue(body, value);
    }


    /*
     * Write a long.
     *
     * @see javax.jms.StreamMessage#writeLong(long)
     */
    public void writeLong(long value) throws JMSException {
        checkWriteable();
        body = ImaUtils.putLongValue(body, value);
    }


    /*
     * Write an object.
     *
     * @see javax.jms.StreamMessage#writeObject(java.lang.Object)
     */
    public void writeObject(Object value) throws JMSException {
        checkWriteable();
        body = ImaUtils.putObjectValue(body, value);
    }


    /*
     * Write a short.
     *
     * @see javax.jms.StreamMessage#writeShort(short)
     */
    public void writeShort(short value) throws JMSException {
        checkWriteable();
        body = ImaUtils.putShortValue(body,value);
    }


    /*
     * Write a string.
     *
     * @see javax.jms.StreamMessage#writeString(java.lang.String)
     */
    public void writeString(String value) throws JMSException {
        checkWriteable();
        body = ImaUtils.putStringValue(body,value);
    }

    /*
     * (non-Javadoc)
     * @see com.ibm.ima.jms.impl.ImaMessage#formatBody()
     */
    public String formatBody() {
    	if (body == null)
    		return null;
        ByteBuffer bb = body.duplicate();
        if (isReadonly) 
        	bb.position(0);
      	else
      		bb.flip();
        if (bb.limit()<1 || bb.get() != (byte)0x9E)
        	return null;
        
        boolean first = true;
        StringBuffer sb = new StringBuffer();
        sb.append('[');
        try {
	        while (bb.remaining() > 0) {
	        	Object obj = ImaUtils.getObjectValue(bb);
	        	if (obj == null)
	        		break;
	        	if (first) 
	        		first = false;
	        	else
	        		sb.append(", ");
	        	if (obj instanceof String) {
	        		sb.append('"').append(obj).append('"');
	        	} else {
	        		sb.append(obj);
	        	}
	        }
        } catch (JMSException jex) {}
        sb.append(']');
        return sb.toString();
    }
}
