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

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.util.Enumeration;

import javax.jms.BytesMessage;
import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.ObjectMessage;
import javax.jms.StreamMessage;
import javax.jms.TextMessage;

import org.eclipse.paho.client.mqttv3.MqttMessage;

import com.ibm.ima.jms.impl.ImaBytesMessage;
import com.ibm.ima.jms.impl.ImaStreamMessage;
import com.ibm.ima.mqcon.msgconversion.ReturnCodes;
import com.ibm.jms.JMSBytesMessage;
import com.ibm.jms.JMSStreamMessage;

public class CompareMessageContents {
	
	/**
	 * This method can be used to simply compare JMS message bodies for content equality
	 * 
	 * Note: For Bytes messages we only expect a string to be the contents
	 * Note: For Object messages we only expect a PersonObject to be the contents
	 * Note: For Stream messages we only expect a string to be the contents
	 * 
	 * @param expected
	 * @param actual
	 * @return true of contents is deemed to be equal; false otherwise
	 * @throws JMSException
	 */
	public static boolean checkJMSMessagebodyEquality(Message expected, Message actual) throws JMSException
	{
		boolean equality = false;
		if(expected instanceof TextMessage)
		{
			if(! (actual instanceof TextMessage))
			{
				if(Trace.traceOn())
				{
					Trace.trace("The actual message was not the expected type. Expecting a text message when actual message was " + actual.getClass());
					
				}
				equality = false;
			}
			else
			{
				String expectedBody = ((TextMessage)expected).getText();
				String actualBody = ((TextMessage)actual).getText();
				
				if(Trace.traceOn())
				{
					Trace.trace("Expected body=" + expectedBody +  " Actual body=" + actualBody);
					
				}
				equality = expectedBody.equals(actualBody);
			}
		}
		else if(expected instanceof BytesMessage)
		{
			if(! (actual instanceof BytesMessage))
			{
				if(Trace.traceOn())
				{
					Trace.trace("The actual message was not the expected type. Expecting a bytes message when actual message was " + actual.getClass());
				
				}
				equality = false;
			}
			else
			{

				int TEXT_LENGTH = new Long(((BytesMessage)actual).getBodyLength()).intValue();
				byte[] textBytes = new byte[TEXT_LENGTH];
				((BytesMessage)actual).readBytes(textBytes, TEXT_LENGTH);
				String actualBody = new String(textBytes);		
				
				if(expected instanceof JMSBytesMessage)
				{
					if(Trace.traceOn())
					{
						Trace.trace("Calling reset");
					
					}
					((JMSBytesMessage)expected).reset();
				}
				if(expected instanceof ImaBytesMessage)
				{
					if(Trace.traceOn())
					{
						Trace.trace("Calling reset");
					
					}
					((ImaBytesMessage)expected).reset();
				}
			
				int TEXT_LENGTH2 = new Long(((BytesMessage)expected).getBodyLength()).intValue();
				byte[] textBytes2 = new byte[TEXT_LENGTH2];
				((BytesMessage)expected).readBytes(textBytes2, TEXT_LENGTH2);
				String expectedBody = new String(textBytes2);		
				
				if(Trace.traceOn())
				{
					Trace.trace("Expected body=" + expectedBody +  " Actual body=" + actualBody);
				}
				equality = expectedBody.equals(actualBody);
			}
		}
		
		else if(expected instanceof StreamMessage)
		{
			if(! (actual instanceof StreamMessage))
			{
				if(Trace.traceOn())
				{
					Trace.trace("The actual message was not the expected type. Expecting a stream message when actual message was " + actual.getClass());
				}
				equality = false;
			}
			else
			{
				if(expected instanceof JMSStreamMessage)
				{
					if(Trace.traceOn())
					{
						Trace.trace("Calling reset");
					
					}
					((JMSStreamMessage)expected).reset();
				}
				if(expected instanceof ImaStreamMessage)
				{
					if(Trace.traceOn())
					{
						Trace.trace("Calling reset");
					
					}
					((ImaStreamMessage)expected).reset();
				}
				
				
					
				String actualBody = ((StreamMessage)actual).readString();
				String expectedBody = ((StreamMessage)expected).readString();
				
				if(Trace.traceOn())
				{
					Trace.trace("Expected body=" + expectedBody +  " Actual body=" + actualBody);
				}
				equality = expectedBody.equals(actualBody);
			}
		}
		
		else if(expected instanceof MapMessage)
		{
			if(! (actual instanceof MapMessage))
			{
				if(Trace.traceOn())
				{
					Trace.trace("The actual message was not the expected type. Expecting a map message when actual message was " + actual.getClass());
				}
				equality = false;
			}
			else
			{
			
				Enumeration<String> iter = ((MapMessage)expected).getMapNames();
				
				while(iter.hasMoreElements())
				{
					
					String expectedKey = (String)iter.nextElement();
					
					String expectedValue = ((MapMessage)expected).getString(expectedKey);
					String actualValue = ((MapMessage)actual).getString(expectedKey);
					
					if(! expectedValue.equals(actualValue))
					{
						if(Trace.traceOn())
						{
							Trace.trace("The actual message did not contained the expected value. Expected Key and Value=" + expectedKey +":" + expectedValue + " Actual Value=" + actualValue);
						}
						equality = false;
						break;
					}
					else
					{
						if(Trace.traceOn())
						{
							Trace.trace("The expected value for key " + expectedKey + " was " + expectedValue + " and the actual value was " + actualValue );
						}
						equality = true;
					}
					
				}
			}
		}
		
		
		else if(expected instanceof ObjectMessage)
		{
			if(! (actual instanceof ObjectMessage))
			{
				if(Trace.traceOn())
				{
					Trace.trace("The actual message was not the expected type. Expecting an object message when actual message was " + actual.getClass());
				}
				equality = false;
			}
			else
			{
				
				PersonObject expectedPersonOjb = (PersonObject)((ObjectMessage)expected).getObject();
				Object actualOjb = ((ObjectMessage)actual).getObject();
				if(actualOjb instanceof PersonObject)
				{
					PersonObject actualPersonOjb = (PersonObject)actualOjb;
					if(expectedPersonOjb.getFirstName().equals(actualPersonOjb.getFirstName()) && (expectedPersonOjb.getLastName().equals(actualPersonOjb.getLastName()))  && (expectedPersonOjb.getLocation().equals(actualPersonOjb.getLocation())) && (expectedPersonOjb.getAge() == actualPersonOjb.getAge()))
					{
						if(Trace.traceOn())
						{
							Trace.trace("A message was received which matched the expected message. Expected: " + expectedPersonOjb.toString() + " Actual: " + actualPersonOjb.toString());
						}
						equality = true;
					}				
					else 
					{
						if(Trace.traceOn())
						{
							Trace.trace("A message was received which did not matched the expected message. Expected: " + expectedPersonOjb.toString() + " Actual: " + actualPersonOjb.toString());
						}
						equality = false;
					}
					
				}
				else
				{
					if(Trace.traceOn())
					{
						Trace.trace("Was expecting a PersonObject but instead we received: " + actualOjb.getClass());
						
					}
					equality = false;
					
				}
				
			}
		}
		
		else if(expected instanceof Message) // wasn't any of the other types so must be just a base message
		{
			if((actual instanceof ObjectMessage) || (actual instanceof TextMessage) || (actual instanceof StreamMessage) || (actual instanceof BytesMessage) || (actual instanceof MapMessage))
			{
				if(Trace.traceOn())
				{
					Trace.trace("The actual message was not the expected type. Expecting an base message when actual message was " + actual.getClass());
					
				}
				equality = false;
			}
			else 
			{
				equality  = true;	
			}
			
		}
		return equality;
	}
	
	
	
