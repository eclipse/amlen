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
package com.ibm.ism.mqbridge.test;

import java.util.ArrayList;
import java.util.List;

public class MessageUtilities {

	public MessageUtilities(LoggingUtilities logger) {
		this.logger = logger;
	}
	
	public List<String> getPayloads() {
		return payloads;
	}

	public void setPayloads(List<String> payloads) {
		this.payloads = payloads;
	}

	public List<String> getOriginalPayloads() {
		return originalPayloads;
	}

	public void setOriginalPayloads(List<String> originalPayloads) {
		this.originalPayloads = originalPayloads;
	}

	public final Object payloadsLock = new Object();
	private List<String> payloads = new ArrayList<String>();
	private List<String> originalPayloads = new ArrayList<String>();
	private List<String> extraPayloads = new ArrayList<String>();
	public final String messageBody = "test";
	LoggingUtilities logger;

	public List<String> getExtraPayloads() {
		return extraPayloads;
	}

	public void setExtraPayloads(List<String> extraPayloads) {
		this.extraPayloads = extraPayloads;
	}

	public static String generateUnicodeChars(int start, int end) {
		StringBuffer unicodeString = new StringBuffer();
		for (int count = start; count < end ; count++) {        	
			char c = (char) count;
			unicodeString.append(String.valueOf(c));
		}       
		return unicodeString.toString();
	}

	public static String duplicateMessage (String messageBody, int numberOfRepeats) {
		StringBuffer sb  = new StringBuffer();
		for (int loop =0; loop < numberOfRepeats; loop++) {
			sb.append(messageBody);
		}
		return messageBody = sb.toString();
	}

	public String checkMessages() {	
		String returnMessage=checkMessagesInt(MessagingTest.numberOfMessages * MessagingTest.numberOfPublishers);
//		synchronized (payloadsLock) {
//			if (originalPayloads.size()!= MessagingTest.NUMBEROFMESSAGES * MessagingTest.numberOfPublishers) {
//				// nothing got published, so we have an error
//				int missingMessages = MessagingTest.NUMBEROFMESSAGES - originalPayloads.size();
//				returnMessage = "not all messages published - test failed";
//				MessagingTest.reason = MessagingTest.reason + "no of mssing messages = " + missingMessages  +"\n";
//				MessagingTest.success =false;
//			} else {
//				List<String> payloadList = getPayloads();
//				if (payloadList.isEmpty()) {
//					returnMessage = "all messages received";
//				} else {
//					for (String message : payloadList) {
//						returnMessage = returnMessage + "missing message is " + message + "\n";
//					}
//				}
//			}
//		}
		return returnMessage;
	}
	
	public String checkMessagesInt(int noOfMessages) {
		String returnMessage="";
		synchronized (payloadsLock) {
			if (originalPayloads.size()!= noOfMessages) {
				// nothing got published, so we have an error
				int missingMessages = noOfMessages - originalPayloads.size();
				returnMessage = "not all messages published - test failed\n";
				returnMessage += "no of missing messages = " + missingMessages  +"\n";
				
				MessagingTest.reason = MessagingTest.reason + "no of missing messages = " + missingMessages  +"\n";
				MessagingTest.success =false;
			} else {
				List<String> payloadList = getPayloads();
				if (payloadList.isEmpty()) {
					returnMessage = "all messages received";
				} else {
					for (String message : payloadList) {
						returnMessage = returnMessage + "missing message is " + message + "\n";
					}
				}
			}
		}
		return returnMessage;
	}

	/**
	 * checks the message body matches the sent message body
	 * checks the identifier was in the original list
	 * checks the message has only been received once
	 * returns the unique message identifier (if present) or the original string if not
	 */
	
	public String checkPayload(String mqPayloadString) {
		String identifier="";
		//logger.printString("mqPayloadString " +mqPayloadString);
			String [] tokenisedPayloadString = mqPayloadString.split("TOKEN");
			if (!(tokenisedPayloadString.length ==2)) {
				logger.printString("tokenisedPayloadString has length " + tokenisedPayloadString.length);
				MessagingTest.success = false;
				MessagingTest.reason = MessagingTest.reason + "tokenisedPayloadString does not have 2 elements\n";
				identifier = mqPayloadString;
			} else {
				// check body OK
				if  (tokenisedPayloadString[0].equalsIgnoreCase(MessagingTest.messageBody)) {
					identifier = tokenisedPayloadString[1];
					synchronized (MessagingTest.messageUtilities.payloadsLock) {
					if (!MessagingTest.messageUtilities.getPayloads().remove(identifier)) {
						MessagingTest.success = false;
							if (MessagingTest.messageUtilities.getOriginalPayloads().contains(identifier)){
								//  if (!MessagingTest.reason.contains("duplicate message received")) {
								MessagingTest.reason = MessagingTest.reason + "duplicate message received: " + identifier+"\n";// + " MQ per: " + MessagingTest.messagePersistence +"\n";
								//  }
							} else {
								MessagingTest.reason = MessagingTest.reason + "unexpected message received: " + identifier +"\n";//+ " MQ per: " + MessagingTest.messagePersistence +"\n";
								MessagingTest.messageUtilities.getExtraPayloads().add(identifier);
							}
						}
					}
				} else {
					logger.printString("corrupted");
					MessagingTest.success = false;
					MessagingTest.reason = MessagingTest.reason + "body of message corrupted: " + tokenisedPayloadString[0]
					+ " should be " +MessagingTest.messageBody+"\n";
				}
			}
			return identifier;
	
//	public void checkMessageArrival() {
//		synchronized (payloadsLock) {
//			
//		}
//	}
}
}
