/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.plugin.impl;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import com.ibm.ima.plugin.ImaDestinationType;
import com.ibm.ima.plugin.ImaMessage;
import com.ibm.ima.plugin.ImaMessageType;
import com.ibm.ima.plugin.ImaReliability;
import com.ibm.ima.plugin.ImaTransaction;
import com.ibm.ima.plugin.util.ImaJson;

public class ImaMessageImpl implements ImaMessage {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    ImaMessageType  msgtype;
    boolean         persistent;
    ImaReliability  qos;
    long            expiration;
    Map<String,Object> props;
    byte []         body;
    int             retain;
    
    String          dest;
    ImaDestinationType destType;
    ImaConnectionImpl   connection;
    final int           seqnum;
    Object          userData;
    static Charset utf8 = Charset.forName("UTF-8");
    
    /*
     * Constructor
     */
    public ImaMessageImpl(ImaMessageType mtype) {
        this.msgtype    = mtype;
        this.qos     = ImaReliability.qos0;
        seqnum = 0;
    }

    /*
     * Constructor for inbound message
     */
    ImaMessageImpl(ImaConnectionImpl con, int seqnum, int type, int flags, String dest, Map<String, Object> props,
            byte[] body) {
        this.msgtype = toMessageType(type);
        this.qos     = toQoS(flags);
        this.persistent = (flags & 0x04) != 0;
        this.retain  = (flags>>3)&3;
        this.dest = dest;
        if ((flags & 0x20) != 0)
            this.destType = ImaDestinationType.queue;
        else 
            this.destType = ImaDestinationType.topic;
        this.connection = con;
        this.props = props;
        this.body = body;
        this.seqnum = seqnum;
    }
    
    
    /*
     * Convert an integer message type from imasever to a ImaMessageType enumeration
     */
    static ImaMessageType toMessageType(int type) {
        switch(type) {
        case 0x01:  /* MTYPE_Message         JMS message with no payload      */
            return ImaMessageType.jmsMessage;
        case 0x02:  /* MTYPE_BytesMessage    JMS BytesMessage                 */
            return ImaMessageType.jmsBytesMessage;
        case 0x03:  /* MTYPE_MapMessage      IMA JMS MapMessage               */
            return ImaMessageType.jmsMapMessage;
        case 0x04:  /* MTYPE_ObjectMessage   IMA JMS ObjectMessage            */
            return ImaMessageType.jmsObjectMessage;
        case 0x05:  /* MTYPE_StreamMessage   IMA JMS StreamMessage            */
            return ImaMessageType.jmsStreamMessage;
        case 0x06:  /* MTYPE_TextMessage     JMS TextMessage                  */
            return ImaMessageType.jmsTextMessage;
        case 0x07:  /* MTYPE_TextMessageNull JMS TextMessage with no content  */
            return ImaMessageType.jmsTextMessage;
        case 0x11:  /* MTYPE_MQTT_Binary     MQTT binary payload              */
            return ImaMessageType.bytes;
        case 0x12:  /* MTYPE_MQTT_Text       MQTT text string payload         */
            return ImaMessageType.text;
        case 0x14:  /* MTYPE_MQTT_TextObject JSON object                      */
            return ImaMessageType.jsonObject;
        case 0x15:  /* MTYPE_MQTT_TextArray  JSON array                       */
            return ImaMessageType.jsonArray;
        case 0x20:  /* MTYPE_NullRetained    Retained message with a null body */
            return ImaMessageType.nullRetained;
        }    
        return ImaMessageType.bytes;
    }
    
    /*
     * Convert an ImaMessageType enumeration into a 
     */
    static int fromMessageType(ImaMessageType type) {
        switch (type) {
        case bytes:            return 0x11;  /* MTYPE_MQTT_Binary     MQTT binary payload           */
        case text:             return 0x12;  /* MTYPE_MQTT_Text       MQTT text string payload      */
        case jsonObject:       return 0x14;  /* MTYPE_MQTT_TextObject JSON object                   */
        case jsonArray:        return 0x15;  /* MTYPE_MQTT_TextArray  JSON array                    */
        case jmsMessage:       return 0x01;  /* MTYPE_Message         JMS message with no payload   */
        case jmsBytesMessage:  return 0x02;  /* MTYPE_BytesMessage    JMS BytesMessage              */
        case jmsMapMessage:    return 0x03;  /* MTYPE_MapMessage      JMS MapMessage                */  
        case jmsObjectMessage: return 0x04;  /* MTYPE_ObjectMessage   JMS ObjectMessage             */
        case jmsStreamMessage: return 0x05;  /* MTYPE_StreamMessage   JMS StreamMessage             */
        case jmsTextMessage:   return 0x06;  /* MTYPE_TextMessage     JMS TextMessage               */
        case nullRetained:     return 0x20;  /* MTYPE_NullRetained    Retained msg with a null body */
        }    
        return 0x11; /* MTYPE_MQTT_Binary */
    }
    
    /*
     * Convert from flags to ImaReliability
     */
    static ImaReliability toQoS(int flags) {
        switch(flags&3) {
        case 0: return ImaReliability.qos0;
        case 1: return ImaReliability.qos1;
        case 2: return ImaReliability.qos2;
        }
        return ImaReliability.qos0;       /* Showld not happen */
    }
    
    /*
     * Convert from ImaReliability to an int value
     */
    static int fromQoS(ImaReliability qos) {
    	switch(qos) {
        case qos0:  return 0;
        case qos1:  return 1;
        case qos2:  return 2;
        }
        return 0;    /* Should not happen */
    }
    
    /*
     * @see com.ibm.ima.ext.ImaMessage#addProperties(java.util.Map)
     */
    public ImaMessage addProperties(Map<String, Object> newprops) {
        if (props == null)
            props = new HashMap<String,Object>();
        props.putAll(newprops);
        return this;
    }