	/**
	 * This method can be used to compare JMS message bodies and headers for content equality
	 * 
	 * Note: For Bytes messages we only expect a string to be the contents
	 * Note: For Object messages we only expect a PersonObject to be the contents
	 * Note: For Stream messages we only expect a string to be the contents
	 * 
	 * @param expected
	 * @param actual
	 * @return true of contents is deemed to be equal; false otherwise
	 * @throws JMSException
	 */
	public static boolean checkJMSMessageEquality(Message expected, Message actual, String expectedJmsDestination, String expectedReplyToPrefix) throws JMSException
	{
		boolean equality = false;
		
		// first check the message body
		equality = CompareMessageContents.checkJMSMessagebodyEquality(expected, actual);
		
		if(equality)
		{
			// now compare the user properties headers
			if(((expected.getBooleanProperty(JmsMessage.userBooleanProperty) == (actual.getBooleanProperty(JmsMessage.userBooleanProperty)))
					&& (expected.getLongProperty(JmsMessage.userLongProperty) == actual.getLongProperty(JmsMessage.userLongProperty))
					&& (expected.getObjectProperty(JmsMessage.userObjectProperty).equals(actual.getObjectProperty(JmsMessage.userObjectProperty)))
					&& (expected.getByteProperty(JmsMessage.userByteProperty)== (actual.getByteProperty(JmsMessage.userByteProperty)))
					&& (expected.getDoubleProperty(JmsMessage.userDoubleProperty)== (actual.getDoubleProperty(JmsMessage.userDoubleProperty)))		
					&& (expected.getFloatProperty(JmsMessage.userFloatProperty)== (actual.getFloatProperty(JmsMessage.userFloatProperty)))
					&& (expected.getIntProperty(JmsMessage.userIntProperty)== (actual.getIntProperty(JmsMessage.userIntProperty)))	
					&& (expected.getShortProperty(JmsMessage.userShortProperty)== (actual.getShortProperty(JmsMessage.userShortProperty)))	
					&& (expected.getStringProperty(JmsMessage.userStringProperty).equals(actual.getStringProperty(JmsMessage.userStringProperty)))	
					))
			{
				if(Trace.traceOn())
				{
					Trace.trace("The user properties were equal");
				}
				
				// now check the JMS props
				
				if(expected.getJMSReplyTo() != null)
				{
					String expectedReplyTo = null;
					String actualReplyTo = null;
					
					if(expectedReplyToPrefix != null && expectedReplyToPrefix.equals(JmsMessage.imaTopic))
					{
						String expectedReplyToRaw = expected.getJMSReplyTo().toString().trim();
						expectedReplyTo = expectedReplyToRaw.replaceAll("topic://", "ImaTopic:");
						// Now modify for any conversion across MQ connectivity
						actualReplyTo = actual.getJMSReplyTo().toString().trim();
					}
					else if (expectedReplyToPrefix != null && expectedReplyToPrefix.equals(JmsMessage.mqTopic))
					{
						String expectedReplyToRaw = expected.getJMSReplyTo().toString().trim();
						expectedReplyTo = expectedReplyToRaw.replaceAll("ImaTopic:", "topic://");
						// Now modify for any conversion across MQ connectivity
						actualReplyTo = actual.getJMSReplyTo().toString().trim();
					}
					else
					{
						expectedReplyTo = expected.getJMSReplyTo().toString().trim();
						actualReplyTo = actual.getJMSReplyTo().toString().trim();
					}

					if(actual.getJMSReplyTo() != null && (expectedReplyTo.equals(actualReplyTo)))
					{
						if(Trace.traceOn())
						{
							Trace.trace("JMS ReplyTo matched: Expected " + expectedReplyTo);
						}
					}
					else
					{

						if(Trace.traceOn())
						{
							Trace.trace("The expected JMS ReplyTo was " + expected.getJMSReplyTo() + " but the actual was " + actual.getJMSReplyTo());
								
						}
						equality = false;
						return equality;
					
					}
				}
				
				if(actual.getJMSDestination() != null && actual.getJMSDestination().toString().equals(expectedJmsDestination))
				{
					if(Trace.traceOn())
					{
						Trace.trace("JMS Destination matched: " + expectedJmsDestination);
					}
				}
				else
				{
					if(Trace.traceOn())
					{
						Trace.trace("The expected JMS destination was incorrect:" + actual.getJMSDestination());
						
					}
					equality = false;
					return equality;
				}
				if(expected.getJMSCorrelationID() != null)
				{
					if((actual.getJMSCorrelationID() != null) && expected.getJMSCorrelationID().equals(actual.getJMSCorrelationID()))
					{
						if(Trace.traceOn())
						{
							Trace.trace("JMS Correlation ID matched: " + expected.getJMSCorrelationID());
						}
					}
					else
					{
						if(Trace.traceOn())
						{
							Trace.trace("The expected JMS Correlation was " + expected.getJMSCorrelationID() + " but the actual was " + actual.getJMSCorrelationID());
							
						}
						equality = false;
						return equality;
					}
				}
				if(expected.getJMSDeliveryMode() == actual.getJMSDeliveryMode())
				{
					if(Trace.traceOn())
					{
						Trace.trace("JMS Delivery mode matched: " + expected.getJMSDeliveryMode());
					}
				}
				else
				{
					if(Trace.traceOn())
					{
						Trace.trace("The expected JMS Delivery mode was " + expected.getJMSDeliveryMode() + " but the actual was " + actual.getJMSDeliveryMode());
						
					}
					equality = false;
					return equality;
				}
				
				if(expected.getJMSExpiration() == actual.getJMSExpiration())
				{
					if(Trace.traceOn())
					{
						Trace.trace("JMS Expiration matched: " + expected.getJMSExpiration());
					}
				}
				else
				{
					if(Trace.traceOn())
					{
						Trace.trace("The expected JMS Expiration was " + expected.getJMSExpiration() + " but the actual was " + actual.getJMSExpiration());
						
					}
					equality = false;
					return equality;
				}
				if(actual.getJMSMessageID() != null)
				{
					if(Trace.traceOn())
					{
						Trace.trace("JMS message ID was found: " + actual.getJMSMessageID());
					}
				}
				else
				{
					if(Trace.traceOn())
					{
						Trace.trace("The JMS Message ID was null");
						
					}
					equality = false;
					return equality;
				}
				
				if(expected.getJMSPriority() == actual.getJMSPriority())
				{
					if(Trace.traceOn())
					{
						Trace.trace("JMS Priority matched: " + expected.getJMSPriority());
					}
				}
				else
				{
					if(Trace.traceOn())
					{
						Trace.trace("The expected JMS Priority was " + expected.getJMSPriority() + " but the actual was " + actual.getJMSPriority());
						
					}
					equality = false;
					return equality;
				}
				

				if(expected.getJMSRedelivered() == actual.getJMSRedelivered())
				{
					if(Trace.traceOn())
					{
						Trace.trace("JMS Redelivered matched: " + expected.getJMSRedelivered());
					}
				}
				else
				{
					if(Trace.traceOn())
					{
						Trace.trace("The expected JMS Redelivery was " + expected.getJMSRedelivered() + " but the actual was " + actual.getJMSRedelivered());
						
					}
					equality = false;
					return equality;
				}
				
				if(expected.getJMSType() != null)
				{
					if((actual.getJMSType() != null) && expected.getJMSType().equals(actual.getJMSType()))
					{
						if(Trace.traceOn())
						{
							Trace.trace("JMS Type matched: " + expected.getJMSType());
						}
					}
					else
					{
						if(Trace.traceOn())
						{
							Trace.trace("The expected JMS Type was " + expected.getJMSType() + " but the actual was " + actual.getJMSType());
							
						}
						equality = false;
						return equality;
					}
				}
				
				
				if(expected.getObjectProperty("JMSXGroupID") != null)
				{
					if((actual.getObjectProperty("JMSXGroupID") != null) && expected.getObjectProperty("JMSXGroupID").equals(actual.getObjectProperty("JMSXGroupID")))
					{
						if(Trace.traceOn())
						{
							Trace.trace("JMSXGroupID matched: " + expected.getObjectProperty("JMSXGroupID"));
						}
					}
					else
					{
						if(Trace.traceOn())
						{
							Trace.trace("The expected JMSXGroupID was " + expected.getObjectProperty("JMSXGroupID") + " but the actual was " + actual.getObjectProperty("JMSXGroupID"));
							
						}
						equality = false;
						return equality;
					}
				}
				
				
				if(expected.getObjectProperty("JMSXGroupSeq") != null)
				{
					if((actual.getObjectProperty("JMSXGroupSeq") != null) && expected.getObjectProperty("JMSXGroupSeq").equals(actual.getObjectProperty("JMSXGroupSeq")))
					{
						if(Trace.traceOn())
						{
							Trace.trace("JMSXGroupSeq property matched: " + expected.getObjectProperty("JMSXGroupSeq"));
						}
					}
					else
					{
						if(Trace.traceOn())
						{
							Trace.trace("The expected JMSXGroupSeq was " + expected.getObjectProperty("JMSXGroupSeq") + " but the actual was " + actual.getObjectProperty("JMSXGroupSeq"));
							
						}
						equality = false;
						return equality;
					}
				}
								
			}			
			else
			{
				if(Trace.traceOn())
				{
					Trace.trace("The actual message contained different user properties to those expected. Expected message=" + expected + " Actual message=" +actual);
					
				}
				equality = false;
			}
			
	
			
		}
		
		return equality;
		
	}
	
