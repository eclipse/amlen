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

/**
 */
package com.ibm.ima.plugin;

import java.util.Map;

/**
 * The ImaMessage interface defines a message within the plug-in and represents the 
 * message as known internally in the MessageSight server.
 * 
 * A message has some header fields, a set of properties, and a body.
 */
public interface ImaMessage {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /**
     * Add to the properties of a message.
     * @param props  The properties to add
     * @return The message
     */
    public ImaMessage addProperties(Map<String,Object> props);
    
    /**
     * Clear all user message properties.
     * @return The message
     */
    public ImaMessage clearProperties();
    
    /**
     * Get the body of the message as byte array.
     * @return A byte array representing the message body or null to indicate there is no body
     */
    public byte []    getBodyBytes();
    
    /**
     * Get the body of the message as a Map.
     * @return A Map containing the body of the message, or null to indicate there is no body.
     * @throws ImaPluginException if the body cannot be converted to a map.
     */
    public Map<String,Object> getBodyMap();
    
    /**
     * Get the body of the message as a String.
     * The body must contain only valid UTF-8 to be converted to a String.
     * @return A String containing the body of the message, or null to indicate there is no body.
     * @throws ImaPlubinException if the body cannot be converted to a String.
     */
    public String      getBodyText();
    
    /**
     * Get a message property as a boolean.
     * @param name  The name of the property
     * @param default_value  The default if the property is missing or not a boolean
     * @return The value of the property as a boolean, or the defaulf_value if the property
     *         is missing or cannot be converted to a boolean.
     */
    public boolean     getBooleanProperty(String name, boolean default_value);
    
    /**
     * Get the name of the destination the message was published to.
     * <p>
     * This is guaranteed to exist and be at least one character long.  This is almost always
     * the destination to which the message was originally published, but this depends on
     * information provided by the originator.
     * 
     * @return The name of the destination.
     */
    public String      getDestination();
    
    /**
     * Get the type of the destination the message was published to.
     * @return The type of the destination which is either a topic or a queue.
     */
    public ImaDestinationType getDestinationType();
    
    
    /**
     * Get the type of the message body.
     * <p>
     * This is the indication from the originator of the message of the physical format 
     * of the message body.  It is possible that the message can be converted to another
     * format, and it is possible that the message body is not validly of the specified format.
     * @return The type of the message body
     */
    public ImaMessageType  getMessageType();
    
    /**
     * Get the persistence of the message.
     * <p>
     * The persistence value of true means that the message continues to be available
     * even if the server fails.  This is commonly implemented by writing the
     * message to non-volatile storage.
     * @return true if the message is persistent, and false if it is not
     */
    public boolean     getPersistence();
    
    /**
     * Get the reliability of the message.
     * <p>
     * The reliability of a message deals with the message guarantee in case of client or
     * network failure.  
     * @return The reliability
     */
    public ImaReliability  getReliability();
    
    /**
     * Get the retain value of the message.
     * If a message is published as retained, it is kept as part of the topic until another message 
     * is published as retained for the topic.  At most one retained message is kept for each topic.
     * Messages are not retained for queues and the retain bit is ignored when the destination is
     * a queue.
     * <p>
     * When a message is received, the setting of the retain flag is based on whether the message 
     * was delivered as part of the creation of a subscription.  If the subscription is already active
     * when a retained message is sent, it will not be seen as retained to the existing subscriptions.
     * @return true if the message is from a retained source, or false if it is not from a retained source.
     */
    public boolean     getRetain();
    
    /**
     * Get the retained value as published.
     * If a message is published as retained, it is kept as part of the topic until another message 
     * is published as retained for the topic.  At most one retained message is kept for each topic.
     * Messages are not retained for queues and the retain bit is ignored when the destination is
     * a queue.
     * <p>
     * When a message is published, the retain flag indicates it should be retained and sent for any
     * new subscriptions.  This retain as published flag indicates whether retain was requested when
     * the message was published.  This can be used to set the retain flag when a message is
     * forwarded. 
     * 
     * @return true if the message is published as retained, or false if it is not published as a retained message.
     */
    public boolean     getRetainAsPublished();
    
    /**
     * Get a read-only map of the user message properties.
     * <p>
     * Only named properties are returned.  These are commonly the user properties.
     * Many of the system properties are returned as other fields.
     * The resulting map cannot be modified.
     * @return A map containing the message properties.
     */
    public Map<String,Object> getProperties();

