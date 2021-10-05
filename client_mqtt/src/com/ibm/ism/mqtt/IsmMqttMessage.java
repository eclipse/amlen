/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ism.mqtt;

import java.util.List;
import java.io.UnsupportedEncodingException;
import java.nio.charset.StandardCharsets;
import java.util.Iterator;
import java.util.Map;
import java.util.Vector;

import com.ibm.ism.mqtt.ImaJson;
import com.ibm.ism.ws.IsmWSMessage;

/**
 *  
 */
public class IsmMqttMessage extends IsmWSMessage implements IsmMqttPropChecker {
	String  topic;
	String  replyto = null;
	byte [] corrid;
	long    expire = -1;
	int     msgtype;
	int     qos;
	int     msgid = 0;
	int     topicAlias;
	int     maxTopicAlias;
	int     subid = 0;
	int []  subids;
	boolean retain = false;
	boolean dup = false;
	Map<String,Object> payloadmap;
	List<Object>       payloadlist;
	List<IsmMqttUserProperty> msgprops;
	String  contentType;
	
	public static final int NullMessage  = 0;
	public static final int BytesMessage = 1;
	public static final int TextMessage  = 2;
	public static final int MapMessage   = 3;
	public static final int ListMessage  = 4;
	
	/**
	 * Create an unset message.
	 * The fields of the message must be set before it can be used
	 */
    public IsmMqttMessage() {
    	super(null);
    }

    /**
     * Create a message with a string payload
     * @param topic   The topic on which to send
     * @param payload The payload of the message
     */
    public IsmMqttMessage(String topic, String payload) {
    	super(payload);
    	this.msgtype = TextMessage;
        if (topic == null || topic.length() == 0) {
            throw new IllegalArgumentException("The topic is not valid.");
        }
        this.topic = topic;
    }

    @SuppressWarnings("unchecked")
	public IsmMqttMessage(String topic, Object payload, int qos, boolean retain) {
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
    	if (topic == null) {
            throw new IllegalArgumentException("The topic is not valid.");
    	}
    	this.topic = topic;
    	if (qos < 0 || qos > 2) {
            throw new IllegalArgumentException("The QoS is not valid.");
    	}
        this.qos     = qos;
        this.retain  = retain;
    }

    /**
     * Create a message with qos and retain set.
     * @param topic
     * @param replyto
     * @param payload
     * @param qos
     * @param retain
     */
    public IsmMqttMessage(String topic, String payload, int qos, boolean retain) {
    	super(payload);
    	this.msgtype = TextMessage;
        //if (topic == null || topic.length() == 0) {
        //    throw new IllegalArgumentException("The topic is not valid.");
        //}
    	this.topic = topic;
        if (qos < 0 || qos > 2) {
            throw new IllegalArgumentException("The QoS is not valid.");
        }
        this.qos     = qos;
        this.retain  = retain;
    }
    
   /**
    * Create a map message
    * @param topic   The topic on which to send
    * @param payload The payload of the message
    */
   public IsmMqttMessage(String topic, Map<String,Object> payload) {
   	   super(null);
   	   this.payloadmap = payload;
   	   this.msgtype = MapMessage;
       if (topic == null || topic.length() == 0) {
           throw new IllegalArgumentException("The topic is not valid.");
       }
       this.topic = topic;
   }
   
   /**
    * Create a list message
    * @param topic   The topic on which to send
    * @param payload The payload of the message
    */
   public IsmMqttMessage(String topic, List<Object> payload) {
   	   super(null);
   	   this.payloadlist = payload;
   	   this.msgtype = ListMessage;
       if (topic == null || topic.length() == 0) {
           throw new IllegalArgumentException("The topic is not valid.");
       }
       this.topic = topic;
   }
    
    /**
     * Get the MQTT packet ID from a message.
     * @return The topic of the message, or null if it is not set
     */
    public int getMessageID() {
        return msgid;
    }
    
    /**
     * Get the message type from a message.
     * @return The topic of the message, or null if it is not set
     * @since MQTTv5
     */
    public int getMessageType() {
        return msgtype;
    }
    
    /**
     * Normally the message type is set automatically.
     * This is only here to allow it to be set incorrectly.
     * @param msgtype
     */
    public void setMessageType(int msgtype) {
        switch (msgtype) {
        case BytesMessage:
        case TextMessage:
            break;
        default:
            throw new RuntimeException("Invalid MessageType: " + msgtype);
        
        }
        this.msgtype = msgtype;
    }
    
    /**
     * Get the content type from a message
     * @return The content type of the message or null if it is not set
     * @since MQTTv5
     */
    public String getContentType() {
        return contentType;
    }

    
    /**
     * Get the topic from a message.
     * @return The topic of the message, or null if it is not set
     */
    public String getTopic() {
        return topic;	
    }
    
    /**
     * Get the subscription ID for a message
     * @return The subscription ID or null to indicate that there are no subscription IDs
     * @since MQTTv5
     */
    public int getSubscriptionID() {
        return subid;
    }
    
