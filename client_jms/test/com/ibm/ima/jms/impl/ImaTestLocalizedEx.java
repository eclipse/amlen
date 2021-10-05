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

import java.util.Locale;

import javax.jms.Destination;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaProperties;

import junit.framework.TestCase;

public class ImaTestLocalizedEx extends TestCase {
	
	String server = "myserver";
	int port = 0;
	int protocol = 1;
	
	public ImaTestLocalizedEx() {
		super("IMA JMS Client localized exception test");
	}
	
	public void testExceptions() {
		ImaJmsException ex = new ImaIllegalStateException("CWLNC0001", "The session is not transacted");
		testLocalizedMessages(ex);

		ex = new ImaInvalidDestinationException("CWLNC0002", "The subscription name is not valid");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0003", (Exception)null, "Unable to create temporary queue");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidDestinationException("CWLNC0004", 
                "A temporary destination can only be used within the connection which created it.");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0005", "The temporary destination is in use.");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidDestinationException("CWLNC0006", 
                "Durable subscription cannot be created for temporary topic");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidDestinationException("CWLNC0007", "The destination does not have a name");
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC0008", "The connection is closed");
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC0009", "The session is closed");
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC0010", "The message consumer is closed");
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC0011", "The message producer is closed");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidDestinationException("CWLNC0012", "The destination must be specified");
		testLocalizedMessages(ex);
		
		Destination dest = new ImaTopic("myTopic");
		ex = new ImaInvalidDestinationException("CWLNC0013", "The destination must be an Eclipse Amlen destination: {0}.", dest);
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC0014", 
	               "A durable subscription cannot be created for a connection with a generated client ID");
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC0015", "The queue browser is closed");
		testLocalizedMessages(ex);
		
		ex = new ImaNamingException("CWLNC0016", "A temporary destination cannot be stored in JNDI.");
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC0017", "The method cannot be performed on an object created as a TopicSession: {0}", "createBrowser");
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC0018", "The method cannot be performed on an object created as a QueueSession: {0}", "createDurableSubscription");
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC0019", "The session is transacted");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0020", (Exception)null, "Send message to server has failed.");
		testLocalizedMessages(ex);
		
		String name = "durableName";
		String cid = "myClientId";
		ex = new ImaIllegalStateException("CWLNC0021", 
                "An active non-shared durable consumer already exists: Name={0} ClientID={1}",
                name, cid);
		testLocalizedMessages(ex);

	    ex = new ImaJmsExceptionImpl("CWLNC0022", (Exception)null, 
        "Connection attempts for all specified servers have failed: Server={0} Port={1} Protocol={2}" + 
        "\nTo see failure information for all failed connection attempts, enable client trace at level 2 or above.",
        server, ""+port, protocol==2 ? "tcps" : "tcp");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0023", (Exception)null, "Unexpected client failure");
		testLocalizedMessages(ex);
		
		int msgtype = 1;
		ex = new ImaJmsExceptionImpl("CWLNC0024", "Unknown message type received: {0}", msgtype);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0025", "Message received is not valid");
		testLocalizedMessages(ex);
		
		int rc = 1;
		ex = new ImaJmsExceptionImpl("CWLNC0026", "Failed to create temporary queue (rc = {0}).", rc);
		testLocalizedMessages(ex);
		
		ex = new ImaSecurityException("CWLNC0027", "Not authorized to resolve socket address for {0}:{1}", server, port);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0028", "IP address ({0}) or port ({1}) is not valid.", server, port);
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalArgumentException("CWLNC0029", "Could not resolve IP address: {0}", server);
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC0030", "The ClientID is already set");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidClientIDException("CWLNC0031", "The clientID is not specified.");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidClientIDException("CWLNC0032", "The clientID is not a valid Unicode string.");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0033", "The String cannot be written because it is too long.");
		testLocalizedMessages(ex);
		
		String className = "myClass";
		ex = new ImaMessageFormatException("CWLNC0034", "The object class is not supported: {0}", className);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0035", "The value for delivery mode is not correct");
		testLocalizedMessages(ex);
		
		int priority = -1;
		ex = new ImaJmsExceptionImpl("CWLNC0036", "The JMSPriority value is not valid.  It must be in the range 0 to 9 and is {0}", priority);
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0037", "The property name is not valid: {0}", name);
		testLocalizedMessages(ex);
		
		ex = new ImaMessageNotWriteableException("CWLNC0038", "The message is not writable");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0039", "The UTF-16 string encoding is not valid");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0040", "The UTF-16 encoding is not valid in name: {0}", name);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0041", (Exception)null, "Serialization error");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0042", "The message cannot be null.");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0043", (Exception)null, "Error in writeBytes()");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0044", (Exception)null, "Request failed due to an interruption.");
		testLocalizedMessages(ex);
		
		ex = new ImaIOException("CWLNC0045", "Socket connection read error: rc={0}", rc);
		testLocalizedMessages(ex);
		
		ex = new ImaIOException("CWLNC0046", "Received message with invalid length: {0}", rc);
		testLocalizedMessages(ex);
		
		ex = new ImaSecurityException("CWLNC0047", (Exception)null, "Not authorized to establish a connection to: {0}", server);
		testLocalizedMessages(ex);
		
		ex = new ImaUnsupportedOperationException("CWLNC0048", "The destination must be set in the producer or in the call to send or publish.");
		testLocalizedMessages(ex);
		
		ex = new ImaUnsupportedOperationException("CWLNC0049", "The destination must be set only once.");
		testLocalizedMessages(ex);
		
		ex = new ImaNoSuchElementException("CWLNC0050", "The browser is closed.");
		testLocalizedMessages(ex);
		
		ex = new ImaNoSuchElementException("CWLNC0051", "There are no more elements to be browsed.");
		testLocalizedMessages(ex);
		
		ex = new ImaUnsupportedOperationException("CWLNC0052", "The {0} method is not supported.  Use {1}.", "wrongmethod", "rightmethod" );
		testLocalizedMessages(ex);
		
		ex = new ImaNullPointerException("CWLNC0053", "The property ({0}) does not exist.","missingprop" );
		testLocalizedMessages(ex);
		
		ex = new ImaNumberFormatException("CWLNC0053", "The property ({0}) does not exist.","missingprop" );
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalArgumentException("CWLNC0054", "The property name must be specified.");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageEOFException("CWLNC0060", "The end of the message has been reached.");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0061", "Boolean conversion error");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0062", "Byte conversion error");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0063", "Double conversion error");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0064", "Integer conversion error");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0065", "Float conversion error");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0066", "Long conversion error");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0067", "Short conversion error");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0068", "String conversion error");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0069", "Character conversion error");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0070", "Byte array conversion error");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageNotReadableException("CWLNC0071", "The message is not readable.");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0072", "The UTF-8 text encoding is not valid.");
		testLocalizedMessages(ex);
		
		int len = 1;
		int remaining = 2;
		ex = new ImaMessageEOFException("CWLNC0073", "The end of the message has been reached. Tried to read {0} bytes but {1} bytes were left.", 
    	        len, remaining);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0074", "The message data is not valid");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0075", (Exception)null, "Deserialization error");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0076", "The byte array data is not valid");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0078", "The item does not exist: {0}", ((name!=null)?name:""));
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl(
				"CWLNC0079", (Exception)null, "Unexpected failure in onMessage");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0080", "Other fields cannot be read until the readBytes completes");
		testLocalizedMessages(ex);
		
		ex = new ImaNullPointerException("CWLNC0081", "A null value cannot be converted to a character.");
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalArgumentException("CWLNC0082", "The item name cannot be null or a zero length string.");
		testLocalizedMessages(ex);
		
		String method = "myMethod";
		ex = new ImaJmsExceptionImpl("CWLNC0091", "The method is not implemented: {0}", method);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0098", "The connection startup failed: {0}", rc );
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC0099", "The stop method was called from onMessage callback.");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0101", "Security socket factory class {0} is not valid", className);
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalArgumentException("CWLNC0102", "None of the requested security protocols is supported. Supported protocols are: {0}", className);
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalArgumentException("CWLNC0103", "None of the requested security cipher suites is supported. Supported cipher suites are: {0}", className);
		testLocalizedMessages(ex);
		
		ex = new ImaSecurityException("CWLNC0104", "No security socket factory found to load.");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0201", "The message content data is not valid.");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0202", "The MapMessage data is not valid");
		testLocalizedMessages(ex);
		
		ex =  new ImaJmsExceptionImpl("CWLNC0203",
                "Failed to create producer: rc={0})", rc);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0204","Failed to create session (rc = {0}).", rc);
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidClientIDException("CWLNC0205", "The client ID is in use: {0}", cid);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0206",
                "Failed to unsubscribe: rc={0} Name={1} ClientID=.", rc, name,
                cid);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsSecurityException("CWLNC0207", "Authorization failed");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0208", "The connection failed: rc={0}", rc);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0209",
                "Failed to create consumer: rc={0} Name={1} ClientID={2}", rc, name, cid);
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0210", "The message does not contain stream data");
		testLocalizedMessages(ex);
		
		ex = new ImaMessageFormatException("CWLNC0211", "The message does not contain a map message");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0213", "Message properties not valid");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0214", "The message properties length is not valid");
		testLocalizedMessages(ex);
		
		String domain = "topic";
		String topicName = "myTopic";
		int producerid = 1;
		String host = "myHost";
		ex = new ImaJmsExceptionImpl("CWLNC0215", "Message for {0} {1} from producer {2} is too big for {3}", 
                domain,
                topicName, 
                producerid,
                host+":"+port);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0216", "Failed to send message for {0} {1} from producer {2}. Unknown failure - rc={3}.", 
                domain,
                topicName, 
                producerid,
                rc);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0217",
                "Destination \"{0}\" is not valid", ((ImaProperties) dest).get("Name").toString());
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0218", "Failed to send message for {0} {1} from producer {2}. Destination is full.",
                domain,
                topicName, 
                producerid);
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidDestinationException("CWLNC0219", "Failed to send message for {0} {1} from producer {2}.  Destination is not valid.",
                domain,
                topicName, 
                producerid);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0220", "Failed to create browser (rc = {0}).", rc);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl(
                "CWLNC0222", "Transaction was rolled back. Reason: {0}.");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0223", "Failed to send message for {0} {1} from producer {2}.  Server capacity exceeded.",
                domain,
                topicName, 
                producerid,
                rc);
		testLocalizedMessages(ex);
		
		int sec = 60;
		String connect = "myConnection";
		ex = new ImaJmsExceptionImpl("CWLNC0224", 
        		"Closing the connection as the server is not responsive: timeout={0} sec connect={1}", sec, connect);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0225", "Failed to send message for Topic \"{0}\" from producer {1}.  The topic is a system topic.",
                topicName, producerid);
		testLocalizedMessages(ex);
		
		int consumerCount = 1;
		ex = new ImaInvalidDestinationException("CWLNC0226", 
                "The subscription has active consumers: name={0} client={1} consumers={2}.", name,
                cid, consumerCount);
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidDestinationException("CWLNC0227", 
                "The subscription was not found: Name={0} ClientID={1}.", name,
                cid);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0228", "The shared subscription cannot be modified: Name={0} ClientID={1}.",
                name, cid);
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalStateException("CWLNC0229", 
                "The subscription cannot be modified because the subscription is in use: Name={0} ClientID={1}.",
                name, cid);
		testLocalizedMessages(ex);
		
