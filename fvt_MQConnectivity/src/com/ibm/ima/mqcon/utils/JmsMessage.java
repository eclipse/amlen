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
package com.ibm.ima.mqcon.utils;

import java.util.UUID;

import javax.jms.BytesMessage;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.Session;
import javax.jms.StreamMessage;

/**
 * Helper class for creating different JMS messages.
 */
public class JmsMessage {
	
	public static String userLongProperty = "LongProperty";
	public static String userObjectProperty = "ObjectProperty";
	public static String userBooleanProperty = "BooleanProperty";
	public static String userByteProperty = "ByteProperty";
	public static String userDoubleProperty = "DoubleProperty";
	public static String userFloatProperty = "FloatProperty";
	public static String userIntProperty = "IntProperty";
	public static String userShortProperty = "ShortProperty";
	public static String userStringProperty = "StringProperty";
	public static String imaTopic = "ImaTopic:";
	public static String mqTopic = "topic://";
	
	public static enum MESSAGE_TYPE {TEXT, STREAM, MAP, BYTES, OBJECT, BASE}
	
	/**
	 * This method can be used to create a JMS message of specified type using random content
	 * @param session - the JMS session from which to create the message
	 * @param type - the JMS message type
	 * @return JMS message
	 * @throws JMSException should we encounter an exception creating a JMS message from the session
	 */
	public static Message createMessageBody(Session session, MESSAGE_TYPE type) throws JMSException
	{
		Message msg = null;
		if(type.equals(MESSAGE_TYPE.TEXT))
		{
			String messageBody = UUID.randomUUID().toString().substring(0, 30);
			msg = session.createTextMessage(messageBody);
			
			if(Trace.traceOn())
			{
				Trace.trace("JMS Text Message generated with contents=" + messageBody);
			}
			
		}
		else if (type.equals(MESSAGE_TYPE.STREAM))
		{
			String messageBody = UUID.randomUUID().toString().substring(0, 30);
			msg = session.createStreamMessage();
			((StreamMessage)msg).writeBytes(messageBody.getBytes());
			
			if(Trace.traceOn())
			{
				Trace.trace("JMS Stream Message generated with contents=" + messageBody);
			}
			
		}
		else if (type.equals(MESSAGE_TYPE.MAP))
		{
			msg = session.createMapMessage();
			String keyA = UUID.randomUUID().toString().substring(0, 30);
			String keyAValue = UUID.randomUUID().toString().substring(0, 30);
			String keyB = UUID.randomUUID().toString().substring(0, 30);
			String keyBValue = UUID.randomUUID().toString().substring(0, 30);
			((MapMessage)msg).setString(keyA, keyAValue);
			((MapMessage)msg).setString(keyB, keyBValue);
			
			if(Trace.traceOn())
			{
				Trace.trace("JMS Map Message generated with name value pairs=" + keyA + ":" + keyAValue + " " + keyB + ":" + keyBValue);
			}
		}
		else if (type.equals(MESSAGE_TYPE.BYTES))
		{
			String messageBodyText = UUID.randomUUID().toString().substring(0, 30);
			
			msg = session.createBytesMessage();
			((BytesMessage)msg).writeBytes(messageBodyText.getBytes());
			
			if(Trace.traceOn())
			{
				Trace.trace("JMS Bytes Message generated with contents=" + messageBodyText);
			}
		}
		else if (type.equals(MESSAGE_TYPE.OBJECT))
		{
			PersonObject person = new PersonObject();
			msg = session.createObjectMessage(person);
			
			if(Trace.traceOn())
			{
				Trace.trace("JMS Object Message generated with contents=" + person.toString());
			}
		}
		else if (type.equals(MESSAGE_TYPE.BASE))
		{
			msg = session.createMessage();
			if(Trace.traceOn())
			{
				Trace.trace("JMS Base Message generated");
			}
		}
		
		return msg;
	}
	
	
	
