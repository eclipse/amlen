/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package com.ibm.ism.ws.test;

import java.nio.charset.StandardCharsets;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.paho.mqttv5.common.packet.MqttProperties;
import org.eclipse.paho.mqttv5.common.packet.UserProperty;

import com.ibm.ism.jsonmsg.JsonMessage;
import com.ibm.ism.mqtt.IsmMqttMessage;
import com.ibm.ism.mqtt.IsmMqttUserProperty;
import com.ibm.ism.ws.IsmWSMessage;
import com.ibm.ism.ws.test.jsonmsg.JsonMessageSend;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttMessage;
import com.ibm.micro.client.mqttv3.MqttTopic;

public class MyMessage {
	
	static final int TEXT = IsmWSMessage.TEXT;
	static final int BINARY = IsmWSMessage.BINARY;

	public static final int ISMWS	= MyConnection.ISMWS;
	public static final int ISMMQTT	= MyConnection.ISMMQTT;
	public static final int MQMQTT	= MyConnection.MQMQTT;
	public static final int PAHO	= MyConnection.PAHO;
	public static final int JSON	= MyConnection.JSONTCP;
	public static final int PAHOV5  = MyConnection.PAHOV5;

	private MqttMessage	mqMqttMessage = null;
	private MqttTopic mqMqttTopic = null;
	private org.eclipse.paho.client.mqttv3.MqttMessage pahoMqttMessage = null;
	private org.eclipse.paho.mqttv5.common.MqttMessage pahoMqttMessagev5 = null;
	private MyJsonMsg jsonMessage = null;
//	private String pahoMqttTopic = null;
	private int msgType = 0;
	private Object message = null;
	private String topic = null;
	private int dataType = IsmWSMessage.BINARY;
	private String messageText = null;
	private boolean incrementing = false;
	private boolean includeQoS = false;
	private int counter = 0;
	private MqttProperties pahov5props = null;
	private MqttProperties receivedv5props = null;
	
	//private boolean _CRC = false;
	//private boolean _randomize = false;
	//private long _seed = 0;
	
	MyMessage(MqttTopic topic, MqttMessage message) throws MqttException {
		mqMqttMessage = message;
		this.message = message;
		mqMqttTopic   = topic;
		this.topic	= mqMqttTopic.getName();
		msgType		= MQMQTT;
		dataType	= IsmWSMessage.BINARY;
	}
	
	MyMessage(String topic,
			org.eclipse.paho.client.mqttv3.MqttMessage message) throws MqttException {
		pahoMqttMessage = message;
		this.message = message;
		this.topic	= topic;
		msgType		= PAHO;
		dataType	= IsmWSMessage.BINARY;
	}
	
	MyMessage(String topic, org.eclipse.paho.mqttv5.common.MqttMessage message) throws org.eclipse.paho.mqttv5.common.MqttException {
		pahoMqttMessagev5 = message;
		this.message = message;
		this.topic = topic;
		receivedv5props = message.getProperties();
		msgType = PAHOV5;
		dataType = IsmWSMessage.BINARY;
	}
	
	MyMessage(String topic, String payload, int QoS, boolean retain, MyConnection con) throws IsmTestException {
		init(con.getConnectionType(), topic, payload, QoS, retain, false, con);
	}

	MyMessage(int msgType, String topic, String payload, int QoS, boolean retain, boolean includeQoS, MyConnection con) throws IsmTestException {
		init(msgType, topic, payload, QoS, retain, includeQoS, con);
	}
	
	/*
	 * Change the UTF-8 character for \u05ff (undefined) to be invalid UTF-8.
	 * The UTF-8 encoding is 0xD7 0xBF
	 */
	byte [] fixPayload(String p) {
	    byte [] b = p.getBytes(StandardCharsets.UTF_8);
	    if (b != null) {
	        for (int i=0; i<b.length; i++) {
	            if (b[i] == (byte)0xd7 && b[i+1] == (byte)0xbf) 
	                b[i] = (byte)0xff;
	        }
	    }
	    return b;
	}