    public void acknowledge(int rc, ImaTransaction transaction) {
        if (seqnum != 0) {
            connection.acknowledge(this, rc, (ImaTransactionImpl) transaction);
            return;
        }
        if (transaction != null)
            throw new RuntimeException("Message with sequence number 0 can not be added to transaction");
    }

    public void acknowledge(int rc) {
        if (seqnum != 0)
            connection.acknowledge(this, rc, null);
    }
    /*
     * @see com.ibm.ima.ext.ImaMessage#clearProperties()
     */
    public ImaMessage clearProperties() {
        props = null;
        return this;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getBodyBytes()
     */
    public byte[] getBodyBytes() {
        return body;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getBodyMap()
     */
    public Map<String, Object> getBodyMap() {
        /* TODO: */
        return null;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getBodyText()
     */
    public String getBodyText() {
        ByteBuffer bb = ByteBuffer.wrap(body);
        String text = ImaPluginUtils.fromUTF8(bb, bb.remaining(), true);
        return text;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getBooleanProperty(java.lang.String, boolean)
     */
    public boolean getBooleanProperty(String name, boolean default_value) {
        Object obj = getProperty(name);
        if (obj == null)
            return default_value;             
        if (obj instanceof Boolean) {
            return ((Boolean)obj).booleanValue();
        } else if (obj instanceof String) { 
            if ("true".equalsIgnoreCase((String)obj))
                return true;
            if ("false".equalsIgnoreCase((String)obj))
                return false;
        } else if (obj instanceof Number) {
            return ((Number)obj).intValue() != 0;
        }
        return default_value;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getDestination()
     */
    public String getDestination() {
        return dest;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getDestinationType()
     */
    public ImaDestinationType getDestinationType() {
        return destType;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getMessageType()
     */
    public ImaMessageType getMessageType() {
        return msgtype;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getPersistence()
     */
    public boolean getPersistence() {
        return persistent;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getReliability()
     */
    public ImaReliability getReliability() {
        return qos;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getRetain()
     */
    public boolean getRetain() {
        return (retain&2) != 0;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getRetainAsPublished()
     */
    public boolean getRetainAsPublished() {
        return retain != 0;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getProperties()
     */
    public Map<String, Object> getProperties() {
        return Collections.unmodifiableMap(props);
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getIntPropery(java.lang.String, int)
     */
    public int getIntPropery(String name, int default_value) {
        Object obj = getProperty(name);
        if (obj == null)
            return default_value;      
        if (obj instanceof Number) {
            return ((Number)obj).intValue();
        } else if (obj instanceof Boolean) {
            return ((Boolean)obj).booleanValue() ? 1 : 0;
        } else if (obj instanceof String) { 
            try {
                return Integer.valueOf((String)obj);
            } catch (Exception e) {
            }
        }
        return default_value;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getProperty(java.lang.String)
     */
    public Object getProperty(String name) {
        if (props == null)
            return null;
        return props.get(name);
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#getStringProperty(java.lang.String)
     */
    public String getStringProperty(String name) {
        Object obj = getProperty(name);
        if (obj instanceof String)
            return (String)obj;
        else return ""+obj;
    }
    
    /*
     * Get a user object associated with this message.
     * @return An object associated with this message or null if there is none
     *
     * @see com.ibm.ima.plugin.ImaMessage#getUserData()
     */
    public Object getUserData() {
        return userData;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#propertyExists(java.lang.String)
     */
    public boolean propertyExists(String name) {
        if (props == null)
            return false;
        return props.containsValue(name);
    }

    /*
     * @see com.ibm.ima.plugin.ImaMessage#setMesageType(com.ibm.ima.plugin.ImaMessageType)
     */
    public ImaMessage setMessageType(ImaMessageType msgtype) {
        this.msgtype = msgtype;
        return this;
    }
    
    /*
     * @see com.ibm.ima.ext.ImaMessage#setBodyBytes(byte[])
     */
    public ImaMessage setBodyBytes(byte[] body) {
        this.body = body;
        return this;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#setBodyMap(java.util.Map)
     */
    public ImaMessage setBodyMap(Map<String, Object> body) {
        StringBuffer sb = new StringBuffer();
        boolean isFirst = true;
        sb.append("{");
        /* Put in all property entries */
        Iterator <String> it = body.keySet().iterator();
        while (it.hasNext()) {
            String key = it.next();
            if (isFirst)
                isFirst = false;
            else
                sb.append(",");
            ImaJson.putString(sb, key);
            sb.append(":");
            ImaJson.put(sb, body.get(key));
        }
        sb.append("}");
        this.body = new String(sb).getBytes(utf8);
        return this;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#setBodyText(java.lang.String)
     */
    public ImaMessage setBodyText(String body) {
    	if (body != null) {
    		this.body = body.getBytes(utf8);
    	} else {
    	    this.body = null;
    	}
        return this;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#setPersistent(boolean)
     */
    public ImaMessage setPersistent(boolean persist) {
        persistent = persist;
        return this;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#setReliability(com.ibm.ima.ext.ImaReliability)
     */
    public ImaMessage setReliability(ImaReliability reliability) {
        qos = reliability;
        return this;
    }

    /*
     * @see com.ibm.ima.ext.ImaMessage#setRetain(boolean)
     */
    public ImaMessage setRetain(boolean retain) {
        this.retain = retain ? 3 : 0;
        return this;
    }
    
    /*
     * Set a user object associated with this message.
     * @param userdata  An object to associate with this message
     * 
     * @see com.ibm.ima.plugin.ImaMessage#setUserData(java.lang.Object)
     */
    public void setUserData(Object userdata) {
        userData = userdata;
    }

}
