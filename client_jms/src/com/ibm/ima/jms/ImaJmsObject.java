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
package com.ibm.ima.jms;

import javax.jms.Message;

import com.ibm.ima.jms.impl.ImaConstants;
import com.ibm.ima.jms.impl.ImaMessage;
import com.ibm.ima.jms.impl.ImaPropertiesImpl;
import com.ibm.ima.jms.impl.ImaReadonlyProperties;

/**
 * Utilities for IBM MessageSight JMS client objects.
 * 
 * These static methods are designed to be used for problem determination when working with
 * IBM MessageSight JMS client applications.  JMS applications should normally avoid using
 * provider specific methods, but it might be necessary to use these when developing
 * IBM MessageSight JMS client applications or when doing provider specific error handling.
 */
public class ImaJmsObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    private static final String ImaClientVersion = ImaConstants.Client_build_version + " Build: " + ImaConstants.Client_build_id;
	
	/**
	 * Return a string form of an IBM MessageSight JMS client object with details.
	 * 
	 * This method is designed to be used for problem determination on IBM MessageSight JMS
	 * client object and allows the level of detail to be specified.  This can be called on any object and 
	 * returns the normal toString() for other objects.
	 * <p>
	 * These detailed strings are designed to be viewed by humans to help in problem determination 
	 * and might not be fully parsable.  They are also not guaranteed to remain the same over time.
	 * All information in the detailed string is available using the public interfaces to the
	 * IBM MessageSight JMS client but show implementation details which are not part of the JMS
	 * interfaces and which are likely to change from release to release.     
	 * <p>
	 * The detailed strings are in the form:
	 * <pre>
	 *     class@hashcode info=value details=value links=value properties=value
	 * </pre>    
	 * The class name is the implementation class name for the object which is commonly the JMS
	 * interface name with the string "Ima" prepended.  These classes commonly implement both the
	 * Topic and Queue messaging domains.  The hashcode is shown to allow the unique object to
	 * be determined.  
	 * <p>
	 * Each of the sections info, details, and links consists of a set of key=value pairs.  If an item
	 * is not set or has a default value the key=value is not shown and the entire section can be 
	 * missing. Not all sections are available for all objects. 
	 * <p>
	 * The properties sections consists of the keyword properties and the value which is the toString()
	 * of a Java properties object. 
	 * <p>
	 * Newlines may be added to the string to increase readability.   
	 * <p>
	 * The details string is a set of characters each of which define the presence of one of the sections.
	 * All sections are optional.  Additional codes are reserved for future use but are not checked.
	 * The following codes can be set:
	 * <dl>
	 * <dt>c<dd>The class name of the implementation class 
	 * <dt>h<dd>The hashCode in hex starting with an at sign (@)
	 * <dt>i<dd>The info section contains common information
	 * <dt>d<dd>The details section contains more detailed information
	 * <dt>l<dd>The links section contains links to other objects
	 * <dt>p<dd>The properties sections contains the properties of the object
	 * <dt>*<dd>Show all sections
	 * </dl>
	 * @param obj      The object to show.  
	 * @param details  The details string
	 * @return A string representing detailed information about the object.
	 */
	public static String toString(Object obj, String details) {
		int opt = convertDetails(details);
		if (obj instanceof ImaMessage) {
			return ((ImaMessage)obj).toString(opt);
		}
		if (obj instanceof ImaPropertiesImpl) {
			return ((ImaPropertiesImpl)obj).toString(opt);
		}
		if (obj instanceof ImaReadonlyProperties) {
			return ((ImaReadonlyProperties)obj).toString(opt);
		}
		if (obj == null)
			return "null";
		return obj.toString();
	}
	
	/**
	 * Return the size in bytes of the message body.
	 * 
	 * Not all JMS message types can return the size of the body.  This allows the body size to
	 * be returned for all IBM MessageSight JMS client messages.  The size is the number of
	 * bytes used to send the body of the message and does not include the size of the 
	 * header or properties.  This is designed to be used for problem determination.
	 * <p>
	 * If the body is null a size of -1 is returned. 
	 * <p>
	 * @param msg    The message which must be an IBM MessageSight JMS client message
	 * @return The size in bytes of the payload or -1 if the payload is null.
	 * @throws ClassCastException If the message is not an IBM MessageSight message
	 */
	public static int bodySize(Message msg) {
	    return ((ImaMessage)msg).bodySize();
	}
	
	/*
	 * Convert the details string to a details option integer
	 */
	static int convertDetails(String details) {
		int  ret = 0;
		if (details == null || details.length()==0)
			return ImaConstants.TS_Common;
		for (int i=0; i<details.length(); i++) {
			switch (details.charAt(i)) {
			case 'b':
			case 'B':
				ret |= ImaConstants.TS_Body;
				break;
			case 'c':
			case 'C':
				ret |= ImaConstants.TS_Class;
				break;
			case 'h':
			case 'H':
				ret |= ImaConstants.TS_HashCode;
				break;
			case 'i':
			case 'I':
				ret |= ImaConstants.TS_Info;
				break;
			case 'd':
			case 'D':
				ret |= ImaConstants.TS_Details;
				break;
			case 'l':
			case 'L':
				ret |= ImaConstants.TS_Links;
				break;	
			case 'p':
			case 'P':
				ret |= ImaConstants.TS_Properties;
				break;
			case '*':
				ret |= ImaConstants.TS_All;
				break;
			}
		}
		return ret;
	}

    /**
     * Gets the properties for an IBM MessageSight JMS client object.
     * <p>
     * This method works for objects which implement the ImaProperties interface.
     * The objects with properties which are not administered objects (including the IBM MessageSight JMS client
     * implementations of Connection, Session, MessageConsumer, and MessageProducer) return read only properties.
     * Only user properties can be modified in these objects.
     * <p>
     * This is just a convenience method as it simply casts the object to ImaProperties.
     * The invoker should make sure that the object is an instance of ImaProperties before
     * using this method.
     * 
     * @param obj  The object from which to return the properties.
     * @return The properties for the object
     * @throws ClassCastException if the object does not implement ImaProperties (RuntimeException)
     */
    public static ImaProperties getProperties(Object obj) {
        return (ImaProperties)obj;
    }
    
    /**
     * Gets the IBM MessageSight JMS client version.
     * <p>
     * This method returns the IBM MessageSight client version.
     * 
     * @return The client version
     */
    public static String getClientVerstion() {
        return ImaClientVersion;
    }
    
}
