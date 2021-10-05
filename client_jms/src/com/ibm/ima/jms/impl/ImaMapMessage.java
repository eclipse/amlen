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

import java.util.Arrays;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;

/**
 * Implement the MapMessage interface for the IBM MessageSight JMS client.
 *
 */
public class ImaMapMessage extends ImaMessage implements MapMessage {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    HashMap<String, Object> map = new HashMap<String, Object>();
    
    /*
     * Constructor for the MapMessage 
     */
    public ImaMapMessage(ImaSession session) {
        super(session, MTYPE_MapMessage);
    }
    
    /*
     * Create a map message from another map message.
     */
    public ImaMapMessage(MapMessage msg, ImaSession session) throws JMSException {
        super(session, MTYPE_MapMessage, (Message)msg);
        Enumeration <?> xmap = msg.getMapNames();
        while (xmap.hasMoreElements()) {
            String prop = ""+xmap.nextElement();
            Object obj = msg.getObject(prop);
            setObject(prop, obj);
        }
    }
    /*
     * Constructor for a cloned received message
     */
    public ImaMapMessage(ImaMessage msg) {
        super(msg);
        this.map = ((ImaMapMessage)msg).map;
    }

    /*
     * When a map message is not writeable, the body byte array is valid.
     * When moving from writeable to readonly, convert the hashmap into bytes.
     */
    public void reset() throws JMSException {
        if (!isReadonly) {
            putMap(map);
            body.flip();
        }
        isReadonly = true;
    }
    
    /*
     * Clear the body of the message and make it writeable again.
     * @see com.ibm.ima.jms.impl.ImaMessage#clearBody()
     */
    public void clearBody() throws JMSException {
        super.clearBody();
        map.clear();
    }
    
