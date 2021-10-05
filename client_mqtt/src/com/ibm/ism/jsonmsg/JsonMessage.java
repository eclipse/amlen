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

package com.ibm.ism.jsonmsg;

import java.util.List;
import java.io.UnsupportedEncodingException;
import java.util.Iterator;
import java.util.Map;

import com.ibm.ism.mqtt.ImaJson;
import com.ibm.ism.ws.IsmWSMessage;

/**
 *  
 */
public class JsonMessage extends IsmWSMessage {
	String  dest;
	String  subname;
	int     msgtype;
	int     qos;
	int     msgid;
	boolean persist;
	boolean isQueue;
	boolean retain = false;
	boolean dup = false;
	Map<String,Object> payloadmap;
	List<Object>       payloadlist;
	
	public static final int NullMessage  = 0;
	public static final int BytesMessage = 1;
	public static final int TextMessage  = 2;
	public static final int MapMessage   = 3;
	public static final int ListMessage  = 4;
	
	/**
	 * Create an unset message.
	 * The fields of the message must be set bfore it can be used
	 */
    public JsonMessage() {
    	super(null);
    }

    /**
     * Create a message with a string payload
     * @param topic   The topic on which to send
     * @param payload The payload of the message
     */
    public JsonMessage(String topic, String payload) {
    	super(payload);
    	this.msgtype = TextMessage;
        this.dest = topic;
    }

    @SuppressWarnings("unchecked")
	public JsonMessage(String subname, String dest, boolean isQueue, Object payload, boolean persist, boolean retain) {
    	super();
    	if (payload == null) {
    		msgtype = NullMessage;
    	} else if (payload instanceof String) {
            paystring = (String)payload;
            msgtype = TextMessage;
    	} else if (payload instanceof byte[]) {
    		paybytes = (byte[])payload;
    		msgtype = BytesMessage;
    	} else if (payload instanceof Map<?,?>) {
    		payloadmap = (Map<String,Object>)payload;
    		msgtype = MapMessage;
    	} else if (payload instanceof List<?>) {
    		payloadlist = (List<Object>)payload;
    		msgtype = ListMessage;
    	} else {
    		throw new IllegalArgumentException("payload object not valid.");
    	}
    	this.subname = subname;
    	this.dest    = dest;
    	this.isQueue = isQueue;
        this.persist = persist;
        this.retain  = retain;
        System.out.println("In JsonMessage() with retain = "+this.retain);
    }

    /**
     * Create a message with qos and retain set.
     * @param topic
     * @param replyto
     * @param payload
     * @param qos
     * @param retain
     */
    public JsonMessage(String dest, boolean isQueue, String payload, boolean persist, boolean retain) {
    	super(payload);
    	this.msgtype = TextMessage;
    	this.dest    = dest;
    	this.isQueue = isQueue;
        this.persist = persist;
        this.retain  = retain;
    }
    
    /**
     * Create a map message
     * @param topic   The topic on which to send
     * @param payload The payload of the message
     */
    public JsonMessage(String topic, Map<String,Object> payload) {
   	    super(null);
   	    this.payloadmap = payload;
   	    this.msgtype = MapMessage;
        this.dest = topic;
    }
   
    /**
     * Create a list message
     * @param topic   The topic on which to send
     * @param payload The payload of the message
     */
    public JsonMessage(String topic, List<Object> payload) {
   	    super(null);
   	    this.payloadlist = payload;
   	    this.msgtype = ListMessage;
        this.dest = topic;
    }
    
    /**
     * Get the name of the subscription.
     * This is normally only set when the message is received from a subscription.
     */
    public String getSubscription() {
        return subname;
    }
    
    /**
     * Get the topic from a message.
     * @return The topic of the message, or null if it is not set
     */
    public int getMessageType() {
        return msgtype;
    }
    
    /**
     * Return is the destination a queue
     */
    public boolean isQueue() {
        return isQueue;
    }

    /**
     * Get the topic from a message.
     * @return The topic of the message, or null if it is not set
     */
    public String getTopic() {
        return isQueue ? null : dest;	
    }

    /**
     * Get the topic from a message.
     * @return The topic of the message, or null if it is not set
     */
    public String getQueue() {
        return isQueue ? dest : null;
    }
    
    
	 /**
	  * Get the message payload as a byte array.
	  * @return The byte array form of the payload
	  */
    public byte [] getPayloadBytes() {
	    if (paybytes == null) {
		    switch (msgtype) {
			case MapMessage:  
			    paystring = jsonString(payloadmap);
				break;
			case ListMessage:
			    paystring = jsonString(payloadlist);
			    break;
			}
			if (paystring != null)
		        try { paybytes = paystring.getBytes("UTF-8"); } catch (UnsupportedEncodingException e) { }
		}		 
	    return paybytes;		
    }
    
    
    /*
     * Create a JSON string from a map
     */
    String jsonString(Map<String,Object> map) {
    	boolean first = true;
	    StringBuffer sb;
	    sb = new StringBuffer();
	    sb.append('{');
	    Iterator<String> it = map.keySet().iterator();
	    while (it.hasNext()) {
	    	String key = it.next();
	    	Object obj = map.get(key);
	    	if (!first) 
	    		sb.append(',');
	        else	
	    		first = false;
	    	ImaJson.putString(sb, key);
	    	sb.append(':');
	    	ImaJson.put(sb, obj);
	    }
	    sb.append('}');
	    return sb.toString();
    }