	void init(int msgType, String topic, String payload, int QoS, boolean retain, boolean includeQoS, MyConnection con) throws IsmTestException {
		this.msgType = msgType;
		this.dataType = TEXT;
		this.topic = topic;
		this.messageText = payload;
		this.includeQoS = includeQoS;
		switch (msgType) {
		case MyConnection.MQMQTT:
			mqMqttMessage = new MqttMessage(fixPayload(payload));
			message = mqMqttMessage;
			mqMqttMessage.setQos(QoS);
			mqMqttMessage.setRetained(retain);
			dataType	= BINARY;
			break;
		case MyConnection.PAHO:
			pahoMqttMessage = new org.eclipse.paho.client.mqttv3.MqttMessage(fixPayload(payload));
			message = pahoMqttMessage;
			pahoMqttMessage.setQos(QoS);
			pahoMqttMessage.setRetained(retain);
			dataType	= BINARY;
			break;
		case MyConnection.PAHOV5:
			pahoMqttMessagev5 = new org.eclipse.paho.mqttv5.common.MqttMessage(fixPayload(payload));
			message = pahoMqttMessagev5;
			pahoMqttMessagev5.setProperties(new MqttProperties());
			pahoMqttMessagev5.setQos(QoS);
			pahoMqttMessagev5.setRetained(retain);
			pahov5props = new MqttProperties();
			dataType = BINARY;
			break;
		case MyConnection.ISMMQTT:
		    if (topic == null)
		        topic = "x";
		    System.out.println("topic=" + topic);
		    if (payload != null && (payload.indexOf(0) >= 0 || payload.indexOf(0x5ff) >= 0)) {
		        message = new IsmMqttMessage(topic, fixPayload(payload), QoS, retain);  
		    } else {
			    message = new IsmMqttMessage(topic, payload, QoS, retain);
		    }    
			break;
		case MyConnection.ISMWS:
			message = new IsmWSMessage(payload);
			break;
		case MyConnection.JSONTCP:
		case MyConnection.JSONWS:
			jsonMessage = new MyJsonMsg(topic, payload);
			jsonMessage.getJsonMessage().setQos(QoS);
			jsonMessage.getJsonMessage().setRetain(retain);
			message = jsonMessage;
			break;
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+1),
					"Unknown message type: "+msgType);
		}
	}
	

	MyMessage(int msgType, String topic, byte[] payload, int QoS, boolean retain, MyConnection con) throws IsmTestException {
		init(msgType, topic, payload, QoS, retain, con.getConnectionType(), con);
	}

	MyMessage(int msgType, String topic, byte[] payload, int QoS, boolean retain, int mtype, MyConnection con) throws IsmTestException {
		init(msgType, topic, payload, QoS, retain, mtype, con);
	}
	void init(int connType, String topic, byte[] payload, int QoS, boolean retain, int mtype, MyConnection con) throws IsmTestException {
		this.msgType = connType;
		this.dataType = BINARY;
		this.topic = topic;
		switch (msgType) {
		case MyConnection.MQMQTT:
			mqMqttMessage = new MqttMessage(payload);
			message = mqMqttMessage;
			mqMqttMessage.setQos(QoS);
			mqMqttMessage.setRetained(retain);
			break;
		case MyConnection.PAHO:
			pahoMqttMessage = new org.eclipse.paho.client.mqttv3.MqttMessage(payload);
			message = pahoMqttMessage;
			pahoMqttMessage.setQos(QoS);
			pahoMqttMessage.setRetained(retain);
			break;
		case MyConnection.PAHOV5:
			pahoMqttMessagev5 = new org.eclipse.paho.mqttv5.common.MqttMessage(payload);
			message = pahoMqttMessagev5;
			pahoMqttMessagev5.setQos(QoS);
			pahoMqttMessagev5.setRetained(retain);
			break;
		case MyConnection.ISMMQTT:
			message = new IsmMqttMessage(topic, payload, QoS, retain);
			break;
		case MyConnection.ISMWS:
			message = new IsmWSMessage(payload, 0, (null == payload ? 0 : payload.length), mtype);
			msgType = mtype;
			break;
		case MyConnection.JSONTCP:
		case MyConnection.JSONWS:
			jsonMessage = new MyJsonMsg(topic, new String(payload));
			message = jsonMessage;
			jsonMessage.getJsonMessage().setQos(QoS);
			jsonMessage.getJsonMessage().setRetain(retain);
			break;
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+2),
					"Unknown message type: "+msgType);
		}
	}
	
	 
	 /**
	  * Create a message with a byte array payload.
	  * @param payload  A byte array containing the payload 
	  * @param start    The starting byte within the byte array
	  * @param len      The length of the message in bytes
	  * @param mtype    The type of the message (TEXT/BINARY)
	  */
	public MyMessage(byte[] payload, int start, int len, int mtype) {
		message	= new IsmWSMessage(payload, start, len, mtype);
		msgType	= ISMWS;
		dataType	= mtype;
	}
	 
	 /**
	  * Create a message with a String payload.
	  * @param payload  A string containing the payload.
	  */
	public MyMessage(String payload) {
		message	= new IsmWSMessage(payload);	 	 
		msgType	= ISMWS;
		dataType	= IsmWSMessage.TEXT;
	}

	public MyMessage(MyJsonMsg message) {
		this.msgType = JSON;
		this.message = message;
		this.jsonMessage = message;
		this.topic = message.getJsonMessage().getTopic();
	}
	
	public MyMessage(IsmWSMessage message) {
		if (message instanceof IsmMqttMessage) {
			this.msgType = ISMMQTT;
			this.message = message;
			this.topic = ((IsmMqttMessage)message).getTopic();
			if (IsmMqttMessage.TextMessage == ((IsmMqttMessage)message).getMessageType()) {
				dataType = TEXT;
			} else {
				dataType = BINARY;
			}
		} else {
			switch (message.getType()) {
			case IsmWSMessage.BINARY:
				message = new IsmWSMessage(message.getPayloadBytes(), 0, message.getPayloadBytes().length, message.getType());
				msgType = BINARY;
				break;
			case IsmWSMessage.TEXT:
				message = new IsmWSMessage(message.getPayload());
				msgType = TEXT;
				break;
			}
		}
	}

	/*
	 * The the length of the payload as bytes
	 */
	public int getMessageLength() {
		int result = 0;
		byte [] payload;
		switch (this.msgType) {
		case ISMMQTT:
		    payload = ((IsmMqttMessage)message).getPayloadBytes();
		    if (payload != null)
		        result = payload.length;
		    break;
		case MQMQTT:
			if (null != mqMqttMessage.getPayload())
				result = mqMqttMessage.getPayload().length;
			break;
		case PAHO:
			if (null != pahoMqttMessage.getPayload())
				result = pahoMqttMessage.getPayload().length;
			break;
		case PAHOV5:
			if (null != pahoMqttMessagev5.getPayload())
				result = pahoMqttMessagev5.getPayload().length;
			break;
		case JSON:
			payload = jsonMessage.getJsonMessage().getPayloadBytes();
			if (payload != null)
				result = payload.length;
			break;
		}
		return result;
	}
	
	/*
	 * Pahov5 splits the message into base and properties, so allow the propserties
	 * to be exported.
	 */
	public MqttProperties getMqttProperties() {
	    return pahov5props;
	}

	/*
	 * Get the implementation message object as an Object
	 */
	public Object getMessage() {
		return message;
	}

	public Object getMessage(boolean withQoS) throws IsmTestException {
	    return getMessage(withQoS, false);
	}

	public Object getMessage(boolean withQoSOption, boolean withCopy) throws IsmTestException {
		Object result = message;
		if (withQoSOption || withCopy) {
			String messageText = withQoSOption ? this.getPayload() + " at QoS="+this.getQos() : this.getPayload();
			switch (msgType) {
			case MQMQTT:
			{
				MqttMessage newMsg = new MqttMessage();
				newMsg.setQos(this.getQos());
				newMsg.setRetained(this.isRetained());
				if (withQoSOption)				    
				    newMsg.setPayload(messageText.getBytes(StandardCharsets.UTF_8));
				else
				    newMsg.setPayload(this.getPayloadBytes());
				result = newMsg;
			}
				break;
			case PAHO:
			{
				org.eclipse.paho.client.mqttv3.MqttMessage newMsg
					= new org.eclipse.paho.client.mqttv3.MqttMessage();
				newMsg.setQos(this.getQos());
				newMsg.setRetained(this.isRetained());
				if (withQoSOption)                  
                    newMsg.setPayload(messageText.getBytes(StandardCharsets.UTF_8));
                else
                    newMsg.setPayload(this.getPayloadBytes());
				result = newMsg;
			}
				break;
			case PAHOV5:
			{
				org.eclipse.paho.mqttv5.common.MqttMessage newMsg
					= new org.eclipse.paho.mqttv5.common.MqttMessage();
				MqttProperties prp = new MqttProperties();
				MqttProperties oldprp = this.getMqttProperties();
				if (oldprp != null) {
				    prp.setPayloadFormat(oldprp.getPayloadFormat());
				    prp.setMessageExpiryInterval(oldprp.getMessageExpiryInterval());
				    prp.setContentType(oldprp.getContentType());
//				    if(oldprp.getResponseTopic() != null){
				    	prp.setResponseTopic(oldprp.getResponseTopic());				    	
//				    }
				    prp.setCorrelationData(oldprp.getCorrelationData());
				    prp.setUserProperties(oldprp.getUserProperties());
				}    
				newMsg.setProperties(prp);
				newMsg.setQos(this.getQos());
				newMsg.setRetained(this.isRetained());
				if (withQoSOption)                  
                    newMsg.setPayload(messageText.getBytes(StandardCharsets.UTF_8));
                else
                    newMsg.setPayload(this.getPayloadBytes());
				result = newMsg;
			}
				break;
			case JSON:
			{
				JsonMessage newMsg = new JsonMessage();
				newMsg.setBody(jsonMessage.getJsonMessage().getPayload());
				newMsg.setQos(jsonMessage.getJsonMessage().getQoS());
				newMsg.setRetain(jsonMessage.getJsonMessage().getRetain());
				newMsg.setTopic(jsonMessage.getJsonMessage().getTopic());
				newMsg.setType(jsonMessage.getJsonMessage().getType());
				result = newMsg;
			}
			    break;
			}
		}
		return result;
	}

	public int getType() {
		return dataType;
	}

	public void clearPayload() {
		switch (msgType) {
		case MQMQTT:
			mqMqttMessage.clearPayload();
			break;
		case PAHO:
			pahoMqttMessage.clearPayload();
			break;
		case PAHOV5:
			pahoMqttMessagev5.clearPayload();
			break;
		case JSON:
			if (null != jsonMessage.getJsonMessage())
				jsonMessage.getJsonMessage().setBody((String)null);
			((JsonMessageSend)jsonMessage).setBody("");
			break;
		case ISMMQTT:
			((IsmMqttMessage)message).setPayload((byte[])null);
			break;
		case ISMWS:
			switch(((IsmWSMessage)message).getType()) {
			case IsmWSMessage.BINARY:
				message = new IsmWSMessage(null, 0, 0, IsmWSMessage.BINARY);
				break;
			case IsmWSMessage.TEXT:
				message = new IsmWSMessage(null);
			}
			break;
			
		}
	}

	/*
	 * Get the payload as a string 
	 */
	public String getPayload() /*throws IsmTestException*/ {
		switch (msgType) {
		case MQMQTT:
			return new String(mqMqttMessage.getPayload(), StandardCharsets.UTF_8);
		case PAHO:
			return new String(pahoMqttMessage.getPayload(),  StandardCharsets.UTF_8);
		case PAHOV5:
			return new String(pahoMqttMessagev5.getPayload(),  StandardCharsets.UTF_8);
		case JSON:
			return jsonMessage.getJsonMessage().getPayload();
		case ISMMQTT:
			return ((IsmMqttMessage)message).getPayload();
		case ISMWS:
			return ((IsmWSMessage)message).getPayload();
		default:
			return null;
		}
	}

	/*
	 * Get the payload as a byte array
	 */
	public byte[] getPayloadBytes() throws IsmTestException {
		switch (msgType) {
		case MQMQTT:
			return mqMqttMessage.getPayload();
		case PAHO:
			return pahoMqttMessage.getPayload();
		case PAHOV5:
			return pahoMqttMessagev5.getPayload();
		case JSON:
			return jsonMessage.getJsonMessage().getPayloadBytes();
		case ISMMQTT:
			return ((IsmMqttMessage)message).getPayloadBytes();
		case ISMWS:
			return ((IsmWSMessage)message).getPayloadBytes();
		default:
			return null;
		}
	}

	public int getQos() throws IsmTestException {
		switch (msgType) {
		case MQMQTT:
			return mqMqttMessage.getQos();
		case PAHO:
			return pahoMqttMessage.getQos();
		case PAHOV5:
			return pahoMqttMessagev5.getQos();
		case JSON:
			return jsonMessage.getJsonMessage().getQoS();
		case ISMMQTT:
			return ((IsmMqttMessage)message).getQoS();
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+4),
					"getQos not supported for message type "+getMsgType());
		}
	}

	public boolean isDuplicate() throws IsmTestException {
		switch (msgType) {
		case MQMQTT:
			return mqMqttMessage.isDuplicate();
		case PAHO:
			return pahoMqttMessage.isDuplicate();
		case PAHOV5:
			return pahoMqttMessagev5.isDuplicate();
		case ISMMQTT:
			return ((IsmMqttMessage)message).getDup();
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+5),
					"isDuplicate not supported for message type "+getMsgType());
		}
	}

	public boolean isRetained() throws IsmTestException {
		switch (msgType) {
		case MQMQTT:
			return mqMqttMessage.isRetained();
		case PAHO:
			return pahoMqttMessage.isRetained();
		case PAHOV5:
			return pahoMqttMessagev5.isRetained();
		case JSON:
			return jsonMessage.getJsonMessage().getRetain();
		case ISMMQTT:
			return ((IsmMqttMessage)message).getRetain();
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+6),
					"isRetained not supported for message type "+getMsgType());
		}
	}

	public void setPayload(byte[] payload) {
		switch (msgType) {
		case MyConnection.MQMQTT:
			mqMqttMessage.setPayload(payload);
			break;
		case MyConnection.PAHO:
			pahoMqttMessage.setPayload(payload);
			break;
		case MyConnection.PAHOV5:
			pahoMqttMessagev5.setPayload(payload);
			break;
		case MyConnection.JSONTCP:
		case MyConnection.JSONWS:
			jsonMessage.getJsonMessage().setBody(payload);
			break;
		case MyConnection.ISMMQTT:
			((IsmMqttMessage)message).setPayload(payload);
			dataType = IsmWSMessage.BINARY;   
			break;
		case MyConnection.ISMWS:
			message = new IsmWSMessage(payload, 0, payload.length, dataType);
			break;
		}
	}

	public void setQos(int qos) throws IsmTestException {
		switch (msgType) {
		case MyConnection.MQMQTT:
			mqMqttMessage.setQos(qos);
			break;
		case MyConnection.PAHO:
			pahoMqttMessage.setQos(qos);
			break;
		case MyConnection.PAHOV5:
			pahoMqttMessagev5.setQos(qos);
			break;
		case MyConnection.JSONTCP:
		case MyConnection.JSONWS:
			jsonMessage.getJsonMessage().setQos(qos);
			((JsonMessageSend)jsonMessage).setQoS(qos);
			break;
		case MyConnection.ISMMQTT:
			((IsmMqttMessage)message).setQos(qos);
			break;
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+7),
					"setQos not supported for message type "+getMsgType());
		}
	}

	public void setRetained(boolean retained) throws IsmTestException {
		switch (msgType) {
		case MQMQTT:
			mqMqttMessage.setRetained(retained);
			break;
		case PAHO:
			pahoMqttMessage.setRetained(retained);
			break;
		case PAHOV5:
			pahoMqttMessagev5.setRetained(retained);
			break;
		case JSON:
			jsonMessage.getJsonMessage().setRetain(retained);
			((JsonMessageSend)jsonMessage).setRetain(retained ? 1 : 0);
		case ISMMQTT:
			((IsmMqttMessage)message).setRetain(retained);
			break;
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
					"setRetained not supported for message type "+getMsgType());
		}
	}

	/*
	 * Set the Content Type property of a message (MQTTv5)
	 */
    public void setContentType(String contentType) throws IsmTestException {
        switch (msgType) {
        case ISMMQTT:
            ((IsmMqttMessage)message).setContentType(contentType);
            break;
        case MQMQTT:
        case PAHO:
        case JSON:
            break;
        case PAHOV5:
            pahov5props.setContentType(contentType);
            break;
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "setContentType is not supported for message type "+getMsgType());
        }
    }
    
    /*
     * Get the Content Type property of a message (MQTTv5)
     */
    public String getContentType() throws IsmTestException {
        switch (msgType) {
        case ISMMQTT:
            return ((IsmMqttMessage)message).getContentType();
        case MQMQTT:
        case JSON:
        case PAHO:
            return null;  /* Does not support MQTTv5 */
        case PAHOV5:
        	if (receivedv5props != null){
        		return receivedv5props.getContentType();
        	} else {
        		return pahov5props.getContentType();        		
        	}
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "getContentType is not supported for message type "+getMsgType());
        }
    }

    /*
     * Set the Message Expiry property of a message (MQTTv5)
     */
    public void setExpiry(long expiry) throws IsmTestException {
        switch (msgType) {
        case ISMMQTT:
            ((IsmMqttMessage)message).setExpire(expiry);
            break;
        case MQMQTT:
        case JSON:
        case PAHO:
            break;   /* Does not support MQTTv5 */
        case PAHOV5:
            if (expiry >= 0)
                pahov5props.setMessageExpiryInterval(expiry);
            else
                pahov5props.setMessageExpiryInterval(null);
            break;
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "setExpriy is not supported for message type "+getMsgType());
        }
    }
    
    /*
     * Get the Content Type property of a message (MQTTv5)
     */
    public long getExpiry() throws IsmTestException {
        switch (msgType) {
        case ISMMQTT:
            return ((IsmMqttMessage)message).getExpire();
        case MQMQTT:
        case PAHO:
        case JSON:
            return 0;    /* Does not support MQTTv5 */
        case PAHOV5:   
            Long expire;
        	if (receivedv5props != null) {
            	expire = receivedv5props.getMessageExpiryInterval();
            } else {
            	expire = pahov5props.getMessageExpiryInterval();
            }
        	
            if (expire == null)
                return -1;
            else
                return ((long)expire) & 0xffffffff;
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "getExpiry is not supported for message type "+getMsgType());
        }
    }

    public Integer getTopicAlias() throws IsmTestException {
    	switch (msgType) {
    	case ISMMQTT:
    		//TODO
    	case MQMQTT:
    	case PAHO:
    	case JSON:
    		return 0; /* Does not support MQTTv5 */
    	case PAHOV5:
    		if (receivedv5props != null) {
    			return receivedv5props.getTopicAlias();
    		} else {
    			return pahov5props.getTopicAlias();
    		}
    	default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "getTopicAlias is not supported for message type "+getMsgType());
    	}
    }
    
    public void setTopicAlias(int alias) throws IsmTestException {
        switch (msgType) {
        case ISMMQTT:
//            ((IsmMqttMessage)message).setReplyTo(replyto);
        	// TODO
            break;
        case MQMQTT:
        case PAHO:
        case JSON:
            break;    /* No support for MQTTv5 */

        case PAHOV5:
            pahov5props.setTopicAlias(alias);
            break;
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "setTopicAlias is not supported for message type "+getMsgType());
        }
    }
    
    
    
    /*
     * Set the User Properties of a message (MQTTv5)
     * The input props might be null
     */
    public void setUserProperties(List<MqttUserProp> props) throws IsmTestException {
        switch (msgType) {
        case ISMMQTT:
            if (props != null) {
                Iterator<MqttUserProp> it = props.iterator();
                while (it.hasNext()) {
                    MqttUserProp prp = it.next();
                    ((IsmMqttMessage)message).addUserProperty(prp.getKey(), prp.getValue());
                }
            }
            break;
            
        case MQMQTT:
        case JSON:
        case PAHO:
            break;   /* No supportt for MQTTv5 */
            
        case PAHOV5:
            if (props != null && props.size() > 0) {
                List<UserProperty> v5props = new Vector<UserProperty>();
                Iterator<MqttUserProp> it = props.iterator();
                while (it.hasNext()) {
                    MqttUserProp prp = it.next();
                    v5props.add(new UserProperty(prp.getKey(), prp.getValue()));
                }
                pahov5props.setUserProperties(v5props);
            }
        	break;
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "setUserProperties is not supported for message type "+getMsgType());
        }
    }
    
    /*
     * Get the list of user properties which can be null.
     */
    public List<MqttUserProp> getUserProperties() throws IsmTestException{
        switch (msgType) {
        case ISMMQTT:
            /* Convert from ISM user props to test driver user props */
            List<IsmMqttUserProperty> mprops = ((IsmMqttMessage)message).getUserProperties();
            if (mprops == null) 
                return null;
            List<MqttUserProp> props = new Vector<MqttUserProp>();
            Iterator<IsmMqttUserProperty> it = mprops.iterator();
            while (it.hasNext()) {
                IsmMqttUserProperty prp = it.next();
                props.add(new MqttUserProp(prp.name(), prp.value()));
            }
            return props;
            
        case MQMQTT:
        case PAHO:
        case JSON:
            return null;   /* These do not support MQTTv5 */
            
        case PAHOV5:   
        	List<UserProperty> v5props = null;
        	if (receivedv5props != null){
        		v5props = receivedv5props.getUserProperties();        		
        	} else {
        		v5props = pahov5props.getUserProperties();
        	}
        	
        	
            if (v5props != null && v5props.size() > 0) {
                List<MqttUserProp> xprops = new Vector<MqttUserProp>();       
                Iterator<UserProperty> itx = v5props.iterator();
                while (itx.hasNext()) {
                    UserProperty prp = itx.next();
                    xprops.add(new MqttUserProp(prp.getKey(), prp.getValue()));
                }
                return xprops;
            } else {
                return null;
            }
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "getUserProperties is not supported for message type "+getMsgType());
        }
    }

    /*
     * Set the Response Topic of a message (MQTTv5)
     */
    public void setResponseTopic(String replyto) throws IsmTestException {
        switch (msgType) {
        case ISMMQTT:
            ((IsmMqttMessage)message).setReplyTo(replyto);
            break;
        case MQMQTT:
        case PAHO:
        case JSON:
            break;    /* No support for MQTTWv5 */

        case PAHOV5:
            pahov5props.setResponseTopic(replyto);
            break;
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "setResponseTopic is not supported for message type "+getMsgType());
        }
    }
    
    /*
     * Get the Response Topic of a message (MQTTv5)
     */
    public String getResponseTopic() throws IsmTestException {
        switch (msgType) {
        case ISMMQTT:
            return ((IsmMqttMessage)message).getReplyTo();
        case MQMQTT:
        case PAHO:
        case JSON:
            return null;
        case PAHOV5:
        	if( receivedv5props != null){
        		return receivedv5props.getResponseTopic();
        	} else{
        		return pahov5props.getResponseTopic();        		
        	}
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "getResponseTopic is not supported for message type "+getMsgType());
        }
    }
    
    /*
     *  Get the Subscription Identifier of a message (MQTTv5)
     */
    
    public List<Integer> getSubscriptionIdentifiers() throws IsmTestException {
    	switch (msgType) {
    	case ISMMQTT:
    	case MQMQTT:
    	case PAHO:
    	case JSON:
    		return null;
    	case PAHOV5:
    		if (receivedv5props != null){
    			return receivedv5props.getSubscriptionIdentifiers();
    		} else{
    			return pahov5props.getSubscriptionIdentifiers();    			
    		}
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
					"getSubscriptionIdentifiers is not supported for message type " + getMsgType());
    	}
    }
    
    /*
     * Set the Payload Format Indicator of a message (MQTTv5)
     */
    
    public void setPayloadFormatIndicator(boolean payloadFormatIndicator) throws IsmTestException {
        switch (msgType) {
        case ISMMQTT:
            ((IsmMqttMessage)message).setMessageType(payloadFormatIndicator ? IsmMqttMessage.TextMessage : IsmMqttMessage.BytesMessage);
            break;
        case MQMQTT:
        case PAHO:
        case JSON:
            break;    /* No support for MQTTv5 */

        case PAHOV5:
            pahov5props.setPayloadFormat(payloadFormatIndicator);
            break;
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "setPayloadFormatIndicator is not supported for message type "+getMsgType());
        }
    }
    
    /*
     * Get the Payload Format Indicator of a message (MQTTv5)
     */
    public Boolean getPayloadFormatIndicator() throws IsmTestException {
        switch (msgType) {
        case ISMMQTT:
        case MQMQTT:
        case PAHO:
        case JSON:
            return null;
        case PAHOV5:
        	if( receivedv5props != null){
        		return receivedv5props.getPayloadFormat();
        	} else{
        		return pahov5props.getPayloadFormat();        		
        	}
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "getPayloadFormatIndicator is not supported for message type "+getMsgType());
        }
    }
    
    
    /*
     * Set the Subscription Identifier of a message (MQTTv5)
     */
    public void setSubscriptionIdentifiers(List<Integer> subscriptionIdentifiers) throws IsmTestException {
        switch (msgType) {
        case ISMMQTT:
        case MQMQTT:
        case PAHO:
        case JSON:
            break;    /* No support for MQTTWv5 */

        case PAHOV5:
            pahov5props.setSubscriptionIdentifiers(subscriptionIdentifiers);
            break;
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "setSubscriptionIdentifiers is not supported for message type "+getMsgType());
        }
    }
    

    /*
     * Set the CoorlationData of a message as a Sgring (MQTTv5)
     */
    public void setCorrelationData(String coor) throws IsmTestException {
        switch (msgType) {
        case ISMMQTT:
            ((IsmMqttMessage)message).setCorrelation(coor);
            break;
        case MQMQTT:
        case PAHO:
        case JSON:
            break;
        case PAHOV5:
            if (coor != null) {
                pahov5props.setCorrelationData(coor.getBytes(StandardCharsets.UTF_8));
            }    
            break;
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "setCorrelationData is not supported for message type "+getMsgType());
        }
    }
    
    /*
     * Set the CoorlationData of a message as a Sgring (MQTTv5)
     */
    public String getCorrelationData() throws IsmTestException {
        switch (msgType) {
        case ISMMQTT:
            return ((IsmMqttMessage)message).getCorrelationString();
            
        case MQMQTT:
        case PAHO:
        case JSON:
            return null;
        case PAHOV5:
        	byte [] corr = null;
        	if(receivedv5props != null){
        		corr = receivedv5props.getCorrelationData();
        	} else {
        		corr = pahov5props.getCorrelationData();        		
        	}
            if (corr == null)
                return null;
            return new String(corr, StandardCharsets.UTF_8);
        default:
            throw new IsmTestException("ISMTEST"+(Constants.MYMESSAGE+8),
                    "getCorrelationData is not supported for message type "+getMsgType());
        }
    }
    
	private String getMsgType() {
		switch (msgType) {
		case MQMQTT:
			return "MQ client MQTT message";
		case PAHO:
			return "PAHO client MQTT message";
		case PAHOV5:
			return "PAHOv5 client MQTT message";
		case JSON:
			return "JSON message";
		case ISMMQTT:
			return "IsmClient MQTT message";
		case ISMWS:
			return "WebSocket message";
		default:
			return "Unknown message type " + msgType;
		}
	}
	
	public void setTopic(String topic) {
		this.topic = topic;
		if (ISMMQTT == msgType) {
			((IsmMqttMessage)message).setTopic(topic);
		} else if (JSON == msgType) {
			jsonMessage.getJsonMessage().setTopic(topic);
			((JsonMessageSend)jsonMessage).setTopic(topic);
		}
	}
	
	public String getTopic() {
		return topic;
	}
	
	public void setIncludeQoS(boolean value) {
		includeQoS = value;
	}
	
	public boolean isIncludeQoS() {
		return includeQoS;
	}
	
	public void setIncrementing(boolean value) {
		incrementing = value;
	}
	
	public boolean isIncrementing() {
		return incrementing;
	}
	
	public void incrementMessage() {
		if (incrementing) {
			counter++;
			this.setPayload((messageText+counter).getBytes());
		}
	}
	
	public int getCounter(){
		return counter;
	}
	
	public MqttTopic getMqttTopic() {
		return mqMqttTopic;
	}
	
	public String toString() {
		String result = "Message:";
		result += "\n\ttype="+getMsgType();
		if (null != topic) {
			result += "\n\ttopic="+topic;
		}
		switch (msgType) {
		case MQMQTT:
		case PAHO:
			try {
				result += "\n\tQoS="+this.getQos();
			} catch (IsmTestException e) {}
			break;
		case PAHOV5:
			try{
				result += "\n\tQoS="+this.getQos();
			} catch (IsmTestException e) {}
			break;
		}
		byte[] data = null;
		try {
			data = this.getPayloadBytes();
			result += "\n\tbody length="+data.length;
			result += "\n\tbody="+TestUtils.bytesToHex(data);
			try {
				result += "\n\tbody as text='"+(new String(data))+"'";
			} catch (Exception e) {}
		} catch (IsmTestException e) {
			result += "\n\t"+e.getMessage();
		}
		return result;
	}

}