	/**
	 * This method can be used to create a JMS message of specified type using random content including a number of
	 * user header properties.
	 * @param session - the JMS session from which to create the message
	 * @param type - the JMS message type
	 * @return JMS message
	 * @throws JMSException should we encounter an exception creating a JMS message from the session
	 */
	public static Message createMessage(Session session, MESSAGE_TYPE type) throws JMSException
	{
		Message msg = null;
		if(type.equals(MESSAGE_TYPE.TEXT))
		{
			String messageBody = UUID.randomUUID().toString().substring(0, 30);
			msg = session.createTextMessage(messageBody);
			if(Trace.traceOn())
			{
				Trace.trace("JMS Text Message generated with contents=" + messageBody);
			}
			
		}
		else if (type.equals(MESSAGE_TYPE.STREAM))
		{
			String messageBody = UUID.randomUUID().toString().substring(0, 30);
			msg = session.createStreamMessage();
			((StreamMessage)msg).writeString(messageBody);
			if(Trace.traceOn())
			{
				Trace.trace("JMS Stream Message generated with contents=" + messageBody);
			}
			
		}
		else if (type.equals(MESSAGE_TYPE.MAP))
		{
			msg = session.createMapMessage();
			String keyA = UUID.randomUUID().toString().substring(0, 30);
			String keyAValue = UUID.randomUUID().toString().substring(0, 30);
			String keyB = UUID.randomUUID().toString().substring(0, 30);
			String keyBValue = UUID.randomUUID().toString().substring(0, 30);
			((MapMessage)msg).setString(keyA, keyAValue);
			((MapMessage)msg).setString(keyB, keyBValue);
			
			if(Trace.traceOn())
			{
				Trace.trace("JMS Map Message generated with name value pairs=" + keyA + ":" + keyAValue + " " + keyB + ":" + keyBValue);
			}
		}
		else if (type.equals(MESSAGE_TYPE.BYTES))
		{
			String messageBody = UUID.randomUUID().toString().substring(0, 30);
			msg = session.createBytesMessage();
			((BytesMessage)msg).writeUTF(messageBody);
			
			if(Trace.traceOn())
			{
				Trace.trace("JMS Bytes Message generated with contents=" + messageBody);
			}
		}
		else if (type.equals(MESSAGE_TYPE.OBJECT))
		{
			PersonObject person = new PersonObject();
			msg = session.createObjectMessage(person);
			
			if(Trace.traceOn())
			{
				Trace.trace("JMS Object Message generated with contents=" + person.toString());
			}
		}
		else if (type.equals(MESSAGE_TYPE.BASE))
		{
			msg = session.createMessage();
			if(Trace.traceOn())
			{
				Trace.trace("JMS Base Message generated");
			}
		}
			
		msg.setLongProperty(userLongProperty, 1234567890);
		msg.setObjectProperty(userObjectProperty, UUID.randomUUID().toString().substring(0, 10));
		msg.setBooleanProperty(userBooleanProperty, true);
		msg.setByteProperty(userByteProperty, (byte) 10);
		msg.setDoubleProperty(userDoubleProperty, 11);
		msg.setFloatProperty(userFloatProperty, (float) 1.2345);
		msg.setIntProperty(userIntProperty, 1234);
		msg.setShortProperty(userShortProperty, (short) 33);
		msg.setStringProperty(userStringProperty, UUID.randomUUID().toString().substring(0, 10));
		
		if(Trace.traceOn())
		{
			Trace.trace("JMS User Properties: LongProperty=1234567890 ObjectProperty=" + msg.getObjectProperty(userObjectProperty) + " BooleanProperty=true ByteProperty=10 DoubleProperty=11 FloatProperty=1.2345 IntProperty=1234 ShortProperty=33 StringProperty=" + msg.getStringProperty(userStringProperty));
		}
		
		return msg;
	}
	
	
	