    /*
     * Create a JSON string from a map
     */
    String jsonString(List<Object> list) {
    	boolean first = true;
	    StringBuffer sb;
	    sb = new StringBuffer();
	    sb.append('[');
	    Iterator<Object> it = list.iterator();
	    while (it.hasNext()) {
	    	Object obj = it.next();
	    	if (!first) 
	    		sb.append(',');
	    	else
	    		first = false;
	    	ImaJson.put(sb, obj);
	    }
	    sb.append(']');
	    return sb.toString();
    }
	 
	/**
	 * Get the message payload as a string.
	 * @return The String form of the payload
	 */
	public String  getPayload() {
	    if (paystring == null) {
		    switch (msgtype) {
			case MapMessage:
				paystring = jsonString(payloadmap);
				break;
			case ListMessage:
				paystring = jsonString(payloadlist);
				break;	
		    default:	
		    	if (paybytes != null)
			        try { paystring = new String(paybytes, "UTF-8"); } catch (UnsupportedEncodingException e) {}
			}
			
		}        
	    return paystring;
	}
	 
    
    /**
     * Return the quality of service (qos) of the message.
     * @return The qos as an integer 0 to 2.
     */
    public int getQoS() {
    	return qos;
    }
    
    
    /**
     * Return the retain value of the message.
     * @return The retain value of the message as a boolean
     */
    
    
    public boolean getRetain() {
    	return retain;
    }
    
    
    /**
     * Return the duplicate flag.
     * This flag is normally set when a message is retransmitted.
     * @return The dup flag as a boolean
     */
    public boolean getDup() {
    	return dup;
    }
    
   
    
    /**
     * Set the topic of a message.
     * @param topic The topic to set.
     * @return This message
     */
    public JsonMessage setTopic(String topic) {
    	this.dest = topic;
    	this.isQueue = false;
    	return this;
    }
    
    /**
     * Set the payload of the message as a string.
     * @param payload   The payload of the message as a string
     * @return This message
     */
    public JsonMessage setBody(String body) {
    	this.msgtype = TextMessage;
    	this.paystring = body;
    	this.paybytes = null;
    	this.payloadmap = null;
    	return this;
    }
    
    /**
     * Set the payload of the message as a map
     * @param map The map as contents
     * @return This message
     */
    public JsonMessage setBody(Map<String,Object> map) {
    	this.msgtype = MapMessage;
    	this.payloadmap = map;
    	this.paystring = null;
    	this.paybytes = null;
    	this.payloadlist = null;
    	return this;
    }

    
    /**
     * Set the payload of the message as a list
     * @param list  The list as contents
     * @return This message
     */
    public JsonMessage setBody(List<Object> list) {
    	this.msgtype = MapMessage;
    	this.payloadlist = list;
    	this.paystring = null;
    	this.paybytes = null;
    	this.payloadmap = null;
    	return this;
    }
    
    
    /**
     * Set the payload of the message as a byte array.
     * @param payload   The message payload as a byte array.
     * @return This message
     */
    public JsonMessage setBody(byte [] body) {
    	if (body != null) {
    		int  len = body.length;
			paybytes = new byte[len];
			System.arraycopy(body, 0, paybytes, 0, len);
		}
    	msgtype = BytesMessage;
    	paystring = null;
    	payloadmap = null;
    	payloadlist = null;
    	return this;
    }
    
    /**
     * Set the payload of the message as a byte array with start and length.
     * @param payload  The message payload as a byte array
     * @param start    The start position within the byte array
     * @param len      The length of the message in bytes
     * @return This message
     */
    public JsonMessage setBody(byte [] body, int start, int len, int msgtype) {
    	if (body != null) {
			paybytes = new byte[len];
			System.arraycopy(body, start, paybytes, 0, len);
		}
    	this.msgtype = msgtype;
    	paystring = null;
    	payloadmap = null;
    	payloadlist = null;
    	return this;
    }


    
    /**
     * Set the quality of server (qos) for this message.
     * @param qos   The quality of service 0 to 2.
     * @return This message
     */
    public JsonMessage setQos(int qos) {
    	this.qos = qos;
    	return this;
    }
    
    /**
     * Set the retain flag for this message.
     * @param retain  The retain flag for the message.
     * @return This message
     */
    public JsonMessage setRetain(boolean retain) {
    	this.retain = retain;
    	return this;
    }
    
    public String toString() {
        StringBuffer buf = new StringBuffer();
        buf.append("{ \"Action\": \"Send\"");
        if (this.isQueue()) 
        	JsonConnection.putOption(buf, "Queue", this.getQueue());
        else
        	JsonConnection.putOption(buf, "Topic", this.getTopic());
        if (this.persist)
        	JsonConnection.putBoolOption(buf, "Persistent", this.persist);
        //if (this.qos > 0)
        JsonConnection.putIntOption(buf, "QoS", this.qos);
        if (this.getRetain())
        	JsonConnection.putBoolOption(buf, "Retain", true);
        
        String payload = this.getPayload();
        buf.append(", \"Body\":");
        buf.append('"');
        ImaJson.escape(buf, payload).append("\"");
        buf.append(" }");
        return buf.toString();
    }

}
