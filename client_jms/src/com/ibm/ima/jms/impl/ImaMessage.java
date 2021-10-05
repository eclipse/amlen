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
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.Map.Entry;

import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;


/**
 * Implement the Message interface for the IBM MessageSight JMS client.
 *
 * This class is the super class for all JMS message types, and implements directly the
 * message without a body. All header and property fields are handled by this class.
 * <p>
 * 
 */
public class ImaMessage implements Message, ImaConstants {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


	static final String		ACK_SQN_PROP_NAME = "JMS_IBM_ACK_SQN";
    static final ThreadLocal < byte[] > tempByte = 
        new ThreadLocal < byte[] > () {
            protected byte[] initialValue() {
                return new byte[1000];
        }
    };
    static final ThreadLocal < char[] > tempChar = 
        new ThreadLocal < char[] > () {
            protected char[] initialValue() {
                return new char[1000];
        }
    };
    
    static final SimpleDateFormat iso8601 = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ");
    /*
     * Message types
     */ 
    static final int  MTYPE_Unknown       = 0;    
    static final int  MTYPE_Message       = 1;
    static final int  MTYPE_BytesMessage  = 2;
    static final int  MTYPE_MapMessage    = 3;
    static final int  MTYPE_ObjectMessage = 4;
    static final int  MTYPE_StreamMessage = 5;
    static final int  MTYPE_TextMessage   = 6;
    
    /*
     * Class variables
     */
    boolean  isReadonlyProps;        /* The message properties cannot be written        */
    HashMap  <String, Object> props; /* The message properties as a hashmap             */
    ImaMessageConsumer  consumer;
    
    /*
     * JMS header fields
     */
    int         version = 1;           
    int         deliverymode = 0;      /* The delivery mode (0=unknown, 1=persist, 2=non_persist */
    long        timetolive = 0;        /* The time to live */
    long        expiration = 0;        /* The expiration time (0=no expiration) */
    int         priority = 4;          /* The priority setting (0-9)  */
    boolean     retain = false;
    String      messageID = null;      /* The generated message ID */
    String      jmstype   = null;      /* The JMS type string */
    long        jmstime   = 0;         /* The JMS timestamp (millis since 1970-01-01T0) */
    Destination jmsreply = null;       /* The reply destination */
    Destination jmsdestination = null; /* The destination */
    String      correlationID = null;  /* The correlation ID */
    int         msgtype;               /* The internal message type */
    int         deliveryCount = 0;     /* The delivery count */
    String      jmsdestname = null;    /* Used to create jmsdestination for consumers */
    ImaSession  session;
    boolean     suspend = false;
    long        ack_sqn = 0;
    long        deliveryTime = 0;
    protected boolean isReadonly = false;
    protected   ByteBuffer    body; 
    /*
     * Constructor for a message without a body.
     * This class is both the superclass of all messages, and implements the message without a body. 
     */
    public ImaMessage(ImaSession session) {
        this(session, MTYPE_Message);
    }
    
    
    /*
     * Constructor for the superclass of all messages. 
     */
    public ImaMessage(ImaSession session, int mtype) {
        this.session = session;
        msgtype = mtype;
        if (session != null) {
            priority = session.defaultPriority;
            timetolive = session.defaultTimeToLive;
            expiration = 0;
        }
    }
    
  
    /*
     * Constructor for the superclass of all messages created from an existing message.
     * This is done to support the conversion of foreign messages to ImaMessage. 
     */
    public ImaMessage(ImaSession session, int mtype, Message msg) throws JMSException {
        this.session = session;
        msgtype = mtype;
        timetolive = session.defaultTimeToLive;
        expiration = 0;
        copyProperties(msg);
        copyJMSHeader(msg);
    }
    
    /*
     * Clone constructor for a received message
     */
    public ImaMessage(ImaMessage msg) {
        session        = msg.session;
        isReadonlyProps= true;
        props          = msg.props;                 /* The message properties as a hashmap             */
        deliverymode   = msg.deliverymode;          /* The delivery mode (0=unknown, 1=persist, 2=non_persist */
        timetolive     = msg.timetolive;            /* The time to live                                */
        expiration     = msg.expiration;            /* The expiration time (0=no expiration)           */
        priority       = msg.priority;              /* The priority setting (0-9)                      */
        deliveryCount  = msg.deliveryCount;         /* Is this message redelivered                     */
        messageID      = msg.messageID;             /* The generated message ID                        */
        jmstype        = msg.jmstype;               /* The JMS type string                             */
        jmstime        = msg.jmstime;               /* The JMS timestamp (millis since 1970-01-01T0)   */
        jmsreply       = msg.jmsreply;              /* The reply destination                           */
        jmsdestination = msg.jmsdestination;        /* The destination                                 */
        correlationID  = msg.correlationID;         /* The correlation ID                              */
        msgtype        = msg.msgtype;               /* The internal message type                       */
        deliveryTime   = msg.deliveryTime;          /* To delayed delivery time                        */
        if (msg.body != null)
            body           = msg.body.duplicate();      /* The body of the message which can be null       */
        isReadonly     = true;                      /* The message body cannot be written              */
    }
    