    /**
     * Get a message property as an integer.
     * <p>
     * If the message property is not an integer, attempt to convert it to an integer.  If the
     * property cannot be converted to an integer, return the default value.
     * @param name  The name of the property
     * @param default_value  The default value if the property is missing or cannot be converted to an integer
     * @return The value of the property as an integer, or the default_value if the property is missing
     *         or cannot be converted to an integer.
     */
    public int         getIntPropery(String name, int default_value);
    
    /**
     * Get a message property as an Object.
     * <p>
     * As it is valid for a property to have a null value, the method propertyExists() can be used
     * to differentiate between an existing property with a null value and a missing property.
     * <p>
     * All properties can be converted to a String, but the resulting String might not be useful in
     * all cases.  
     * @param name  The name of the property
     * @return The value of the property (which might be null), or null if the property does not exist.
     */
    public Object      getProperty(String name);

    /**
     * Get a message property as a String.
     * <p>
     * As it is valid for a property to have a null value, the method propertyExists() can be used
     * to differentiate between an existing property with a null value and a missing property.
     * @param name  The property name
     * @return The value of the property as a string (which might be null), or null if the property does not exist.
     */
    public String      getStringProperty(String name);

    
    /**
     * Get a user object associated with this message.
     * @return An object associated with this message or null if there is none
     */
    public Object getUserData();
    
    /**
     * Return whether a property exists.
     * 
     * @param name  The property name
     * @return true if the property exists, and false if it is missing
     */
    public boolean     propertyExists(String name);

    /**
     * Set the message type.
     * @param msgtype  The messge type
     * @return This message
     */
    public ImaMessage setMessageType(ImaMessageType msgtype);
    
    /**
     * Set the body as a byte array.
     * <p>
     * This completely replaces any existing body.
     * @param body  The byte array to set as the body
     * @return The message
     */
    public ImaMessage  setBodyBytes(byte [] body);
    
    /**
     * Set the body as a Map.
     *
     * The actual format of the resulting message depends on the message type.  
     * If the message type is a known form of mapped message it will be formatted by this type,
     * otherwise it will be encoded as a JSON string. 
     * <p>
     * This completely replaces any existing body. 
     * @param body  The byte array to set as the body
     * @return The message
     */
    public ImaMessage  setBodyMap(Map<String,Object> body);
    
    /**
     * Set the body as a String.
     * <p>
     * This completely replaces any existing body. 
     * @param body  The string to set as the body.
     * @return The message
     */
    public ImaMessage  setBodyText(String body);
    
    /**
     * Set the persistence of the message.
     * <p>
     * The persistence value of true means that the message continues to be available
     * even if the server fails.  This is commonly implemented by writing the
     * message to non-volatile storage.
     * 
     * @param persist true for persistent and false for non-persistent
     * @return The message
     */
    public ImaMessage  setPersistent(boolean persist);
    
    /**
     * Set the reliability of the message.
     * <p>
     * The reliability of a message deals with the message guarantee in case of client or
     * network failure.  To ensure message delivery in the case of a server failure
     * the message should also be persistent.
     * 
     * @param reliability  The reliability 
     * @return The message
     */
    public ImaMessage  setReliability(ImaReliability reliability);
    
    /**
     * Set a user object associated with this message.
     * @param userdata  An object to associate with this message
     */
    public void setUserData(Object userdata);
    
    
    /**
     * Set the retain value of the message.
     * 
     * If a message is published as retained, it is kept as part of the topic until another message 
     * is published as retained for the topic.  At most one retained message is kept for each topic.
     * Messages are not retained for queues and the retain bit is ignored when the destination is
     * a queue.  When a subscription to a topic is created, any retained messages are sent to the 
     * subscription.  If the subscription is a wildcard subscription there can be multiple retained
     * messages.
     *   
     * @param retain true if the message should be retained, false if it should not be retained.
     * @return The message
     */
    public ImaMessage  setRetain(boolean retain);
    
    /**
     * Acknowledge a received message.
     * @param rc   A return code, 0=good.
     */
    public void        acknowledge(int rc);
    
    /**
     * Acknowledge a received message.
     * 
     * @param rc A return code, 0=good.
     */
    public void acknowledge(int rc, ImaTransaction transaction);

}
