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

package com.ibm.ism.ws;

import java.io.UnsupportedEncodingException;

/**
 * This represents a WebSockets message
 */
public class IsmWSMessage  {
	 protected byte [] paybytes;
	 protected String  paystring;
	 protected int     mtype = 1;
	 public    String  reason;
	 
	 /** The type of the message is text */
	 public static final int TEXT   = 1;
	 /** The type of the message is binary */
	 public static final int BINARY = 2;
	 
	 protected IsmWSMessage() {
	 }
	 
	 /**
	  * Create a message with a byte array payload.
	  * @param payload  A byte array containing the payload 
	  * @param start    The starting byte within the byte array
	  * @param len      The length of the message in bytes
	  * @param mtype    The type of the message (TEXT/BINARY)
	  */
	 public IsmWSMessage(byte[] payload, int start, int len, int mtype) {
		 if (payload != null) {
			 paybytes = new byte[len];
			 System.arraycopy(payload, start, paybytes, 0, len);
		 }
		 this.mtype = mtype;
	 }
	 
	 /**
	  * Create a message with a String payload.
	  * @param payload  A string containing the payload.
	  */
	 public IsmWSMessage(String payload) {
	     paystring = payload;	 	 
	 }
	 
	 
	 /**
	  * Get the message payload as a byte array.
	  * @return The byte array form of the payload
	  */
	 public byte [] getPayloadBytes() {
		 if (paybytes == null)
			try { paybytes = paystring.getBytes("UTF-8"); } catch (UnsupportedEncodingException e) { }
	     return paybytes;		
	 }
	 
	 
	 /**
	  * Get the message payload as a string.
	  * @return The String form of the payload
	  */
	 public String  getPayload() {
		 if (paystring == null && paybytes != null)
		     try { paystring = new String(paybytes, "UTF-8"); } catch (UnsupportedEncodingException e) {}
	     return paystring;
	 }
	 
	 
	 /**
	  * Get the message type.
	  * <br>1 = Text message
	  * <br>2 = Binary message
	  * @return The message type
	  */
     public int getType() {
		 return mtype;
	 }
     
     
     /**
      * Set the message type. 
      * <br>1 = Text message
	  * <br>2 = Binary message
      * @param mtype The message type
      * @return This message
      */
	 public IsmWSMessage setType(int mtype) {
		 this.mtype = mtype;
	     return this;	 
	 }
}