    /*
     * Clone a message as a readonly message
     */
    ImaMessage cloneMessage() {
        switch (msgtype) {
        default:    
        case MTYPE_Message:             return new ImaMessage(this);
        case MTYPE_BytesMessage:        return new ImaBytesMessage(this);
        case MTYPE_MapMessage:          return new ImaMapMessage(this);
        case MTYPE_ObjectMessage:       return new ImaObjectMessage(this);
        case MTYPE_StreamMessage:       return new ImaStreamMessage(this);
        case MTYPE_TextMessage:         return new ImaTextMessage(this);
        }
    }
    
    
    /*
     * Copy all named properties. 
     * This is done as part of converting foreign messages.
     */
    void copyProperties(Message msg) throws JMSException {
        Enumeration<?> prp = msg.getPropertyNames();
        while (prp.hasMoreElements()) {
            String prop = String.valueOf(prp.nextElement());
            /* Ignore JMS_ properties from non-IMA messages */
            if (msg instanceof ImaMessage || !(prop.startsWith("JMS_"))) {
                Object obj = msg.getObjectProperty(prop);
                setObjectProperty(prop, obj);
            }    
        }
    }

    
    /*
     * Copy all fields from the JMS header.
     */
    void copyJMSHeader(Message msg) throws JMSException {
        this.deliverymode = msg.getJMSDeliveryMode();
        this.correlationID = msg.getJMSCorrelationID();
        this.messageID = msg.getJMSMessageID();
        this.jmstype = msg.getJMSType();
        /* Reply is kept only for IMA Messages */
        if (msg instanceof ImaMessage) {
            this.jmsreply = msg.getJMSReplyTo();
            this.jmsdestination = msg.getJMSDestination();
            /* The other provider might not be JMS 2.0 */
            this.deliveryTime = ((ImaMessage)msg).getJMSDeliveryTime();
        }    
    }