    /*
     * Initialize the content.
     * This reads the content of the message into a hashmap.
     */
    public void initContent() throws JMSException {
        super.initContent();
        if (body != null)
        	body.position(0);
        if (!isReadonly) {
            isReadonly = true;
            getMap(map);
        }    
    }
    
    
    /* 
     * @see javax.jms.MapMessage#getBoolean(java.lang.String)
     */
    public boolean getBoolean(String name) throws JMSException {
        Object obj = map.get(name);
        if (obj == null) {
        	ImaNumberFormatException nex = new ImaNumberFormatException("CWLNC0078", "A call to get data from a MapMessage failed because the map element {0} does not exist in the message or the element contains a null object.", ((name!=null)?name:""));
            ImaTrace.traceException(2, nex);
            throw nex;
        }
        if (obj instanceof Boolean)
            return ((Boolean)obj).booleanValue();
        else if (obj instanceof String) 
            return Boolean.valueOf((String)obj);
        else {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0061", "A method call to retrieve a Boolean from a Message object failed because the value is not Boolean and cannot be successfully converted to Boolean.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }

    /* 
     * @see javax.jms.MapMessage#getByte(java.lang.String)
     */
    public byte getByte(String name) throws JMSException {
        Object obj = map.get(name);
        if (obj == null) {
        	ImaNumberFormatException nex = new ImaNumberFormatException("CWLNC0078", "A call to get data from a MapMessage failed because the map element {0} does not exist in the message or the element contains a null object.", ((name!=null)?name:""));
            ImaTrace.traceException(2, nex);
            throw nex;
        }
        if (obj instanceof Byte)
            return ((Byte)obj).byteValue();
        else if (obj instanceof String) 
            return Byte.valueOf((String)obj);
        else {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0062", "A method call to retrieve a Byte from a Message object failed because the value is not of type Byte and cannot be successfully converted to Byte.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }

    /* 
     * @see javax.jms.MapMessage#getBytes(java.lang.String)
     */
    public byte[] getBytes(String name) throws JMSException {
        Object obj = map.get(name);
        if (obj == null)
            return null;
        if (obj instanceof byte [])
            return (byte [])obj;
        ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0070", "A call to getBytes() or readBytes() on a Message object failed because the value is not of type byte array (byte[]) and cannot be successfully converted to a byte array.");
        ImaTrace.traceException(2, jex);
        throw jex;
    }

    /* 
     * @see javax.jms.MapMessage#getChar(java.lang.String)
     */
    public char getChar(String name) throws JMSException {
        Object obj = map.get(name);
        if (obj == null) {
            if (map.containsKey(name)) {
            	ImaNullPointerException nex = new ImaNullPointerException("CWLNC0081", "A call to getChar() on a MapMessage or to readChar() on a StreamMessage failed because the map element {0} contains a null object or the stream object is null.", name);
                ImaTrace.traceException(2, nex);
                throw nex;
            }
            ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0078", "A call to get data from a MapMessage failed because the map element {0} does not exist in the message or the element contains a null object.", ((name!=null)?name:""));
            ImaTrace.traceException(2, jex);
            throw jex;
        }    
        if (obj instanceof Character)
            return ((Character)obj).charValue();
        ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0069", "A call to getChar() or readChar() on a Message object failed because the value is not of type Character and cannot be successfully converted to Character.");
        ImaTrace.traceException(2, jex);
        throw jex;
    }

    /* 
     * @see javax.jms.MapMessage#getDouble(java.lang.String)
     */
    public double getDouble(String name) throws JMSException {
        Object obj = map.get(name);        
        if (obj == null) {
        	ImaNumberFormatException nex = new ImaNumberFormatException("CWLNC0078", "A call to get data from a MapMessage failed because the map element {0} does not exist in the message or the element contains a null object.", ((name!=null)?name:""));
            ImaTrace.traceException(2, nex);
            throw nex;
        }
        if (obj instanceof Double)
            return ((Double)obj).doubleValue();
        else if (obj instanceof String) 
            return Double.valueOf((String)obj);
        else if (obj instanceof Float)
            return ((Float)obj).doubleValue();
        else {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0063", "A method call to retrieve a Double from a Message object failed because the value is not of type Double and cannot be successfully converted to Double.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }

    /* 
     * @see javax.jms.MapMessage#getFloat(java.lang.String)
     */
    public float getFloat(String name) throws JMSException {
        Object obj = map.get(name);
        if (obj == null) {
        	ImaNumberFormatException nex = new ImaNumberFormatException("CWLNC0078", "A call to get data from a MapMessage failed because the map element {0} does not exist in the message or the element contains a null object.", ((name!=null)?name:""));
            ImaTrace.traceException(2, nex);
            throw nex;
        }
        if (obj instanceof Float)
            return ((Float)obj).floatValue();
        else if (obj instanceof String) 
            return Float.valueOf((String)obj);
        else {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0065", "A method call to retrieve a Float from a Message object failed because the value is not of type Float and cannot be successfully converted to Float.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }

    /* 
     * @see javax.jms.MapMessage#getInt(java.lang.String)
     */
    public int getInt(String name) throws JMSException {
        Object obj = map.get(name);
        if (obj == null) {
        	ImaNumberFormatException nex = new ImaNumberFormatException("CWLNC0078", "A call to get data from a MapMessage failed because the map element {0} does not exist in the message or the element contains a null object.", ((name!=null)?name:""));
            ImaTrace.traceException(2, nex);
            throw nex;
        }
        if (obj instanceof Integer)
            return ((Integer)obj).intValue();
        else if (obj instanceof String)
            return Integer.valueOf((String)obj);
        else if (obj instanceof Byte)
            return ((Byte)obj).intValue();
        else if (obj instanceof Short)
            return ((Short)obj).intValue();
        else {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0064", "A method call to retrieve an Integer from a Message object failed because the value is not of type Integer and cannot be successfully converted to Integer.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }

    /* 
     * @see javax.jms.MapMessage#getLong(java.lang.String)
     */
    public long getLong(String name) throws JMSException {
        Object obj = map.get(name);
        if (obj == null) {
        	ImaNumberFormatException nex = new ImaNumberFormatException("CWLNC0078", "A call to get data from a MapMessage failed because the map element {0} does not exist in the message or the element contains a null object.", ((name!=null)?name:""));
            ImaTrace.traceException(2, nex);
            throw nex;
        }
        if (obj instanceof Long)
            return ((Long)obj).longValue();
        else if (obj instanceof String)
            return Long.valueOf((String)obj);
        else if (obj instanceof Integer)
            return ((Integer)obj).longValue();
        else if (obj instanceof Byte)
            return ((Byte)obj).longValue();
        else if (obj instanceof Short)
            return ((Short)obj).longValue();
        else {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0066", "A method call to retrieve a Long from a Message object failed because the value is not of type Long and cannot be successfully converted to Long.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }

    /* 
     * @see javax.jms.MapMessage#getMapNames()
     */
    @SuppressWarnings("rawtypes")
    public Enumeration getMapNames() throws JMSException {
        return new SetEnumeration(map.keySet());
    }

    /* 
     * @see javax.jms.MapMessage#getObject(java.lang.String)
     */
    public Object getObject(String name) throws JMSException {
        return map.get(name);
    }

    /* 
     * @see javax.jms.MapMessage#getShort(java.lang.String)
     */
    public short getShort(String name) throws JMSException {
        Object obj = map.get(name);
        if (obj == null) {
        	ImaNumberFormatException nex = new ImaNumberFormatException("CWLNC0078", "A call to get data from a MapMessage failed because the map element {0} does not exist in the message or the element contains a null object.", ((name!=null)?name:""));
            ImaTrace.traceException(2, nex);
            throw nex;
        }
        if (obj instanceof Short)
            return ((Short)obj).shortValue();
        else if (obj instanceof String)
            return Short.valueOf((String)obj);
        else if (obj instanceof Byte)
            return ((Byte)obj).shortValue();
        else {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0067", "A method call to retrieve a Short from a Message object failed because the value is not of type Short and cannot be successfully converted to Short.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }

    /* 
     * @see javax.jms.MapMessage#getString(java.lang.String)
     */
    public String getString(String name) throws JMSException {
        Object obj = map.get(name);
        if (obj != null && obj instanceof byte[]) {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0068", "A call to getString() or readString() on a Message object failed because the value is not of type String and cannot be successfully converted to String.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        return obj == null ? null : obj.toString();
    }

    /* 
     * @see javax.jms.MapMessage#itemExists(java.lang.String)
     */
    public boolean itemExists(String name) throws JMSException {
        return map.containsKey(name);
    }

    /* 
     * @see javax.jms.MapMessage#setBoolean(java.lang.String, boolean)
     */
    public void setBoolean(String name, boolean value) throws JMSException {
        setObject(name, Boolean.valueOf(value));
    }

    /* 
     * @see javax.jms.MapMessage#setByte(java.lang.String, byte)
     */
    public void setByte(String name, byte value) throws JMSException {
        setObject(name, Byte.valueOf(value));
    }

    /* 
     * @see javax.jms.MapMessage#setBytes(java.lang.String, byte[])
     */
    public void setBytes(String name, byte[] value) throws JMSException {
        setObject(name, value);
    }

    /* 
     * @see javax.jms.MapMessage#setBytes(java.lang.String, byte[], int, int)
     */
    public void setBytes(String name, byte[] value, int pos, int len) throws JMSException {
        byte [] b = new byte[len];
        System.arraycopy(value, pos, b, 0, len);
        setObject(name, b);
    }

    /* 
     * @see javax.jms.MapMessage#setChar(java.lang.String, char)
     */
    public void setChar(String name, char value) throws JMSException {
        setObject(name, Character.valueOf(value));
    }

    /* 
     * @see javax.jms.MapMessage#setDouble(java.lang.String, double)
     */
    public void setDouble(String name, double value) throws JMSException {
        setObject(name, Double.valueOf(value));
    }

    /* 
     * @see javax.jms.MapMessage#setFloat(java.lang.String, float)
     */
    public void setFloat(String name, float value) throws JMSException {
        setObject(name, Float.valueOf(value));
    }

    /* 
     * @see javax.jms.MapMessage#setInt(java.lang.String, int)
     */
    public void setInt(String name, int value) throws JMSException {
        setObject(name, Integer.valueOf(value));
    }

    /* 
     * @see javax.jms.MapMessage#setLong(java.lang.String, long)
     */
    public void setLong(String name, long value) throws JMSException {
        setObject(name, Long.valueOf(value));
    }

    /* 
     * @see javax.jms.MapMessage#setObject(java.lang.String, java.lang.Object)
     */
    public void setObject(String name, Object value) throws JMSException {
        if (name == null || name.length() == 0) {
        	ImaIllegalArgumentException iex = new ImaIllegalArgumentException("CWLNC0082", "A method call to create a MapMessage element failed because the name specified is null or is an empty string.");
            ImaTrace.traceException(2, iex);
            throw iex;
        }
        checkWriteable();
        if (value == null ||
            value instanceof String    ||
            value instanceof Integer   ||
            value instanceof Boolean   ||
            value instanceof Byte      ||
            value instanceof Long      ||
            value instanceof Double    ||
            value instanceof Float     ||
            value instanceof Character ||
            value instanceof byte []   ||
            value instanceof Short ) {
            map.put(name, value);
        } else {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0034", "A method call to set an object on a message failed because the input object type ({0}) is not supported.  The object types that are supported are Boolean, Byte, byte[], Character, Double, Float, Integer, Long, Short, and String.", value.getClass());
            ImaTrace.traceException(2, jex);
            throw jex;
        }

    }

    /* 
     * @see javax.jms.MapMessage#setShort(java.lang.String, short)
     */
    public void setShort(String name, short value) throws JMSException {
        setObject(name, Short.valueOf(value));
    }

    /* 
     * @see javax.jms.MapMessage#setString(java.lang.String, java.lang.String)
     */
    public void setString(String name, String value) throws JMSException {
        setObject(name, (Object)value);
    }
    
    /*
     * Fill in the stream from a map
     */
    private void putMap(Map <String,Object> map) throws JMSException {
        checkWriteable();
        body.position(0);
        body.put((byte)0x9F);
        Iterator <String> it = map.keySet().iterator();
        while (it.hasNext()) {
            String key = it.next();
            body = ImaUtils.putNameValue(body,key);
            body = ImaUtils.putObjectValue(body,map.get(key));
        }
    }    
    
    /*
     * Fill in a map from the stream
     */
    void getMap(Map <String,Object>map) throws JMSException {
        checkReadable();
        if ((body == null) || (body.get(0) != (byte)0x9F)) {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0211", "A call to reset() or to initContent() on a MapMessage failed because the message is not a MapMessage. This can occur if a message received from MessageSight is incorrectly cast to MapMessage.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        body.position(1);
        while (body.remaining()>0) {
            int otype = ImaUtils.getObjectType(body);
            S_Type type = FieldTypes[otype];
            if (type != S_Type.Name && type != S_Type.NameLen && type != S_Type.ID) {
            	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0202", "A call to getMap() on a MapMessage failed because the data content type is not known.  This indicates that data corruption occurred.");
                ImaTrace.traceException(2, jex);
                throw jex;
            }
            String key = ImaUtils.getNameValue(body,otype);
            Object obj = ImaUtils.getObjectValue(body);
            map.put(key, obj);
        }
    }   

    /*
     * @see com.ibm.ima.jms.impl.ImaMessage#formatBody()
     */
    public String formatBody() {
    	int i;
    	StringBuffer sb = new StringBuffer();
    	if (map == null)
        	return null;
    	Object [] keys = map.keySet().toArray();
    	Arrays.sort(keys);
        sb.append("{");
        for (i=0; i<keys.length; i++ ) {
        	if (i>0)
	            sb.append(", ");
            sb.append((String)keys[i]).append('=').append(map.get((String)keys[i]));
        }
        sb.append("}");
        return sb.toString();
    }
}