//		// Temporarly omit this test -- CWLNC0230 is reserved for future use
//		ex = new ImaMessageFormatException("CWLNC0230", "The message body cannot be assigned to the specified type.");
//		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0231", "Failed to send message for {0} {1} from producer {2}.  Server memory exceeded (rc = {3}).",
                domain, topicName, producerid, rc);
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0241", "The selection rule is too complex");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0242", "The IS expression is not valid");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0243", "The IN must be followed by a group");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0244", "The IN group is not valid");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0245", "The separator in an IN group is missing or not valid");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0246", "The LIKE must be followed by a string");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException( "CWLNC0247", "The ESCAPE within a like must be a single character string");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0248", "A string in a selection rule is too complex");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0249", "The identifier or constant is not valid: {0}", 
                cid);
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0250", "The BETWEEN operator is not valid");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0251", "Too many left parentheses");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0252", "A field or constant is expected at the end of a selection rule");
		testLocalizedMessages(ex);
		
		String op = "op";
		ex = new ImaInvalidSelectorException("CWLNC0253", "The {0} operator does not allow a boolean argument",
                op);
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0254", "The {0} operator does not allow a numeric argument",
                op);
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0255", "The {0} operator requires an identifier as an argument",
                op);
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0256", "A boolean result is required for the selector");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0258", "Too many right parentheses");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0259", "The {0} operator does not allow a string or boolean argument", 
                op);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0290", "The like string contains a code point which is not a valid Unicode character.");
		testLocalizedMessages(ex);
		
		ex = new ImaInvalidSelectorException("CWLNC0291", "The compare of boolean and numeric constants always gives an unknown.");
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0301",
		        "The property value is not serializable: {0}", name);
		testLocalizedMessages(ex);
		
		String value = "val";
		ex = new ImaJmsPropertyException("CWLNC0302",
		        "The property \"{0}\" is not a valid name: {1}",
		        name, value);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsPropertyException(
		        "CWLNC0303",
		        "The property \"{0}\" is not a known property.", name);
		testLocalizedMessages(ex);
		
		String enumeration = "enum";
		ex = new ImaJmsPropertyException(
		        "CWLNC0304",
		        "The property \"{0}\" is of an enumeration \"{1}\" that is not known.",
		        name, enumeration);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsPropertyException("CWLNC0305",
		        "The property \"{0}\" is not set.", name);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsPropertyException("CWLNC0306",
		        "The property \"{0}\" must be one of: {1}. The value is {2}.",
		        name, enumeration, value);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsPropertyException(
                "CWLNC0307",
                "The property \"{0}\" is not a valid boolean string.  The value is: {1}",
                name, value);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsPropertyException(
		        "CWLNC0308",
		        "The property \"{0}\" is not a valid boolean value.  The value is: {1}",
		        name, value);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsPropertyException(
		        "CWLNC0309",
		        "The property \"{0}\" is not an integer.  The value is: {1}",
		        name, value);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsPropertyException(
		        "CWLNC0310",
		        "The property {0} has a negative value which is not valid.  The value is: {1}.",
		        name, value);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsPropertyException(
		        "CWLNC0311",
		        "The property \"{0}\" has too small a value.  The value is {1}. The minimum value is: {2}.",
		        name, value, value);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsPropertyException(
		        "CWLNC0312",
		        "The property \"{0}\" has too large a value. The value is {1}. The maximum value is: {2}.",
		        name, value, value);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsExceptionImpl("CWLNC0313", (Exception)null,
		        "The property \"{0}\" is not valid: {1}",
		        name, value);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsPropertyException("CWLNC0314",
		        "The property is a system property: {0}", name);
		testLocalizedMessages(ex);
		
		ex = new ImaJmsPropertyException("CWLNC0315", (Exception)null,
		        "The property \"{0}\" is not a valid selector: {1}"
		                + name, value);
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalArgumentException("CWLNC0316",
		        "The validator must be specified.");
		testLocalizedMessages(ex);
		
		ex = new ImaIllegalArgumentException("CWLNC0317",
		        "he validator is not valid: {0}", name);
		testLocalizedMessages(ex);
		
		ex = new ImaRuntimeIllegalStateException("CWLNC0318",
		        "The properties are read only.");
		testLocalizedMessages(ex);
		
		ex = new ImaRuntimeException("CWLNC0350",
		        "The IMA trace level configuration is not an integer: {0}. Valid values are 0 to 9.", value);
		testLocalizedMessages(ex);
		
		ex = new ImaRuntimeException("CWLNC0351",
		        "The IMA trace level \"{0}\" is not valid. Valid values are 0 to 9.", value);
		testLocalizedMessages(ex);
		
		ex = new ImaRuntimeException("CWLNC0352",
		        "The trace destination is not valid.");
		testLocalizedMessages(ex);
		
		ex = new ImaRuntimeException("CWLNC0353", (Exception)null,
		        "Failure setting trace destination: {0}", value);
		testLocalizedMessages(ex);
		
		ex = new ImaTransactionInProgressException("CWLNC0480", (Exception)null,
		        "Method call not permitted for an XASession object.");
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC0481", rc,
		        "Recover failed due to invalid flags ({0}).", value);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC0482", rc,
		        "Call to {0} failed because Xid is null.", name);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC0483", rc,
		        "Call to {0} failed.  Expected Xid {1} but found {2}.", name, port, rc);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC0484", rc,
		        "Call to {0} failed.  Expected branch {1} but found {2}.", name, port, rc);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC0485", rc,
		        "Call to {0} failed.  Expected global transaction {1} but found {2}.", name, port, rc);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC0486", rc,
		        "Global transaction failed for {0}: IMA server was unreachable.", name);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC0487", rc,
		        "Global transaction failed for {0}: Xid ({1}) is unknown.", name, rc);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC0488", rc,
		        "Global transaction failed for {0}: Global transaction was marked as rollback only.", name);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC0489", rc,
		        "Global transaction failed for {0}: Requested Xid ({1}) already exists.", name, rc);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC0490", rc,
		        "Global transaction failed for {0}: Invalid parameter.", name);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC0491", rc,
		        "Global transaction failed for {0}: Global transaction was heuristically committed.", name);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC0492", rc,
		        "Global transaction failed for {0}: Global transaction was heuristically rolled back.", name);
		testLocalizedMessages(ex);
		
		ex = new ImaXAException("CWLNC0493", rc,
		        "Global transaction failed for {0}: Error {1} returned by the IMA server.", name, rc);
		testLocalizedMessages(ex);
	}

	public void testLocalizedMessages(ImaJmsException ex) {
	    
	    Locale frlo = new Locale("fr","FR");
	    Locale delo = new Locale("de", "DE");
	    Locale zhlo = new Locale("zh");
	    Locale zhTWlo = new Locale("zh","TW");
	    Locale jalo = new Locale("ja");
	    
	    assertTrue(ex.getMessage(frlo).endsWith("fr]"));
	    assertTrue(ex.getMessage(delo).endsWith("de]"));
	    assertTrue(ex.getMessage(zhlo).endsWith("zh]"));
	    assertTrue(ex.getMessage(zhTWlo).endsWith("zh_TW]"));
	    assertTrue(ex.getMessage(jalo).endsWith("ja]"));
	    
	    Locale.setDefault(frlo);
	    assertTrue((((Throwable)ex).getLocalizedMessage()).endsWith("fr]"));
	    
	    Locale.setDefault(delo);
	    assertTrue((((Throwable)ex).getLocalizedMessage()).endsWith("de]"));
	    
	    Locale.setDefault(zhlo);
	    assertTrue((((Throwable)ex).getLocalizedMessage()).endsWith("zh]"));
	    
	    Locale.setDefault(zhTWlo);
	    assertTrue((((Throwable)ex).getLocalizedMessage()).endsWith("zh_TW]"));
	    
	    Locale.setDefault(jalo);
	    assertTrue((((Throwable)ex).getLocalizedMessage()).endsWith("ja]"));
	    

	}
	
    /*
     * Main test 
     */
    public void main(String args[]) {
        try {
        	new ImaTestLocalizedEx().testExceptions();
        } catch (Exception ex) {
        	ex.printStackTrace();
        }
    }
    
}