	public static boolean checkJMSToMQTTEquality(Message expected, MqttMessage actual) throws JMSException
	{
		boolean equality = false;
		if(expected instanceof TextMessage)
		{
			String expectedBody = ((TextMessage)expected).getText();
			String actualBody = actual.toString();
				
			if(Trace.traceOn())
			{
				Trace.trace("Expected body=" + expectedBody +  " Actual body=" + actualBody);
			}
			equality = expectedBody.equals(actualBody);
		}
		else if(expected instanceof BytesMessage)
		{
			try
			{
				if(expected instanceof JMSBytesMessage)
				{
					if(Trace.traceOn())
					{
						Trace.trace("Calling reset");
					
					}
					((JMSBytesMessage)expected).reset();
				}
				if(expected instanceof ImaBytesMessage)
				{
					if(Trace.traceOn())
					{
						Trace.trace("Calling reset");
					
					}
					((ImaBytesMessage)expected).reset();
				}
				
				String expectedBody = ((BytesMessage)expected).readUTF();	
	
				ByteArrayInputStream in = new ByteArrayInputStream(actual.getPayload());
				DataInputStream dataIn = new DataInputStream(in);	
				String actualBody = dataIn.readUTF();
				if(Trace.traceOn())
				{
					Trace.trace("Expected body=" + expectedBody +  " Actual body=" + actualBody);
				}
				equality = expectedBody.equals(actualBody);
			}
			catch(IOException ioe)
			{
				if(Trace.traceOn())
				{
					Trace.trace("An exception occurred whilst attempting the test. Exception=" + ioe.getMessage());
				}
				System.out.println("\nAn exception occurred whilst sending a message. Exception=" + ioe.getMessage());
				System.exit(ReturnCodes.IOE_EXCEPTION);
			}
		}
		else if(expected instanceof StreamMessage)
		{
			
			if(expected instanceof JMSStreamMessage)
			{
				if(Trace.traceOn())
				{
					Trace.trace("Calling reset");
					
				}
				((JMSStreamMessage)expected).reset();
			}
			if(expected instanceof ImaStreamMessage)
			{
				if(Trace.traceOn())
				{
					Trace.trace("Calling reset");
					
				}
				((ImaStreamMessage)expected).reset();
			}
				
			String expectedRawBody = ((StreamMessage)expected).readString();
			String expectedBody = "[\"" + expectedRawBody + "\"]";
			String actualBody = new String(actual.getPayload());
				
			if(Trace.traceOn())
			{
				Trace.trace("Expected body=" + expectedBody +  " Actual body=" + actualBody);
			}
			equality = expectedBody.equals(actualBody);
			if(equality == false)
			{
				// check we dont have a mq style mqtt conversion message
				expectedBody = "<stream><elt>" +expectedRawBody + "</elt></stream>";
				equality = expectedBody.equals(actualBody);
			}
			
		}
		else if(expected instanceof MapMessage)
		{
			
			Enumeration<String> iter = ((MapMessage)expected).getMapNames();		
			// Expecting only two
			
			String expectedKey1 = (String)iter.nextElement();				
			String expectedValue1 = ((MapMessage)expected).getString(expectedKey1);
			String expectedKey2 = (String)iter.nextElement();				
			String expectedValue2 = ((MapMessage)expected).getString(expectedKey2);
			
			String bodyIma = "{\""+ expectedKey1+"\":\""+expectedValue1 + "\",\"" + expectedKey2+"\":\""+expectedValue2 + "\"}";
			
		
		
			String actualBody = new String(actual.getPayload());
		
			if(Trace.traceOn())
			{
				Trace.trace("Expected body=" + bodyIma +  " Actual body=" + actualBody);
			}
			equality = bodyIma.equals(actualBody);
			if(equality == false)
			{
				// check to see if we have an MQ format - note I have checked the map message is being sent to 
				// MQ correctly using the MQ explorer but not sure how to format the MQ conversion so until this is understood
				// I will just check the string contains the expected information
				
				if(actualBody.contains(expectedKey1) && actualBody.contains(expectedValue1) && actualBody.contains(expectedKey2) && actualBody.contains(expectedValue2)){
					
					if(Trace.traceOn())
					{
						Trace.trace("The message was checked for an MQ conversion and passed");
					}
					equality = true;
				}
			}
			
				
			
		}
		else if(expected instanceof ObjectMessage)
		{
			try
			{
				PersonObject jmsperson = (PersonObject)((ObjectMessage)expected).getObject();
				ByteArrayInputStream in = new ByteArrayInputStream(actual.getPayload()); 
				ObjectInputStream is = new ObjectInputStream(in); 
				PersonObject mqttperson = (PersonObject)is.readObject();
				equality = jmsperson.equals(mqttperson);
				if(Trace.traceOn())
				{
					Trace.trace("Expected body=" + jmsperson +  " Actual body=" + mqttperson);
				}
			}
			catch(IOException ioe)
			{
				if(Trace.traceOn())
				{
					Trace.trace("An exception occurred whilst attempting the test. Exception=" + ioe.getMessage());
				}
				System.out.println("\nAn exception occurred whilst sending a message. Exception=" + ioe.getMessage());
				System.exit(ReturnCodes.IOE_EXCEPTION);
			}
			catch(ClassNotFoundException cfe)
			{
				if(Trace.traceOn())
				{
					Trace.trace("An exception occurred whilst attempting the test. Exception=" + cfe.getMessage());
				}
				System.out.println("\nAn exception occurred whilst sending a message. Exception=" + cfe.getMessage());
				System.exit(ReturnCodes.IOE_EXCEPTION);
			}
		}
		else if(expected instanceof Message) // wasn't any of the other types so must be just a base message
		{
			
			if(Trace.traceOn())
			{
				Trace.trace("Expected body length=0 Actual body=" + actual.getPayload().length);
			}
			equality = actual.getPayload().length == 0;
		}
	
		return equality;
	}

}
