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
package com.ibm.ima.jms.impl;

import java.io.Serializable;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.zip.CRC32;

import javax.jms.BytesMessage;
import javax.jms.CompletionListener;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.JMSProducer;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.ObjectMessage;
import javax.jms.StreamMessage;
import javax.jms.TextMessage;

public class ImaJmsProducer extends ImaReadonlyProperties implements JMSProducer {

    CompletionListener listener;
    ImaMessage msg;
    boolean isClosed;
    ImaSession session;                                   /* The parent session */
    ImaConnection connect;
    int deliverymode = Message.DEFAULT_DELIVERY_MODE;     /* Default the delivery mode */
    int priority =     Message.DEFAULT_PRIORITY;          /* Default the priority */
    long    ttl  =     Message.DEFAULT_TIME_TO_LIVE;      /* Default the time to live */
    boolean disableMessageID = false;                 
    boolean disableTimestamp = false;
    long    deliveryDelay = 0;
    boolean retainSet = false;
    
    char [] msgidBuffer = new char[19];
    int msgidCount = 0;
    long msgidTime = 0L;
    
    private ImaProducerAction   sendMsgAction;    /* For anonymous msgs */
    
    public ImaJmsProducer(ImaJmsContext context) {
        super(null);
        session = context.session;
        connect = session.connect;
        props = new ImaPropertiesImpl(ImaJms.PTYPE_Connection);
        putInternal("context", context);
        initMsgID();
        if (connect.isTraceable(6)) {
            connect.trace("JMSProducer created: producer=" + toString(ImaConstants.TS_All));
        }
        int val = connect.getInt("DisableMessageID", -1);
        if (val >= 0)
            setDisableMessageID(val != 0);
        val = connect.getInt("DisableMessageTimestamp", -1);
        if (val >= 0) 
            setDisableMessageTimestamp(val != 0);
        priority = session.defaultPriority;
        long ttl = (long)connect.getInt("DefaultTimeToLive", -1) * 1000;
        if (ttl >= 0)
            setTimeToLive(ttl);
        
        msg = new ImaMessage(session, ImaMessage.MTYPE_Unknown);
    }
    
    /* 
     * Check if the session is closed.
     */
    void checkClosed() {
        if (session.isClosed.get()) {
            ImaIllegalStateRuntimeException jex = new ImaIllegalStateRuntimeException("CWLNC0008", "The connection is closed");
            connect.traceException(2, jex);
            throw jex;
        }
    }
    
    /*
     * Initialize the message ID
     */
    void initMsgID() {
        msgidBuffer[0] = 'I';
        msgidBuffer[1] = 'D';
        msgidBuffer[2] = ':';
        
        CRC32 crc = new CRC32();
        crc.update(session.connect.clientid.getBytes());
        long now = System.currentTimeMillis();
        crc.update((int)((now/77)&0xff));
        crc.update((int)((now/79731)&0xff));
        crc.update(hashCode()/77);
        long crcval = crc.getValue();
        for (int i=11; i<15; i++) {
            msgidBuffer[i] = ImaProducer.base32[(int)(crcval&0x1F)];
            crcval >>= 5;
        }
        msgidCount = 0;
        makeMsgID();
    }
    
    /*
     * Make a message ID.
     * The message ID consists of the required characters "ID:" followed by 40 bit
     * timestamp in base 32, a four digit producer unique string, and a four base32 counter.
     */
    String makeMsgID() {
        int count = msgidCount++;
        if ((count&0xFFFFF) != ((count-1)&0xFFFFF)) {
            long now = System.currentTimeMillis();
            if (now != msgidTime) {
                msgidTime = now;
                now >>= 2;
                for (int i=10; i>2; i--) {
                    msgidBuffer[i] = ImaProducer.base32[(int)(now&0x1f)]; 
                    now >>= 5;
                }
            }
        } 
        for (int i=18; i>14; i--) {
            msgidBuffer[i] = ImaProducer.base32[count&0x1f];
            count >>= 5;
        }
        return new String(msgidBuffer);    
    }
    