    /**
     * Get the topic alias
     * @return The topic alias
     * @since MQTTv5
     */
    public int getTopicAlias() {
        return topicAlias;
    }
    
    
    /**
     * Get an array of subscription IDs for a message.
     * Some servers can send back multiple
     * @return An array of subscription IDs, or null to indicate there are no subscription IDs
     * @since MQTTv5
     */
    public int [] getSubscriptionIDArray() {
        if (subids == null) {
            if (subid == 0)
                return null;
            int [] ret = new int[1];
            ret[0] = subid;
            return ret;
        } else {
            return subids;
        }
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
     * Get the list of properties, or null if there are no properties
     * @return A list of user properties
     * @since MQTTv5
     */
    public synchronized List<IsmMqttUserProperty> getUserProperties() {
        return msgprops;
    }

    
    /*
     * Add a user property
     * @since MQTTv5
     */
    public synchronized void addUserProperty(String name, String value) {
        if (msgprops == null) {
            msgprops = (List<IsmMqttUserProperty>)new Vector<IsmMqttUserProperty>();
        }
        if (name == null)
            name = "";
        if (value == null)
            value = "";
        msgprops.add(new IsmMqttUserProperty(name, value));
    }
    
    /*
     * Clear the user properties for a message
     * @since MQTTv5
     */
    public synchronized void clearProperties() {
        msgprops = null;
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
     * Return the reply to topic of a message.
     * @return The reply to topic as a String, or null to indicate the message has no replyto.
     */
    public String getReplyTo() {
    	return replyto;
    	
    }
    
    
    /**
     * Return the correlation data of a message.
     * @return The correlation or null to indicate the message has no correlation ID.
     */
    public byte [] getCorrelation() {
    	return corrid;
    }
    
    /*
     * Get the correction data as a string
     */
    public String getCorrelationString() {
        if (corrid == null)
            return null;
        return new String(corrid, StandardCharsets.UTF_8);
    }
    
    /**
     * Get the message expire time in seconds
     * @return The expire time in seconds
     * @since MQTTv5
     */
    public long getExpire() {
        return this.expire;
    }
    
    
    /**
     * Set the topic of a message.
     * @param topic The topic to set.
     * @return This message
     */
    public IsmMqttMessage setTopic(String topic) {
    	this.topic = topic;
    	return this;
    }
    
    /**
     * Set the payload of the message as a string.
     * @param payload   The payload of the message as a string
     * @return This message
     */
    public IsmMqttMessage setPayload(String payload) {
    	this.msgtype = TextMessage;
    	this.paystring = payload;
    	this.paybytes = null;
    	this.payloadmap = null;
    	return this;
    }
    
    /**
     * Set the payload of the message as a map
     * @param map The map as contents
     * @return This message
     */
    public IsmMqttMessage setPayload(Map<String,Object> map) {
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
    public IsmMqttMessage setPayload(List<Object> list) {
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
    public IsmMqttMessage setPayload(byte [] payload) {
    	if (payload != null) {
    		int  len = payload.length;
			paybytes = new byte[len];
			System.arraycopy(payload, 0, paybytes, 0, len);
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
    public IsmMqttMessage setPayload(byte [] payload, int start, int len, int msgtype) {
    	if (payload != null) {
			paybytes = new byte[len];
			System.arraycopy(payload, start, paybytes, 0, len);
		}
    	this.msgtype = msgtype;
    	paystring = null;
    	payloadmap = null;
    	payloadlist = null;
    	return this;
    }

    /**
     * Set the reply to topic.
     * @param replyto  The reply to topic
     * @return This message.
     */
    public IsmMqttMessage setReplyTo(String replyto) {
    	this.replyto = replyto;
    	return this;
    }
    
    /**
     * Set the correlation ID.
     * @param corrid  The coorelation ID
     * @return This message.
     */
    public IsmMqttMessage setCorrelation(byte [] corrid) {
    	this.corrid = corrid;
    	return this;
    }
    
    /**
     * Set the correlation ID.
     * @param corrid  The coorelation ID
     * @return This message.
     */
    public IsmMqttMessage setCorrelation(String corrid) {
        if (corrid != null) {
            this.corrid = corrid.getBytes(StandardCharsets.UTF_8);
        }    
        return this;
    }
    
   
    /**
     * Set message expire time in seconds
     * @param expire  The expire time in seconds
     * @since MQTTv5
     */
    public IsmMqttMessage setExpire(long expire) {
        if (expire < 0) {
            this.expire = -1;
        } else {
            if (expire > 0xffffffffl)
                this.expire = 0xffffffff;
            else
                this.expire = expire;
        }    
        return this;
    }
    
    /**
     * Set the content type for a message
     * @param contentType  The content type as a string
     * @since MQTTv5
     */
    public void setContentType(String contentType) {
        this.contentType = contentType;
    }

    
    /**
     * Set the quality of server (qos) for this message.
     * @param qos   The quality of service 0 to 2.
     * @return This message
     */
    public IsmMqttMessage setQos(int qos) {
    	this.qos = qos;
    	return this;
    }
    
    /**
     * Set the MQTT message ID.
     * @param msgid  The message ID which is a value 1 to 65535
     * @return This message
     */
    public IsmMqttMessage setMessageID(int msgid) {
    	this.msgid = msgid;
    	return this;
    }
    
    /**
     * Set the retain flag for this message.
     * @param retain  The retain flag for the message.
     * @return This message
     */
    public IsmMqttMessage setRetain(boolean retain) {
    	this.retain = retain;
    	return this;
    }
    
    /**
     * Set the dup flag for this message.
     * @param dup   A boolean value which specifies if this is a duplicate (retransmitted) message.
     * @return This message
     */
    public IsmMqttMessage setDup(boolean dup) {
    	this.dup = dup;
    	return this;
    }
    

    public void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, int value) {
        boolean valid = true;
        switch (fld.id) {
        case IsmMqttConnection.MPI_PayloadFormat:
            if (value < 0 || value > 2)
                valid = false;
            msgtype = value + 1;
            break;
            
        case IsmMqttConnection.MPI_MsgExpire:
            if (value < 0)
                valid = false;
            expire = value;
            break;
        case IsmMqttConnection.MPI_TopicAlias:
            if (value < 1 || value > maxTopicAlias)
                valid = false;
            topicAlias = value;
            break;
            
        case IsmMqttConnection.MPI_SubID:
            if (value == 0) {
                valid = false;
            } else {
                if (subid == 0) {
                    subid = value;
                } else {
                    if (subids == null) {
                        subids = new int[2];
                        subids[0] = subid;
                        subids[1] = value;
                    } else {
                        int [] newsubids = new int[subids.length+1];
                        System.arraycopy(subids, 0, newsubids, 0, subids.length);
                        newsubids[subids.length] = value;
                        subids = newsubids;
                    }
                }
                    
            }
            break;
        } 
        if (!valid)
            throw new IllegalArgumentException("The " + fld.name + " property is not valid: " + value);
    }

    public void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, String str) {
        switch (fld.id) {
        case IsmMqttConnection.MPI_ContentType:
            setContentType(str);
            break;
        case IsmMqttConnection.MPI_ReplyTopic:
            setReplyTo(str);
            break;
        } 
    }

    public void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, byte[] ba) {
        switch (fld.id) {
        case IsmMqttConnection.MPI_Correlation:      
            setCorrelation(ba);
            break;
        }
    }
    
    public void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, String name, String value) {
        addUserProperty(name, value);    
    }
    
    String getMsgTypeName() {
        switch (msgtype) {
        default:
        case BytesMessage: return "Byte";
        case TextMessage:  return "Text";
        case ListMessage:  return "List";
        case MapMessage:   return "Map";
        }
    }
    
    /*
     * Return a string at va
     * 0 = basic info
     * 1 = show properties
     * 2 = show the whole message
     */
    public String toString(int level) {
        StringBuffer sb = new StringBuffer();
        int len;
        byte [] body = null;
        if (paybytes != null) {
            len = paybytes.length;
            body = paybytes;
        } else if (paystring != null) {
            body = paystring.getBytes(StandardCharsets.UTF_8);
            len = body.length;
        } else {
            len = 0;
        }
        sb.append("IsmMqttMessage len=" + len + " QoS=" + getQoS());
        if (msgtype != BytesMessage) {
            sb.append(" type=" + getMsgTypeName());
        }
        if (msgid != 0) {
            sb.append(" msgid=" + msgid);
        }
        if (getRetain())
            sb.append(" retain=1");
        if (getDup())
            sb.append(" dup=1");
        
        if (level > 0 && (subid != 0 || subids != null)) {
            sb.append(" subid=");
            if (subids == null) {
                sb.append(subid);
            } else {
                for (int j=0; j<subids.length; j++) {
                    if (j != 0)
                        sb.append(',');
                    sb.append(subids[j]);
                }    
            }    
        }
        if (level > 0 && topicAlias != 0) {
            sb.append(" alias=" + topicAlias);
        }
        
        if (getTopic() != null) {
            sb.append(" topic=\"" + getTopic() + "\"\n");
        } else {
            sb.append("\n");
        }
        
        if (level > 0) {
            if (contentType != null) {
                sb.append("   _ContentType=\"" + contentType + "\"\n");
            }
            if (replyto != null) {
                sb.append("   _ResponseTopic=\"" + replyto + "\"\n");
            }
            if (corrid != null) {
                sb.append("   _CorrelationData=\"" + getCorrelationString() + "\"\n");
            }
            if (msgprops != null) {
                Iterator<IsmMqttUserProperty> it = msgprops.iterator();
                while (it.hasNext()) {
                    IsmMqttUserProperty prop = it.next();
                    sb.append("   " + prop.name() + "=\"" + prop.value() + "\"\n");
                }
            }
        }
        /* Put out full payload */
        if (paystring != null && len <= 128) {
            sb.append("   body=\"" + paystring + "\"\n" );
        } else {
            String out = new String(getPayloadBytes(), StandardCharsets.UTF_8);
            if (out.length() > 128 && level < 2)
                out.substring(0, 128);
            sb.append("   body=\"" + out + "\"\n" );
        }
        return sb.toString();
    }
    
    /*
     * Return a string at the default level
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return toString(0);
    }

}
