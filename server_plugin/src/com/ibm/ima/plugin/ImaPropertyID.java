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
package com.ibm.ima.plugin;

/**
 * A property ID defines identifiers for header fields that are larger than one byte.
 * <p>
 * A message consists of a small number of header fields, a set of properties, and a body.
 * Only items which are small (up to a byte) are kept in the header as this is common to
 * all messages.  Larger header fields are kept as system properties.
 * <p>
 * User properties have names, but system properties are identified by number.  When a message
 * is delivered to a client these system properties are commonly presented as protocol unique
 * header or identifier fields and not as user properties.  These system properties can also be
 * used to hold information known only to a single protocol.  
 * <p>
 * System properties can be interchanged between protocols when the meaning is similar, but 
 * if the concepts do not match should be ignored when they come from another protocol.  When
 * receiving a message the plug-in should always check that the type and value of the property are acceptable
 * to the receiving protocol.  The receiving protocol must be tolerant that any of these 
 * properties are not set.
 * <p>
 * The most common system property is ID_Topic.  This is used to retain the topic
 * name to which the message was published.  This is important in cases where wildcard topic
 * subscriptions are allowed.  This is the only property used by the native MQTT protocol.
 * The JMS native protocol uses the system topics Timestamp to DeliveryTime.
 * <p>
 * An ID must be in the range 0 to 16777216.  The values 0 to 255 are reserved for common
 * use.  To define private IDs, use values in the range 256 to 16777216. 
 */
public class ImaPropertyID {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    /** The creation timestamp of the message - long as milliseconds since 1970-01-01T00Z */
    public static final int  Timestamp    =  2;   
    /** The expiration timestamp of the message - long as milliseconds since 1970-01-01T00Z */
    public static final int  Expire       =  3;   
    /** The message ID (commonly system generated) - String */
    public static final int  MsgID        =  4; 
    /** The correlation ID - String */
    public static final int  CorrID       =  5; 
    /** The type string - String */
    public static final int  JMSType      =  6; 
    /** The reply to queue - String destination name */
    public static final int  ReplyToQ     =  7;  
    /** The reply to topic - String destination name */
    public static final int  ReplyToT     =  8;  
    /** The topic name (this is needed if the protocol allows wildcard topics) - String */
    public static final int  Topic        =  9;    
    /** To time before which the message should not be delivered - long as milliseconds since 1970-01-01T00Z */
    public static final int  DeliveryTime = 10;  
    /** The queue name (this is needed if the protocol allows wildcard queues) - String */
    public static final int  Queue        = 11;  
    /** The user ID of the originator of the message - String */
    public static final int  UserID       = 12;   
    /** The client ID of the originator of the message - String */
    public static final int  ClientID     = 13;   
    /** The domain of the client ID.  This is used to segment the topic space. - String */
    public static final int  Domain       = 14;   
    /** A security token authenticating this message */
    public static final int  Token        = 15;  
    /** The device identifier of the originator - String */
    public static final int  DeviceID     = 16;   
    /** The application identifier of the originator - String */
    public static final int  AppID        = 17;    
    /** The protocol which originated the message - String */
    public static final int  Protocol     = 18;   
    /** An identifier of an object associated with the message */
    public static final int  ObjectID     = 19;  
    /** An identifier of the group of a message - String */
    public static final int  GroupID      = 20;   
    /** The sequence within a group - int */
    public static final int  GroupSeq     = 21;    

}