	/**
	 * This method can be used to create a JMS message of specified type using random content including a number of
	 * user header properties and user defined JMS(X) header properties
	 * @param session - the JMS session from which to create the message
	 * @param type - the JMS message type
	 * @return JMS message
	 * @throws JMSException should we encounter an exception creating a JMS message from the session
	 */
	public static Message createMessageJMSProps(Session session, MESSAGE_TYPE type) throws JMSException
	{
		Message msg = null;
		if(type.equals(MESSAGE_TYPE.TEXT))
		{
			String messageBody = UUID.randomUUID().toString().substring(0, 30);
			msg = session.createTextMessage(messageBody);
			if(Trace.traceOn())
			{
				Trace.trace("JMS Text Message generated with contents=" + messageBody);
			}
			
		}
		else if (type.equals(MESSAGE_TYPE.STREAM))
		{
			String messageBody = UUID.randomUUID().toString().substring(0, 30);
			msg = session.createStreamMessage();
			((StreamMessage)msg).writeString(messageBody);
			if(Trace.traceOn())
			{
				Trace.trace("JMS Stream Message generated with contents=" + messageBody);
			}
			
		}
		else if (type.equals(MESSAGE_TYPE.MAP))
		{
			msg = session.createMapMessage();
			String keyA = UUID.randomUUID().toString().substring(0, 30);
			String keyAValue = UUID.randomUUID().toString().substring(0, 30);
			String keyB = UUID.randomUUID().toString().substring(0, 30);
			String keyBValue = UUID.randomUUID().toString().substring(0, 30);
			((MapMessage)msg).setString(keyA, keyAValue);
			((MapMessage)msg).setString(keyB, keyBValue);
			
			if(Trace.traceOn())
			{
				Trace.trace("JMS Map Message generated with name value pairs=" + keyA + ":" + keyAValue + " " + keyB + ":" + keyBValue);
			}
		}
		else if (type.equals(MESSAGE_TYPE.BYTES))
		{
			String messageBody = UUID.randomUUID().toString().substring(0, 30);
			msg = session.createBytesMessage();
			((BytesMessage)msg).writeUTF(messageBody);
			
			if(Trace.traceOn())
			{
				Trace.trace("JMS Bytes Message generated with contents=" + messageBody);
			}
		}
		else if (type.equals(MESSAGE_TYPE.OBJECT))
		{
			PersonObject person = new PersonObject();
			msg = session.createObjectMessage(person);
			
			if(Trace.traceOn())
			{
				Trace.trace("JMS Object Message generated with contents=" + person.toString());
			}
		}
		else if (type.equals(MESSAGE_TYPE.BASE))
		{
			msg = session.createMessage();
			if(Trace.traceOn())
			{
				Trace.trace("JMS Base Message generated");
			}
		}
			
		msg.setLongProperty(userLongProperty, 1234567890);
		msg.setObjectProperty(userObjectProperty, UUID.randomUUID().toString().substring(0, 10));
		msg.setBooleanProperty(userBooleanProperty, true);
		msg.setByteProperty(userByteProperty, (byte) 10);
		msg.setDoubleProperty(userDoubleProperty, 11);
		msg.setFloatProperty(userFloatProperty, (float) 1.2345);
		msg.setIntProperty(userIntProperty, 1234);
		msg.setShortProperty(userShortProperty, (short) 33);
		msg.setStringProperty(userStringProperty, UUID.randomUUID().toString().substring(0, 10));
		
		if(Trace.traceOn())
		{
			Trace.trace("JMS User Properties: LongProperty=1234567890 ObjectProperty=" + msg.getObjectProperty(userObjectProperty) + " BooleanProperty=true ByteProperty=10 DoubleProperty=11 FloatProperty=1.2345 IntProperty=1234 ShortProperty=33 StringProperty=" + msg.getStringProperty(userStringProperty));
		}
		
		String tempDestName = UUID.randomUUID().toString().substring(0, 10);
		Destination tempDest = session.createTopic(tempDestName);
		msg.setJMSReplyTo(tempDest);
		msg.setJMSCorrelationID(UUID.randomUUID().toString().substring(0, 8));
		msg.setStringProperty("JMSXGroupID", UUID.randomUUID().toString().substring(0, 5));
		msg.setIntProperty("JMSXGroupSeq", ((new Integer(4)).intValue()));
		
		if(Trace.traceOn())
		{
			Trace.trace("JMS Properties: JMSReplyTo=" + tempDestName + " JMSCorrelationID=" + msg.getJMSCorrelationID() + " JMSXGroupID=" + msg.getStringProperty("JMSXGroupID") + " JMSXGroupSeq= " + msg.getIntProperty("JMSXGroupSeq"));
		}
		
		return msg;
	}

}