    /*
     * Create a message from an action with a buffer
     */
    static ImaMessage makeMessage(ByteBuffer msgBody, ImaSession session, ImaConsumer consumer) throws JMSException {
        ImaMessage msg;
        
        try {
            synchronized(msgBody) {   /* TODO: why */
                int msgtype = msgBody.get(ImaAction.BODY_TYPE_POS);
                switch (msgtype) {
                case MTYPE_Message:         msg = new ImaMessage(session);              break;
                case MTYPE_BytesMessage:    msg = new ImaBytesMessage(session);         break;
                case MTYPE_MapMessage:      msg = new ImaMapMessage(session);           break;
                case MTYPE_ObjectMessage:   msg = new ImaObjectMessage(session);         break;
                case MTYPE_StreamMessage:   msg = new ImaStreamMessage(session);        break;
                case MTYPE_TextMessage:     msg = new ImaTextMessage(session, null);    break;
                default:
                	ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0024", "The client failed to process a message received from MessageSight because the message type ({0}) is unknown.", msgtype);
                    ImaTrace.traceException(2, jex);
                    throw jex;
                }
                
                byte flags = msgBody.get(ImaAction.FLAGS_POS);
                if ((flags & 0x10) != 0) {
                    msg.suspend = true;
                }
                msg.ack_sqn = msgBody.getLong(ImaAction.MSG_ID_POS);
                msg.deliverymode = (flags & 0x80) == 0 ? DeliveryMode.NON_PERSISTENT  : DeliveryMode.PERSISTENT;
                msg.retain = (flags & 0x40) != 0;
                msg.priority = msgBody.get(ImaAction.PRIORITY_POS);
                msg.deliveryCount = msgBody.get(ImaAction.DUP_POS) + 1;
                msgBody.position(ImaAction.DATA_POS);
                msg.jmsdestination = null;
                msg.jmsdestname = null;
                msg.props = (HashMap<String,Object>)ImaUtils.getMapValue(msgBody, msg);
                if (msg.jmsdestname == null)
                	msg.jmsdestination = consumer.getDest();
                
                int otype = ImaUtils.getObjectType(msgBody);
                if (otype != S_Null) {
                    msg.body = ImaUtils.getByteBufferValue(msgBody,otype);
                }    
                if (msgBody.position() != msgBody.limit()) {
                	 if (ImaTrace.isTraceable(2)) {
                		 ImaTrace.trace("Invalid message received: session=" + session.toString(ImaConstants.TS_All) +
                             " position=" + msgBody.position() + " limit=" + msgBody.limit() +
                             " sqn=" + msg.ack_sqn + " otype=" + otype);
                     }
                	 ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0025", "The client failed to parse a message received from MessageSight because the message is not valid.");
                	 ImaTrace.traceException(2, jex);
                	 throw jex;
                }
                msg.initContent();
            }
        } catch (Exception e) {
        	if ((e instanceof ImaJmsExceptionImpl))
        		throw (ImaJmsExceptionImpl)e;
        	ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0025", e, "Message received is not valid");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        msg.isReadonly = true;
        return msg;
    }
    
    
    /*
     * Reset the content.  
     * Do nothing in a base class, but can be subclassed
     */
    void reset() throws JMSException {
    }
    
    /*
     * Initialize the content.
     * This is called when the content is initialized.  If setting body and bodylen
     * are enough, then no further action is required.
     * Do nothing in the base class.    
     */
    void initContent() throws JMSException {
        isReadonlyProps = true;
    }
    
    /* 
     * User acknowledge.
     * @see javax.jms.Message#acknowledge()
     */
    public void acknowledge() throws JMSException { 
    	/*
    	 * Note about disableACK.  Just as when acknowledge is called on a transacted session, acknowledge called on
    	 * a disableACK session will have no affect.  We considered throwing an exception for this but have decided
    	 * against it.
    	 */
    	
        session.acknowledge(false);
    }

    
    /*
     * Clear the body.
     * This is subclassed by some message types. 
     * @see javax.jms.Message#clearBody()
     */
    public void clearBody() throws JMSException {
        if (body != null)
            body.clear();
        isReadonly = false;
    }

    
    /* 
     * Clear the properties and allow properties to be written.
     * @see javax.jms.Message#clearProperties()
     */
    public void clearProperties() throws JMSException {
        if (props != null)
            props = null;
        isReadonlyProps = false;
    }

    
    /* 
     * Get a boolean property.
     * 
     * @see javax.jms.Message#getBooleanProperty(java.lang.String)
     */
    public boolean getBooleanProperty(String name) throws JMSException {
        Object obj = getObjectProperty(name);
        if (obj == null)
            return false;                /* Required by TCK, not specified in spec */
        if (obj instanceof Boolean)
            return ((Boolean)obj).booleanValue();
        else if (obj instanceof String) 
            return Boolean.valueOf((String)obj);
        ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0061", "A method call to retrieve a Boolean from a Message object failed because the value is not Boolean and cannot be successfully converted to Boolean.");
        ImaTrace.traceException(2, jex);
        throw jex;
    }

    
    /* 
     * Get a byte property.
     * 
     * @see javax.jms.Message#getByteProperty(java.lang.String)
     */
    public byte getByteProperty(String name) throws JMSException {
        Object obj = getObjectPropertyNoNull(name);
        if (obj instanceof Byte)
            return ((Byte)obj).byteValue();
        else if (obj instanceof String) 
            return Byte.valueOf((String)obj);
        ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0062", "A method call to retrieve a Byte from a Message object failed because the value is not of type Byte and cannot be successfully converted to Byte.");
        ImaTrace.traceException(2, jex);
        throw jex;
    }

    
    /* 
     * Get a double property.
     * 
     * @see javax.jms.Message#getDoubleProperty(java.lang.String)
     */
    public double getDoubleProperty(String name) throws JMSException {
        Object obj = getObjectProperty(name);
        if (obj == null) {
        	NullPointerException nex = new ImaNullPointerException("CWLNC0053","A method call to retrieve a message property failed because the specified property ({0}) does not exist on the message.",name);
            ImaTrace.traceException(2, nex);
            throw nex;
        }
        if (obj instanceof Double)
            return ((Double)obj).doubleValue();
        else if (obj instanceof Float)
            return ((Float)obj).doubleValue();
        else if (obj instanceof String) 
            return Double.valueOf((String)obj);
        ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0063", "A method call to retrieve a Double from a Message object failed because the value is not of type Double and cannot be successfully converted to Double.");
        ImaTrace.traceException(2, jex);
        throw jex;
    }

    
    /* 
     * Get a float property
     * 
     * @see javax.jms.Message#getFloatProperty(java.lang.String)
     */
    public float getFloatProperty(String name) throws JMSException {
        Object obj = getObjectProperty(name);
        if (obj == null) {
        	NullPointerException nex = new ImaNullPointerException("CWLNC0053","A method call to retrieve a message property failed because the specified property ({0}) does not exist on the message.", name);
            ImaTrace.traceException(2, nex);
            throw nex;
        }
        if (obj instanceof Float)
            return ((Float)obj).floatValue();
        else if (obj instanceof String) 
            return Float.valueOf((String)obj);
        ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0065", "A method call to retrieve a Float from a Message object failed because the value is not of type Float and cannot be successfully converted to Float.");
        ImaTrace.traceException(2, jex);
        throw jex;
    }

    
    /* 
     * Get an integer property.
     * 
     * @see javax.jms.Message#getIntProperty(java.lang.String)
     */
    public int getIntProperty(String name) throws JMSException {
        Object obj = getObjectPropertyNoNull(name);
        if (obj instanceof Integer)
            return ((Integer)obj).intValue();
        else if (obj instanceof Byte || obj instanceof Short)
            return Integer.valueOf(((Number)obj).intValue());
        else if (obj instanceof String) 
            return Integer.valueOf((String)obj);
        ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0064", "A method call to retrieve an Integer from a Message object failed because the value is not of type Integer and cannot be successfully converted to Integer.");
        ImaTrace.traceException(2, jex);
        throw jex;
    }

    
    /* 
     * Get the JMS correlation ID.
     * 
     * @see javax.jms.Message#getJMSCorrelationID()
     */
    public String getJMSCorrelationID() throws JMSException {
        return correlationID;
    }

    
    /* 
     * Get the JMS correlation ID as a byte array.
     * 
     * @see javax.jms.Message#getJMSCorrelationIDAsBytes()
     */
    public byte[] getJMSCorrelationIDAsBytes() throws JMSException {
        throw new ImaUnsupportedOperationException("CWLNC0052", "A call to method {0} failed because the method is not supported.  Use {1}.", "getJMSCorrelationIDAsBytes", "getJMSCorrelationID");
    }

    
    /* 
     * Get the JMS delivery mode.
     * 
     * @see javax.jms.Message#getJMSDeliveryMode()
     */
    public int getJMSDeliveryMode() throws JMSException {
        return deliverymode;
    }

    
    /* 
     * Get the JMS destination.
     * @see javax.jms.Message#getJMSDestination()
     */
    public Destination getJMSDestination() throws JMSException {
        if (jmsdestination == null) {
            if (jmsdestname != null && jmsdestname.length() > 0) {
                jmsdestination = new ImaTopic(jmsdestname);
            }
        }
        return jmsdestination;
    }

    
    /* 
     * Get the JMS expiration time.
     * 
     * @see javax.jms.Message#getJMSExpiration()
     */
    public long getJMSExpiration() throws JMSException {
        return expiration;
    }

    
    /* 
     * Get the JMS message ID.
     * 
     * @see javax.jms.Message#getJMSMessageID()
     */
    public String getJMSMessageID() throws JMSException {
        return messageID;
    }

    
    /* 
     * Get the JMS priority.
     * 
     * @see javax.jms.Message#getJMSPriority()
     */
    public int getJMSPriority() throws JMSException {
        return priority;
    }

    
    /* 
     * Get the JMS redelivered property.
     * 
     * @see javax.jms.Message#getJMSRedelivered()
     */
    public boolean getJMSRedelivered() throws JMSException {
        return deliveryCount > 1;
    }

    
    /* 
     * Get the JMS reply to property.
     * 
     * @see javax.jms.Message#getJMSReplyTo()
     */
    public Destination getJMSReplyTo() throws JMSException {
        return jmsreply;
    }

    
    /* 
     * Get the JMS timestamp property.
     * 
     * @see javax.jms.Message#getJMSTimestamp()
     */
    public long getJMSTimestamp() throws JMSException {
        return jmstime;
    }

    
    /* 
     * Get the JMS type property.
     * 
     * @see javax.jms.Message#getJMSType()
     */
    public String getJMSType() throws JMSException {
        return jmstype;
    }

    
    /* 
     * Get a long property.
     * 
     * @see javax.jms.Message#getLongProperty(java.lang.String)
     */
    public long getLongProperty(String name) throws JMSException {
        Object obj = getObjectPropertyNoNull(name);
        if (obj instanceof Long)
            return ((Long)obj).longValue();
        else if (obj instanceof Integer || obj instanceof Byte || obj instanceof Short)
            return Long.valueOf(((Number)obj).longValue());
        else if (obj instanceof String) {
            return Long.valueOf((String)obj);
        }
        ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0066", "A method call to retrieve a Long from a Message object failed because the value is not of type Long and cannot be successfully converted to Long.");
        ImaTrace.traceException(2, jex);
        throw jex;
    }

    
    /* 
     * Get an object property.
     * 
     * @see javax.jms.Message#getObjectProperty(java.lang.String)
     */
    public final Object getObjectProperty(String name) {
        if (name == null)
            return null;
        if (name.startsWith("JMS")) {
            if (name.charAt(3) == '_') {
            	if (name.equals("JMS_IBM_ACK_SQN")) {
        		   if (ack_sqn != 0)
        		       return Long.valueOf(ack_sqn);
            	}
                if (name.equals("JMS_IBM_Retain")) {
                    return retain ? Integer.valueOf(1) : Integer.valueOf(0);
                }
                return null;
            }
            if (name.charAt(3) == 'X') {
                if (name.equals("JMSXDeliveryCount")) {
                    return deliveryCount;
                }    
            }
        } 
        return props != null ? props.get(name) : null;
    }

    
    /*
     * Get an object property.
     * 
     * @see com.ibm.msg.client.jms.JmsReadablePropertyContext#getObjectProperty(java.lang.String)
     */
    Object getObjectPropertyNoNull(String name) {
        Object obj = getObjectProperty(name);
        if (obj == null) {
        	NumberFormatException nex = new ImaNumberFormatException("CWLNC0053","A method call to retrieve a message property failed because the specified property ({0}) does not exist on the message.", name);
            ImaTrace.traceException(2, nex);
            throw nex;
        }
        return obj;
    }   
      
    
    /* 
     * Get an enumeration of property names.
     *  
     * @see javax.jms.Message#getPropertyNames()
     */
    public Enumeration <?> getPropertyNames() {
        HashSet<String> set = new HashSet<String>();
        if (props != null)
            set.addAll((Set<String>) props.keySet());
    	set.add("JMS_IBM_Retain");
    	if (ack_sqn != 0)
            set.add("JMS_IBM_ACK_SQN");
        set.add("JMSXDeliveryCount"); 
    	return new SetEnumeration(set);
    }

    
    /* 
     * Get a short property.
     * 
     * @see javax.jms.Message#getShortProperty(java.lang.String)
     */
    public short getShortProperty(String name) throws JMSException {
        Object obj = getObjectPropertyNoNull(name);
        if (obj instanceof Short)
            return ((Short)obj).shortValue();
        else if (obj instanceof Byte || obj instanceof Short)
            return Short.valueOf(((Number)obj).shortValue());
        else if (obj instanceof String) 
            return Short.valueOf((String)obj);
        ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0067", "A method call to retrieve a Short from a Message object failed because the value is not of type Short and cannot be successfully converted to Short.");
        ImaTrace.traceException(2, jex);
        throw jex;
    }

    
    /* 
     * Get a string property.
     * 
     * @see javax.jms.Message#getStringProperty(java.lang.String)
     */
    public String getStringProperty(String name) throws JMSException {
        Object prop = getObjectProperty(name);
        if (prop == null)
            return null;
        return prop.toString();
    }

    
    /* 
     * Check if a property exists.
     * 
     * @see javax.jms.Message#propertyExists(java.lang.String)
     */
    public boolean propertyExists(String name) throws JMSException {
        if (name == null)
            return false;
        if (name.startsWith("JMS")) {
            if (name.charAt(3) == '_') {
                if (name.equals("JMS_IBM_Retain")) {
                    return true;
                }
        	    if (name.equals("JMS_IBM_ACK_SQN")) {
        	        return ack_sqn != 0;
        	    }
        	    return false;
            }
            if (name.charAt(3) == 'X') {
                if (name.equals("JMSXDeliveryCount")) {
                    return true;
                }    
            }
        }        
    	return props != null ? props.containsKey(name) : false;
    }

    
    /* 
     * Set a boolean property.
     * 
     * @see javax.jms.Message#setBooleanProperty(java.lang.String, boolean)
     */
    public void setBooleanProperty(String name, boolean value) throws JMSException {
        setObjectProperty(name, Boolean.valueOf(value));
    }

    
    /* 
     * Set a byte property.
     * 
     * @see javax.jms.Message#setByteProperty(java.lang.String, byte)
     */
    public void setByteProperty(String name, byte value) throws JMSException {
        setObjectProperty(name, Byte.valueOf(value));
    }

    
    /* 
     * Set a float property.
     * 
     * @see javax.jms.Message#setDoubleProperty(java.lang.String, double)
     */
    public void setDoubleProperty(String name, double value) throws JMSException {
        setObjectProperty(name, Double.valueOf(value));
    }

    
    /* 
     * Set a float property.
     * 
     * @see javax.jms.Message#setFloatProperty(java.lang.String, float)
     */
    public void setFloatProperty(String name, float value) throws JMSException {
        setObjectProperty(name, Float.valueOf(value));
    }

    
    /* 
     * Set an integer property.
     * 
     * @see javax.jms.Message#setIntProperty(java.lang.String, int)
     */
    public void setIntProperty(String name, int value) throws JMSException {
        setObjectProperty(name, Integer.valueOf(value));
    }

    
    /* 
     * Set the JMS correlation ID.
     * 
     * @see javax.jms.Message#setJMSCorrelationID(java.lang.String)
     */
    public void setJMSCorrelationID(String arg0) throws JMSException {
        correlationID = arg0;
    }

    
    /* 
     * Set the JMS correlation ID as bytes.
     * 
     * @see javax.jms.Message#setJMSCorrelationIDAsBytes(byte[])
     */
    public void setJMSCorrelationIDAsBytes(byte[] arg0) throws JMSException {
        throw new ImaUnsupportedOperationException("CWLNC0052", "A call to method {0} failed because the method is not supported.  Use {1}.", "setJMSCorrelationIDAsBytes", "setJMSCorrelationID");
    }

    
    /* 
     * Set the JMS delivery mode.
     * 
     * @see javax.jms.Message#setJMSDeliveryMode(int)
     */
    public void setJMSDeliveryMode(int deliveryMode) throws JMSException {
        this.deliverymode = deliveryMode;
    }

    
    /* 
     * Set the JMS destination.
     * 
     * @see javax.jms.Message#setJMSDestination(javax.jms.Destination)
     */
    public void setJMSDestination(Destination dest) throws JMSException {
        jmsdestination = dest;
    }

    
    /* 
     * Set the JMS Expiration.
     * 
     * @see javax.jms.Message#setJMSExpiration(long)
     */
    public void setJMSExpiration(long expire) throws JMSException {
        expiration = expire; 
        timetolive = -1;
    }
    
    public void setJMSExpiration2(long expire) {
        expiration = expire; 
        timetolive = -1;
    }

    
    /* 
     * Set the JMS message ID.
     * 
     * @see javax.jms.Message#setJMSMessageID(java.lang.String)
     */
    public void setJMSMessageID(String msgid) throws JMSException {
        messageID = msgid;
    }

    
    /* 
     * Set the JMS priority.
     * 
     * @see javax.jms.Message#setJMSPriority(int)
     */
    public void setJMSPriority(int priority) throws JMSException {
        if (priority < 0 || priority > 9) {
        	ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0036", "A call to the setPriority() method on a Message object or on a MessageProducer object failed because the input value is not a valid priority ({0}).  Valid priority values are integers in the range 0 to 9.", priority);
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        this.priority = priority;
    }

    
    /* 
     * Set the JMS redelivered flag.
     * 
     * @see javax.jms.Message#setJMSRedelivered(boolean)
     */
    public void setJMSRedelivered(boolean redelivered) throws JMSException {
        deliveryCount = 2;
    }

    
    /* 
     * Set the JMS reply to string.
     * 
     * @see javax.jms.Message#setJMSReplyTo(javax.jms.Destination)
     */
    public void setJMSReplyTo(Destination replyto) throws JMSException {
        jmsreply = replyto;
    }

    
    /* 
     * Set the JMS timestamp.
     * 
     * @see javax.jms.Message#setJMSTimestamp(long)
     */
    public void setJMSTimestamp(long timestamp) throws JMSException {
        jmstime = timestamp;
    }

    
    /* 
     * Set the JMS type string.
     * 
     * @see javax.jms.Message#setJMSType(java.lang.String)
     */
    public void setJMSType(String typestr) throws JMSException {
        jmstype = typestr;
    }

    
    /* 
     * Set an long property.
     * 
     * @see javax.jms.Message#setLongProperty(java.lang.String, long)
     */
    public void setLongProperty(String name, long value) throws JMSException {
        setObjectProperty(name, value);
    }

    /* 
     * Set an object property.
     * 
     * @see javax.jms.Message#setObjectProperty(java.lang.String, java.lang.Object)
     */
    public void setObjectProperty(String name, Object value) throws JMSException {
        if (isReadonlyProps) {
        	ImaMessageNotWriteableException jex = new ImaMessageNotWriteableException("CWLNC0038", "A write operation failed on a message object body or on a message object property because the message was in read only state.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        if (!isValidName(name)) {
            if (name==null || name.length()==0) {
            	IllegalArgumentException iex = new ImaIllegalArgumentException("CWLNC0054","A method call to set a message property failed because the specified property name is null or is an empty string.");
                ImaTrace.traceException(2, iex);
                throw iex;
            }
            ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0037", "A method called to create a message property failed because the property name value ({0}) is not valid.", name);
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        if (props == null)
            props = new HashMap<String,Object>();
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
                      	retain = ((Integer)value).intValue() == 1;
                    else if (value instanceof Byte)
                        retain = ((Byte)value).intValue() == 1;
                    else if (value instanceof Short)
                        retain = ((Short)value).intValue() == 1;
                    else 
                        retain = false;
                    return;
                }
                return;    /* Ignore all other props starting JMS_ */
            } 
            if (name.startsWith("JMS") && !name.startsWith("JMSX"))
                return;
            props.put(name, value);
        } else {
        	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0034", "A method call to set an object on a message failed because the input object type ({0}) is not supported.  The object types that are supported are Boolean, Byte, byte[], Character, Double, Float, Integer, Long, Short, and String.", value.getClass());
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }

    /* 
     * Set a short property.
     * 
     * @see javax.jms.Message#setShortProperty(java.lang.String, short)
     */
    public void setShortProperty(String name, short value) throws JMSException {
        setObjectProperty(name, value);
    }

    /* 
     * Set a string property.
     * 
     * @see javax.jms.Message#setStringProperty(java.lang.String, java.lang.String)
     */
    public void setStringProperty(String name, String value) throws JMSException {
        setObjectProperty(name, (Object)value);
    }
    /*
     * Check that the message is writeable
     */
    void checkWriteable() throws JMSException {
        if (isReadonly) {
        	ImaMessageNotWriteableException jex = new ImaMessageNotWriteableException("CWLNC0038", "A write operation failed on a message object body or on a message object property because the message was in read only state.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        if (body == null)
            body = ByteBuffer.allocate(64);
    }
    /*
     * Check that the message is readable
     */
    void checkReadable() throws JMSException {
        if (!isReadonly) {
        	ImaMessageNotReadableException jex = new ImaMessageNotReadableException("CWLNC0071", "The client failed to read message object content because the message is not readable.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }


    
    /*******************************************************************************/
    /*                                                                             */
    /* Implementation methods                                                      */
    /*                                                                             */
    /*******************************************************************************/
    
    /*
     * Check that a property name is valid
     */
    static boolean isValidName(String name) {
        if (name == null || name.length() == 0)
            return false;
        for (int i=0; i<name.length(); i++) {
            int codept = (int)name.charAt(i);
            if (codept >= 0xd800 && codept <= 0xdbff && (i+1 < name.length())) {
                codept = ((codept&0x3ff)<<10) + (name.charAt(++i)&0x3ff) + 0x10000; 
            }
            if (i==0) {
                if (!Character.isJavaIdentifierStart(codept))
                    return false;
            } else {
                if (!Character.isJavaIdentifierPart(codept))
                    return false;
            }
        }
        return true;
    }

    
    /*
     * Private class to implement the Enumerator interface for a Set
     */
    final static class SetEnumeration implements Enumeration<String> {
        private Iterator<String> it;

        SetEnumeration(Set<String> set) {
            it = set.iterator();
        }

        public boolean hasMoreElements() {
            return it.hasNext();
        }

        public String nextElement() {
            return it.next();
        }
    }
    
    
    /*
     * Return the size of the body 
     */
    public int bodySize() {
    	if (body == null)
    		return -1;
    	return body.limit();
    }


    /*
     * @see javax.jms.Message#getBody(java.lang.Class)  
     * @since JMS 2.0
     */
    @SuppressWarnings("unchecked")
    public <T> T getBody(Class<T> cls) throws JMSException {
        if (body == null)
            return (T) null;
        
        switch (msgtype) {
        case MTYPE_Message:
            break;
            
        case MTYPE_BytesMessage:
            if (cls.isAssignableFrom(byte[].class)) {
                byte [] ret = new byte[body.limit()];
                body.get(ret);
                return (T) ret;
            }
            if (body.limit() == 0)
                return (T) null;
            break;
            
        case MTYPE_MapMessage:
            if (cls.isAssignableFrom(Map.class))
                return (T)((ImaMapMessage)this).map;
            if ((((ImaMapMessage)this).map).isEmpty())
                return (T) null;
            break;
            
        case MTYPE_ObjectMessage:
            if (cls.isInstance(((ImaObjectMessage)this).getObject()))
                return (T)(((ImaObjectMessage)this).getObject());
            break;
            
        case MTYPE_StreamMessage:
            break;
            
        case MTYPE_TextMessage:
            if (cls.isInstance("")) 
                return (T)((ImaTextMessage)this).getText();
            break;
        }
        throw new ImaMessageFormatException("CWLNC0230", "The message body cannot be assigned to the specified type.");
    }



    /*
     * @see javax.jms.Message#isBodyAssignableTo(java.lang.Class)
     * @since JMS 2.0
     */
    @SuppressWarnings({ "rawtypes", "unchecked" })
    public boolean isBodyAssignableTo(Class cls) throws JMSException {
        if (cls == null)
            return false;
        if (body == null)
            return true;
        
        switch (msgtype) {
        case MTYPE_Message:
            break;
            
        case MTYPE_BytesMessage:
            if (cls.isAssignableFrom(byte[].class)) {
                return true;
            }
            if (body.limit() == 0)
                return true;
            break;
            
        case MTYPE_MapMessage:
            if (cls.isAssignableFrom(Map.class))
                return true;
            if ((((ImaMapMessage)this).map).isEmpty())
                return true;
            break;
            
        case MTYPE_ObjectMessage:
            if (cls.isInstance(((ImaObjectMessage)this).getObject()))
                return true;
            break;
            
        case MTYPE_StreamMessage:
            break;
            
        case MTYPE_TextMessage:
            if (cls.isInstance(""))
                return true;
            break;
        }
        return false;
    }

    
    /*
     * @see javax.jms.Message#getJMSDeliveryTime()
     * @since JMS 2.0
     */
    public long getJMSDeliveryTime() throws JMSException {
        return deliveryTime;
    }

    /*
     * @see javax.jms.Message#setJMSDeliveryTime(long)
     * @since JMS 2.0
     */
    public void setJMSDeliveryTime(long deliveryTime) throws JMSException {
        this.deliveryTime = deliveryTime;
    }
    
    
    
    /*
     * Return the object as a string.
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return toString(TS_Common);
    }
    
    /*
     * Default formatBody
     */
    public String formatBody() {
    	return null;
    }
    
    /*
     * Display the properties.
     * This matches the toString() for map except that if the
     * value is a string it is surrounded by quote marks.
     */
    static String toPropString(Map<String,Object> prop) {
        if (prop == null)
            return "{}";
        Iterator<Entry<String,Object>> i = prop.entrySet().iterator();
        if (! i.hasNext())
            return "{}";

        StringBuilder sb = new StringBuilder();
        sb.append("{ ");
        for (;;) {
            Entry<String,Object> e = i.next();
            String key = e.getKey();
            Object value = e.getValue();
            sb.append(key);
            sb.append('=');
            if (value instanceof String)
                sb.append("\"" + value + "\"");
            else
                sb.append(value);
            if (! i.hasNext())
                return sb.append(" }").toString();
            sb.append(',').append(' ');
        }
    }
    
    /*
     * Superclass implementation for all message types for debug toString().
     * This is externalized by ImaJmsObject.toString()
     */
    public String toString(int opt) {
    	StringBuffer sb = new StringBuffer();
    	boolean started = false;
    	if ((opt&TS_Class) != 0) {
    		String mtype;
    		switch (msgtype) {
    		case MTYPE_BytesMessage:   mtype = "ImaBytesMessage";       break;
    		case MTYPE_MapMessage:     mtype = "ImaMapMessage";         break;
    		case MTYPE_ObjectMessage:  mtype = "ImaObjectMessage";      break;
    		case MTYPE_StreamMessage:  mtype = "ImaStreamMessage";      break;
    		case MTYPE_TextMessage:    mtype = "ImaTextMessage";        break;
    		default:                   mtype = "ImaMessage";            break;
    		}
    		sb.append(mtype);
    		started = true;
    	}
    	if ((opt&TS_HashCode) != 0) {
    		sb.append('@').append(Integer.toHexString(hashCode()));
    		started = true;
    	}
    	if ((opt&TS_Info) != 0) {
    		if (started)
    			sb.append(' ');
    		started = true;
    		if (body == null)
    		    sb.append("body=null");
    		else
    			sb.append("body="+body.limit());
    		if (deliveryCount > 1)
    			sb.append(" JMSRedelivered=" + (deliveryCount-1));
    		if (messageID != null)
    			sb.append(" JMSMessgeID=").append(messageID);
    		if (jmsdestination != null)
    			sb.append(" JMSDestination=").append(jmsdestination);
    		if (deliverymode != 0 || jmsreply != null || jmstime != 0 || expiration != 0 || ack_sqn != 0)
    		    sb.append("\n  ");
    		if (deliverymode != 0)
    		    sb.append("JMSDeliveryMode=").append(deliverymode==DeliveryMode.PERSISTENT ?
    		    		"PERSISTENT" : "NON-PERISTENT");
    		if (jmsreply != null)
    			sb.append(" JMSReplyTo=").append(jmsreply);
    		if (jmstime != 0)
    		    synchronized (ImaMessage.class) {
                    sb.append(" JMSTimestamp=").append(iso8601.format(jmstime));
                }
    		if (expiration != 0)
    		    synchronized (ImaMessage.class) {
                    sb.append(" JMSExpire=").append(iso8601.format(expiration));
    		    }
    		if (ack_sqn != 0)
    			sb.append(" JMS_IBM_ACK_SQN=").append(ack_sqn);
    		/* TODO: */
    	}

    	if ((opt&TS_Details) != 0) {
    		if (priority != 4 || jmstype != null || correlationID != null || retain) {
				if (started)
					sb.append("\n  ");
				boolean needspace = false;
	    		if (priority != 4) {
	    			sb.append("JMSPriority="+priority);
	    			needspace = true;
	    		}		
	    		if (jmstype != null) {
	    			if (needspace)
	    				sb.append(' ');
	    			sb.append("JMSType=").append(jmstype);
	    			needspace = true;
	    		}		
	    		if (correlationID != null) {
	    			if (needspace)
	    				sb.append(' ');
	    			sb.append("JMSCorrelationID=").append(correlationID);
	    			needspace = true;
	    		}	
	            if (retain) {
	                if (needspace)
	                    sb.append(' ');
	                sb.append("JMS_IBM_Retain=1");
	                needspace = true;
	            }
    		}
    	}
    	if ((opt&TS_Links) != 0) {
    		if (session != null) {
    			if (started)
    				sb.append("\n  ");
    		    sb.append("session=").append(Integer.toHexString(session.hashCode()));
    		    sb.append(" connect=").append(session.connect);
    		}    
    	}
    	if ((opt&TS_Properties) != 0) {
    		if (props != null && props.size() > 0) {
	    		if (started)
	    			sb.append("\n  ");
	    		started = true;
	    		sb.append("properties=").append(toPropString(props));
    		}
    	}
    	if ((opt&TS_Body) != 0) {
    		String bodys = null;
        	if (this instanceof ImaBytesMessage)
        		bodys = ((ImaBytesMessage)this).formatBody(1024, started ? "       " : "     ");
        	else
        		bodys = formatBody();
    		if (bodys != null) {
	    	    if (started)
	    	        sb.append("\n  ");
	    	    started = true;
	    		sb.append("body=").append(bodys);
    		}
    	}
    	return sb.toString();
    }

}