    /*
     * 
     * @see javax.jms.JMSProducer#clearProperties()
     */
    public JMSProducer clearProperties() {
        if (msg != null && msg.props != null) {
            msg.props = null;
        }    
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#getAsync()
     */
    public CompletionListener getAsync() {
        return listener;
    }
    
    /*
     * @see javax.jms.JMSProducer#getBooleanProperty(java.lang.String)
     */
    public boolean getBooleanProperty(String name) {
        Object obj = msg.getObjectProperty(name);
        if (obj == null)
            return false;                /* Required by TCK, not specified in spec */
        if (obj instanceof Boolean)
            return ((Boolean)obj).booleanValue();
        else if (obj instanceof String) 
            return Boolean.valueOf((String)obj);
        ImaMessageFormatRuntimeException jex = new ImaMessageFormatRuntimeException("CWLNC0061", "Boolean conversion error");
        connect.traceException(2, jex);
        throw jex;
    }

    /*
     * @see javax.jms.JMSProducer#getByteProperty(java.lang.String)
     */
    public byte getByteProperty(String name) {
        Object obj = msg.getObjectPropertyNoNull(name);
        if (obj instanceof Byte)
            return ((Byte)obj).byteValue();
        else if (obj instanceof String) 
            return Byte.valueOf((String)obj);
        ImaMessageFormatRuntimeException jex = new ImaMessageFormatRuntimeException("CWLNC0062", "Byte conversion error");
        connect.traceException(2, jex);
        throw jex;
    }

    /*
     * (non-Javadoc)
     * @see javax.jms.JMSProducer#getDeliveryDelay()
     */
    public long getDeliveryDelay() {
        return deliveryDelay;
    }

    /*
     * @see javax.jms.JMSProducer#getDeliveryMode()
     */
    public int getDeliveryMode() {
        return deliverymode;
    }

    /*
     * @see javax.jms.JMSProducer#getDisableMessageID()
     */
    public boolean getDisableMessageID() {
        return disableMessageID;
    }

    /*
     * @see javax.jms.JMSProducer#getDisableMessageTimestamp()
     */
    public boolean getDisableMessageTimestamp() {
        return disableTimestamp;
    }

    /*
     * @see javax.jms.JMSProducer#getDoubleProperty(java.lang.String)
     */
    public double getDoubleProperty(String name) {
        Object obj = msg.getObjectProperty(name);
        if (obj == null) {
            NullPointerException nex = new ImaNullPointerException("CWLNC0053", "The property does not exist: {0}" + name);
            connect.traceException(2, nex);
            throw nex;
        }
        if (obj instanceof Double)
            return ((Double)obj).doubleValue();
        else if (obj instanceof Float)
            return ((Float)obj).doubleValue();
        else if (obj instanceof String) 
            return Double.valueOf((String)obj);
        ImaMessageFormatRuntimeException jex = new ImaMessageFormatRuntimeException("CWLNC0063", "Double conversion error");
        connect.traceException(2, jex);
        throw jex;
    }

    /*
     * @see javax.jms.JMSProducer#getFloatProperty(java.lang.String)
     */
    public float getFloatProperty(String name) {
        Object obj = msg.getObjectProperty(name);
        if (obj == null) {
            NullPointerException nex = new ImaNullPointerException("CWLNC0053", "The property does not exist: {0}" + name);
            connect.traceException(2, nex);
            throw nex;
        }
        if (obj instanceof Float)
            return ((Float)obj).floatValue();
        else if (obj instanceof String) 
            return Float.valueOf((String)obj);
        ImaMessageFormatRuntimeException jex = new ImaMessageFormatRuntimeException("CWLNC0065", "Float conversion error");
        connect.traceException(2, jex);
        throw jex;
    }

    /*
     * @see javax.jms.JMSProducer#getIntProperty(java.lang.String)
     */
    public int getIntProperty(String name) {
        Object obj = msg.getObjectPropertyNoNull(name);
        if (obj instanceof Integer)
            return ((Integer)obj).intValue();
        else if (obj instanceof Byte || obj instanceof Short)
            return Integer.valueOf(((Number)obj).intValue());
        else if (obj instanceof String) 
            return Integer.valueOf((String)obj);
        ImaMessageFormatRuntimeException jex = new ImaMessageFormatRuntimeException("CWLNC0064", "Integer conversion error");
        connect.traceException(2, jex);
        throw jex;
    }

    /*
     * @see javax.jms.JMSProducer#getJMSCorrelationID()
     */
    public String getJMSCorrelationID() {
        return msg.correlationID;
    }

    /*
     * @see javax.jms.JMSProducer#getJMSCorrelationIDAsBytes()
     */
    public byte[] getJMSCorrelationIDAsBytes() {
        throw new java.lang.UnsupportedOperationException("getJMSCorrelationIDAsBytes is not supported.  Use getJMSCorrelationID.");
    }

    /*
     * @see javax.jms.JMSProducer#getJMSReplyTo()
     */
    public Destination getJMSReplyTo() {
        return msg.jmsreply;
    }

    /*
     * @see javax.jms.JMSProducer#getJMSType()
     */
    public String getJMSType() {
        return msg.jmstype;
    }

    /*
     * @see javax.jms.JMSProducer#getLongProperty(java.lang.String)
     */
    public long getLongProperty(String name) {
        Object obj = msg.getObjectPropertyNoNull(name);
        if (obj instanceof Long)
            return ((Long)obj).longValue();
        else if (obj instanceof Integer || obj instanceof Byte || obj instanceof Short)
            return Long.valueOf(((Number)obj).longValue());
        else if (obj instanceof String) {
            return Long.valueOf((String)obj);
        }
        ImaMessageFormatRuntimeException jex = new ImaMessageFormatRuntimeException("CWLNC0066", "Long conversion error");
        connect.traceException(2, jex);
        throw jex;
    }

    /*
     * @see javax.jms.JMSProducer#getObjectProperty(java.lang.String)
     */
    public Object getObjectProperty(String name) {
        return msg.getObjectProperty(name);
    }

    /*
     * @see javax.jms.JMSProducer#getPriority()
     */
    public int getPriority() {
        return priority;
    }

    /*
     * @see javax.jms.JMSProducer#getPropertyNames()
     */
    public Set<String> getPropertyNames() {
        Set<String> set = new HashSet<String>();
        if (msg.props != null)
            set.addAll((Set<String>) msg.props.keySet());
        if (retainSet)
            set.add("JMS_IBM_Retain");
        return Collections.unmodifiableSet(set);
    }

    /*
     * @see javax.jms.JMSProducer#getShortProperty(java.lang.String)
     */
    public short getShortProperty(String name) {
        Object obj = msg.getObjectPropertyNoNull(name);
        if (obj instanceof Short)
            return ((Short)obj).shortValue();
        else if (obj instanceof Byte || obj instanceof Short)
            return Short.valueOf(((Number)obj).shortValue());
        else if (obj instanceof String) 
            return Short.valueOf((String)obj);
        ImaMessageFormatRuntimeException jex = new ImaMessageFormatRuntimeException("CWLNC0067", "Short conversion error");
        connect.traceException(2, jex);
        throw jex;
    }

    @Override
    public String getStringProperty(String name) {
        Object prop = msg.getObjectProperty(name);
        if (prop == null)
            return null;
        return prop.toString();
    }

    /*
     * @see javax.jms.JMSProducer#getTimeToLive()
     */
    public long getTimeToLive() {
        return msg.timetolive;
    }

    /*
     * @see javax.jms.JMSProducer#propertyExists(java.lang.String)
     */
    public boolean propertyExists(String name) {
        if (name == null)
            return false;
        if (name.startsWith("JMS_")) {
            if (name.equals("JMS_IBM_Retain") && retainSet) 
                return true;
            return false;
        }      
        return msg.props != null ? msg.props.containsKey(name) : false;
    }

    /*
     * @see javax.jms.JMSProducer#send(javax.jms.Destination, javax.jms.Message)
     */
    public JMSProducer send(Destination dest, Message msg) {
        ImaMessage lmsg;
        
        if (msg == null) { 
            throw new ImaMessageFormatRuntimeException("CWLNC0042", "The message cannot be null");
        }
        
        /*
         * If this is an ImaMessage, use it as it is.
         * Otherwise, create an ImaMessage with its properties and content.
         */
        try {
            if (msg instanceof ImaMessage) {
                lmsg = (ImaMessage)msg;
                if (this.msg.props != null && lmsg.isReadonlyProps) {
                    throw new ImaMessageNotWriteableRuntimeException("CWLNC0000", "The message is not writeable");
                }
            } else {
                if (msg instanceof BytesMessage) {
                    lmsg = (ImaMessage)new ImaBytesMessage((BytesMessage)msg, session);
                } else if (msg instanceof MapMessage) {
                    lmsg = (ImaMessage)new ImaMapMessage((MapMessage)msg, session);
                } else if (msg instanceof ObjectMessage) {
                    lmsg = (ImaMessage)new ImaObjectMessage((ObjectMessage)msg, session);
                } else if (msg instanceof StreamMessage) {
                    lmsg = (ImaMessage)new ImaStreamMessage((StreamMessage)msg, session); 
                } else if (msg instanceof TextMessage) {
                    lmsg = (ImaMessage)new ImaTextMessage((TextMessage)msg, session); 
                } else {
                    lmsg = (ImaMessage)new ImaMessage(session, ImaMessage.MTYPE_Message, msg);
                }
            }    
            ((ImaMessage)lmsg).reset();
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
        
        /*
         * Do the send
         */
        sendX(dest, lmsg);
        
        /* 
         * Set send fields in the original message as well, but ignore this if 
         * there is an error. 
         */
        if (lmsg != msg) {
            try {
                msg.setJMSDeliveryMode(deliverymode);
                msg.setJMSPriority(priority);
                msg.setJMSTimestamp(lmsg.getJMSTimestamp());
                msg.setJMSExpiration(lmsg.getJMSExpiration());
                msg.setJMSDestination(dest); 
                msg.setJMSMessageID(lmsg.getJMSMessageID());
            } catch (JMSException jmse) {
            }
        }
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#send(javax.jms.Destination, java.lang.String)
     */
    public JMSProducer send(Destination dest, String text) {
        ImaTextMessage msg;
        try {
            msg = new ImaTextMessage(session, text);
            msg.reset();
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
        sendX(dest, msg);
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#send(javax.jms.Destination, java.util.Map)
     */
    public JMSProducer send(Destination dest, Map<String, Object> value) {
        ImaMapMessage msg;
        try {
            msg = new ImaMapMessage(session);
            Iterator <String> it = value.keySet().iterator();
            while (it.hasNext()) {
                String prop = it.next();
                msg.setObject(prop, value.get(prop));
            }    
            msg.reset();
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
        sendX(dest, msg);
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#send(javax.jms.Destination, byte[])
     */
    public JMSProducer send(Destination dest, byte[] value) {
        ImaBytesMessage msg;
        try {
            msg = new ImaBytesMessage(session);
            msg.writeBytes(value);
            msg.reset();
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
        sendX(dest, msg);
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#send(javax.jms.Destination, java.io.Serializable)
     */
    public JMSProducer send(Destination dest, Serializable obj) {
        ImaObjectMessage msg;
        try {
            msg = new ImaObjectMessage(session);
            msg.setObject(obj);
            msg.reset();
        } catch (JMSException jex) {
            throw ImaRuntimeException.mapException(jex);
        }
        sendX(dest, msg);
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setAsync(javax.jms.CompletionListener)
     */
    public JMSProducer setAsync(CompletionListener listener) {
        this.listener = listener;
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setDeliveryDelay(long)
     */
    public JMSProducer setDeliveryDelay(long delay) {
        deliveryDelay = delay;
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setDeliveryMode(int)
     */
    public JMSProducer setDeliveryMode(int mode) {
        if (mode != DeliveryMode.NON_PERSISTENT && mode != DeliveryMode.PERSISTENT) {
            ImaJmsRuntimeException jex = new ImaJmsRuntimeException("CWLNC0035", "The value for delivery mode is not correct");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        deliverymode = mode;
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setDisableMessageID(boolean)
     */
    public JMSProducer setDisableMessageID(boolean disable) {
        disableMessageID = disable;
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setDisableMessageTimestamp(boolean)
     */
    public JMSProducer setDisableMessageTimestamp(boolean disable) {
        disableTimestamp = disable;
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setJMSCorrelationID(java.lang.String)
     */
    public JMSProducer setJMSCorrelationID(String cid) {
        msg.correlationID = cid;
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setJMSCorrelationIDAsBytes(byte[])
     */
    public JMSProducer setJMSCorrelationIDAsBytes(byte[] cid) {
        throw new java.lang.UnsupportedOperationException("setJMSCorrelationIDAsBytes is not supported.  Use setJMSCorrelationID.");
    }

    /*
     * @see javax.jms.JMSProducer#setJMSReplyTo(javax.jms.Destination)
     */
    public JMSProducer setJMSReplyTo(Destination replyto) {
        msg.jmsreply = replyto;
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setJMSType(java.lang.String)
     */
    public JMSProducer setJMSType(String typestr) {
        msg.jmstype = typestr;
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setPriority(int)
     */
    public JMSProducer setPriority(int priority) {
        if (priority < 0 || priority > 9) {
            ImaJmsRuntimeException jex = new ImaJmsRuntimeException("CWLNC0036", "The JMSPriority value is not valid.  It must be in the range 0 to 9 and is {0}", priority);
            connect.traceException(2, jex);
            throw jex;
        }
        this.priority = priority;
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setProperty(java.lang.String, boolean)
     */
    public JMSProducer setProperty(String name, boolean value) {
        setObjectProperty(msg, name, value);
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setProperty(java.lang.String, byte)
     */
    public JMSProducer setProperty(String name, byte value) {
        setObjectProperty(msg, name, value);
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setProperty(java.lang.String, short)
     */
    public JMSProducer setProperty(String name, short value) {
        setObjectProperty(msg, name, value);
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setProperty(java.lang.String, int)
     */
    public JMSProducer setProperty(String name, int value) {
        setObjectProperty(msg, name, value);
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setProperty(java.lang.String, long)
     */
    public JMSProducer setProperty(String name, long value) {
        setObjectProperty(msg, name, value);
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setProperty(java.lang.String, float)
     */
    public JMSProducer setProperty(String name, float value) {
        setObjectProperty(msg, name, value);
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setProperty(java.lang.String, double)
     */
    public JMSProducer setProperty(String name, double value) {
        setObjectProperty(msg, name, value);
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setProperty(java.lang.String, java.lang.String)
     */
    public JMSProducer setProperty(String name, String value) {
        setObjectProperty(msg, name, value);
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setProperty(java.lang.String, java.lang.Object)
     */
    public JMSProducer setProperty(String name, Object obj) {
        setObjectProperty(msg, name, obj);
        return this;
    }

    /*
     * @see javax.jms.JMSProducer#setTimeToLive(long)
     */
    public JMSProducer setTimeToLive(long ttl) {
        msg.timetolive = ttl;
        return this;
    }

    /* 
     * Set an object property.
     * 
     * @see javax.jms.Message#setObjectProperty(java.lang.String, java.lang.Object)
     */
    public void setObjectProperty(ImaMessage msg, String name, Object value) {
        if (msg.isReadonlyProps) {
            ImaMessageNotWriteableRuntimeException jex = 
                new ImaMessageNotWriteableRuntimeException("CWLNC0038", "The message is not writable");
            connect.traceException(2, jex);
            throw jex;
        }
        if (!ImaMessage.isValidName(name)) {
            if (name==null || name.length()==0) {
                IllegalArgumentException iex = new ImaIllegalArgumentException("CWLNC0082", "The item name cannot be null or a zero length string.");
                connect.traceException(2, iex);
                throw iex;
            }
            ImaMessageFormatRuntimeException jex = new ImaMessageFormatRuntimeException("CWLNC0037", "The property name is not valid: {0}", name);
            connect.traceException(2, jex);
            throw jex;
        }
        if (msg.props == null)
            msg.props = new HashMap<String,Object>();
        if (value == null ||
            value instanceof String  ||
            value instanceof Integer ||
            value instanceof Boolean ||
            value instanceof Byte    ||
            value instanceof Long    ||
            value instanceof Double  ||
            value instanceof Float   ||
            value instanceof Short ) {

            if (name.startsWith("JMS_")) {
                if (name.equals("JMS_IBM_Retain")) {
                    if (value instanceof Integer)
                        msg.retain = ((Integer)value).intValue() == 1;
                    else if (value instanceof Byte)
                        msg.retain = ((Byte)value).intValue() == 1;
                    else if (value instanceof Short)
                        msg.retain = ((Short)value).intValue() == 1;
                    else 
                        msg.retain = false;
                    return;
                }
                retainSet = true;
                return;    /* Ignore all other props starting JMS_ */
            } 
            msg.props.put(name, value);
        } else {
            ImaMessageFormatRuntimeException jex = new ImaMessageFormatRuntimeException("CWLNC0034", "The object class is not supported: {0}", value.getClass());
            connect.traceException(2, jex);
            throw jex;
        }
    }
    
    /*
     * Check if the destination is valid
     */
    void checkDestination(Destination dest) {
        if (dest == null) {
            ImaInvalidDestinationRuntimeException jex = 
                new ImaInvalidDestinationRuntimeException("CWLNC0012", "The destination must be specified");
            connect.traceException(2, jex);
            throw jex;
        }
        if (!(dest instanceof ImaDestination)) {
            ImaInvalidDestinationRuntimeException jex = 
                new ImaInvalidDestinationRuntimeException("CWLNC0013", "The destination must be an IBM MessageSight destination: {0}", dest);
            connect.traceException(2, jex);
            throw jex;
        }
        String destname = ((ImaDestination)dest).getName();
        if (destname == null) {
            ImaInvalidDestinationRuntimeException jex = 
                new ImaInvalidDestinationRuntimeException("CWLNC0007", "The destination does not have a name");
            connect.traceException(2, jex);
            throw jex;
        }
    }

    /*
     * Common method to send a message with async message handling.
     * 
     * This is a preliminary implementation of async messaging which matches the letter
     * but not the spirit as it does a synchronous send and then does the notify.
     * 
     * We need to handle the case where there is not a fully formed message so that
     * we do have a message to send to the completion listener.
     * TODO: a real implementation
     * 
     * @param dest  The destination object to send to.
     * @param lmsg  The message containing header and properties
     * @param body  The body of the message
     */
    public void sendX(Destination dest, ImaMessage lmsg) {
        CompletionListener listener = this.listener;
        /*
         * Update system and user properties from the producer
         */
        if (this.msg.jmsreply != null)
            lmsg.jmsreply = this.msg.jmsreply;
        if (this.msg.jmstype != null)
            lmsg.jmstype = this.msg.jmstype;
        if (this.msg.correlationID != null)
            lmsg.correlationID = this.msg.correlationID;
        if (this.msg.props != null)
            lmsg.props.putAll(this.msg.props);
        
        if (listener != null) {
            try {
                sendMessage(dest, lmsg);
                lmsg.reset();
                ImaConnection.currentConnection.set(connect);
                listener.onCompletion(lmsg);
                ImaConnection.currentConnection.set(null);
            } catch (Exception ex) {
                listener.onException(lmsg, ex);
            }
        } else {
            sendMessage(dest, lmsg);
        }
    }

    /*
     * Common method to send a message.
     * 
     * @param dest  The destination object to send to.
     * @param lmsg  The message containing header and properties
     * @param body  The body of the message
     */
    public void sendMessage(Destination dest, ImaMessage lmsg) {
        ImaProducerAction lmsgAction = null;
        int domain;
        
        checkClosed();
        checkDestination(dest);
        
        /*
         * As we modify the input message, we must make sure that we lock the message from when 
         * the header fields are modified, until we build the JMS header.  Of course if the
         * application is sending the message from multiple threads, it cannot look at the
         * results of the send.
         */
        synchronized (lmsg) {
            /*
             * Complete the message
             */
            long ts;
            boolean  expireSet = false;
            
            lmsg.priority = priority;
            lmsg.deliverymode = deliverymode;
            if (!disableTimestamp) {
                ts = System.currentTimeMillis();
                lmsg.jmstime = ts;
                if (ttl > 0) {
                    lmsg.setJMSExpiration2(ts + ttl);
                    expireSet = true;
                }    
                if (deliveryDelay > 0) {
                    lmsg.deliveryTime = ts + deliveryDelay;
                    expireSet = true;
                }
            } else {
                ts = 0;
                lmsg.jmstime = 0;
                lmsg.setJMSExpiration2(0);
            }
            lmsg.jmsdestination = dest;
            if (!disableMessageID) {
                lmsg.messageID = makeMsgID();    
            } else {
                lmsg.messageID = null;
            }
            
        
            /*
             * The JMS specification requires that this be called only in a single thread,
             * and therefore we should not need to lock, and indeed we have not locked the
             * previous work here.  However, since this code should be synchronized, if
             * should not cost us much to do so here, and it will prevent almost all
             * consequences of violating the JMS session threading model.
             */
            synchronized (session.producer_lock) {
                final int actionType;
                final boolean wait;

                if ((lmsg.deliverymode == DeliveryMode.PERSISTENT) ||
                        (session.transacted && !session.asyncSendAllowed) ||
                         listener != null ) {      /* Temp until we implement async for real */
                    actionType = ImaAction.messageNoProdWait;
                    wait = true;
                } else {
                    actionType = ImaAction.messageNoProd;
                    wait = false;
                }
                domain = ((ImaPropertiesImpl) dest).getString("ObjectType").equals("topic") ? ImaJms.Topic : ImaJms.Queue;
                String destname = ((ImaDestination)dest).getName();
                try {
                    if (sendMsgAction == null) {
                        sendMsgAction = new ImaProducerAction(actionType, session, domain, destname);
                    } else {
                        sendMsgAction.reset(actionType, domain, destname);
                    }    
                    if (connect.isloopbackclient)
                        sendMsgAction.senddestname = destname;
                    lmsgAction = sendMsgAction;
                    
                    lmsgAction.setMessageHdr(lmsg.msgtype, deliverymode, priority, this.session.disableACK, 
                            expireSet, msg.retain); 
                    
                    lmsgAction.outBB = ImaUtils.putMapValue(lmsgAction.outBB, lmsg.props, lmsg, dest); 
                    lmsgAction.request(lmsg.body, wait|connect.isloopbackclient);
                } catch (JMSException jex) {
                    throw ImaRuntimeException.mapException(jex);
                }
                if (wait && (lmsgAction.rc != 0)) {
                    switch(lmsgAction.rc) {               
                    case ImaReturnCode.IMARC_MsgTooBig:
                        ImaJmsRuntimeException jex = new ImaJmsRuntimeException("CWLNC0215", "Message for {0} {1} from producer {2} is too big for {3}", 
                                (domain == ImaJms.Topic)?"topic":"queue", destname,
                                connect.client.host+":"+connect.client.port);
                        connect.traceException(2, jex);
                        throw jex;
                    case ImaReturnCode.IMARC_DestinationFull:
                        ImaJmsRuntimeException jex2 = new ImaJmsRuntimeException("CWLNC0218", "Failed to send message for {0} {1} from producer {2}. Destination is full.",
                                (domain == ImaJms.Topic)?"topic":"queue", destname, 0);
                        connect.traceException(2, jex2);
                        throw jex2;
                    case ImaReturnCode.IMARC_DestNotValid:
                        ImaInvalidDestinationRuntimeException jex3 = new ImaInvalidDestinationRuntimeException("CWLNC0219", "Failed to send message for {0} {1} from producer {2}.  Destination is not valid.",
                                (domain == ImaJms.Topic)?"topic":"queue", destname, 0);
                        connect.traceException(2, jex3);
                        throw jex3;
                    case ImaReturnCode.IMARC_BadSysTopic:
                        ImaJmsRuntimeException jex6 = new ImaJmsRuntimeException("CWLNC0225", "Failed to send message for Topic{0} from producer {1}.  The topic is a system topic.",
                                destname, 0);
                        connect.traceException(2, jex6);
                        throw jex6;
                    case ImaReturnCode.IMARC_NotAuthorized:
                    case ImaReturnCode.IMARC_NotAuthenticated: 
                        ImaJmsSecurityRuntimeException jex7 = new ImaJmsSecurityRuntimeException("CWLNC0207", "Authorization failed.");
                        connect.traceException(2, jex7);
                        throw jex7;
                    case ImaReturnCode.IMARC_ServerCapacity:
                        ImaJmsRuntimeException jex5 = new ImaJmsRuntimeException("CWLNC0223", "Failed to send message for {0} {1} from producer {2}.  Server capacity exceeded.",
                                (domain == ImaJms.Topic)?"topic":"queue", destname, 0, lmsgAction.rc);
                        connect.traceException(2, jex5);
                        throw jex5;
                    default:
                        ImaJmsRuntimeException jex4 = new ImaJmsRuntimeException("CWLNC0216", "Failed to send message for {0} {1} from producer {2}. Unknown failure - rc={3}.", 
                                (domain == ImaJms.Topic)?"topic":"queue", destname, 0, lmsgAction.rc);
                        connect.traceException(2, jex4);
                        throw jex4;
                    }
                }
            }   
        }
    }
    
    /*
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return toString(ImaConstants.TS_Common);
    }
    
    /*
     * Class name for toString 
     */
    public String getClassName() {
        return "ImaJMSProducer";
    }
}
