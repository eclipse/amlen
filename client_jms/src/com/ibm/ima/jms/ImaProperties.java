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

import java.util.Map;
import java.util.Set;

import javax.jms.JMSException;


/**
 * Defines a set of properties used primarily to configure IBM MessageSight JMS client administered objects.
 * <p>
 * Properties are defined for the IBM MessageSight JMS client administered objects which are created 
 * using ImaJmsFactory.  These administered objects implement the ImaProperties interface.
 * The Connection, Session, MessageConsumer, and  MessageProducer objects within the 
 * IBM MessageSight JMS client also implement the ImaProperties interface with read only properties
 * using the properties of the objects from which they were created.
 * <p>
 * In the IBM MessageSight JMS client there are three administered objects:
 * <dl>
 * <dt>ConnectionFactory</dt><dd>The ConnectionFactory implementation in the IBM MessageSight JMS client implements the
 * three connection factory interfaces: ConnectionFactory, TopicConnectionFactory, and QueueConnectionFactory.</dd>
 * <dt>Queue</dt><dd>The Queue implementation in the IBM MessageSight JMS client implements two destination 
 * interfaces: Destination and Queue.</dd>
 * <dt>Topic</dt><dd>The Topic implementation in the IBM MessageSight JMS client implements two destination 
 * interfaces: Destination and Topic.</dd>
 * </dl>
 * The Connection, Session,  MessgeConsumer, and MessageProducer objects within the IBM MessageSight JMS client contain 
 * a set of read only properties.  These properties are derived from the objects used to create them.  Some of the
 * read only properties present state information.  Where this is the case, the properties are modified and kept current 
 * internally by the JMS client. 
 * Properties with names containing the string "user" (other than "userid") can be modified in the read only 
 * properties.  The MessageProducer and MessageConsumer objects within the IBM MessageSight JMS client
 * contain a set of read only properties which are based on the Destination properties but are expanded
 * as required.  If the MessageProducer is created without a Destination, some of these properties will not be set.
 * <p>
 * To get the properties associated with any IBM MessageSight object, you can cast it to ImaProperties, 
 * or use ImaJmsFactory.getProperties().
 * In either case, you should either check that the object is an instance of ImaProperties, or catch the ClassCastException.
 * <p>
 * Each administered object has a set of properties and these have a simple validation routine.  The validation
 * is not done when the property is set, but when validate() is called.  The single property validation methods
 * throw a JMSException which is also an ImaJmsException.  The multiple property form of validate returns an
 * array of ImaJmsException objects.  The properties are valid when the single property validate() methods do not 
 * throw exceptions, or when the multiple property form of validate() returns null.
 * <p>
 * The names of known properties are processed independent of case and the names are mapped to the preferred case 
 * when the property is set. 
 * <p>
 * When a connection is created with a userid and password, the property "userid" is
 * set on the connection object.
 * 
 * <p>
 * @see com.ibm.ima.jms.ImaJmsFactory The set of properties which can be set for each
 * of the IBM MessageSight JMS client administered objects.
 */
public interface ImaProperties {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    /**
     * Include warning messages in the array of strings returned by validate.
     */
    public final static boolean WARNINGS = false;
    
    /**
     * Do not include warning messages in the array of strings returned by validate.  Use this setting to ignore warnings.
     */
    public final static boolean NOWARNINGS = true;
    
    /**
     * Clears all properties.
     * 
     * @throws IllegalStateException If the properties are read only 
     */
    public void clear() throws JMSException;
    
    /**
     * Checks if a property has been set. 
     * @param name  The name of the property
     * @return true if the property has been set.
     */
    public boolean exists(String name);
 
    /**
     * Returns the value of the specified property as an Object.
     * 
     * Any serializable object can be returned as a property value for an administered object
     * 
     * @param  name  The name of the property
     * @return The object value of the property, or null if it does not exist.
     */
    public Object get(String name);
        
    
    /**
     * Returns the value of the specified property as an int.
     * 
     * If the property is an enumeration, return the value of the enumeration.  
     * If the value is a Boolean, return 0 for false and 1 for true. 
     * <p>
     * If the property does not exist or cannot be converted to an integer, return the default_value.
     * 
     * @param  name  The name of the property
     * @param  default_value  The value to return if the property is missing or not an integer.
     * @return The integer value of the property, or default_value if the property does not exist 
     *  or cannot be converted to an integer.
     */
    public int getInt(String name, int default_value);
    
    /**
     * Returns the value of the specified property as a boolean.
     * 
     * If the property is an enumeration, if the enumerated value is 0 it is false;
     * all other values are true.  If the value is an integer, the value 0 is false;
     * all other values are true.    
     * <p>
     * If the property does not exist or cannot be converted to a boolean, return the default_value.
     * 
     * @param  name  The name of the property
     * @param  default_value  The value to return if the property is missing or not a boolean.
     * @return The boolean value of the property, or default_value if the property does not exist 
     *  or cannot be converted to a boolean.
     */
    public boolean getBoolean(String name, boolean default_value);
    
    /**
     * Returns the value of the specified property as a String.
     * 
     * All properties can be represented as a string, but for complex objects the string is only
     * a reference to the object.
     * 
     * @param  name  The name of the property
     * @return The string value of the property.  If the property does not exist, return null.
     */
    public String getString(String name);
    
    /**
     * Returns the set of properties.
     * 
     * This set is a copy of the properties at the time of the call.
     * @return A Set containing the property names available 
     */
    public Set<String> propertySet();
        
    /**
     * Sets a property.
     * <p>
     * Very little checking is done at the time of the set.  
     * It is strongly recommended that a validate method be run on the property
     * either before or after it is set.
     * Any serializable object can be set as a value.
     * 
     * @param name  The name of the property
     * @param value The new value for the property
     * @return The previous value of the property, which can be null
     * @throws JMSException For an internal error of if the object is not serializable
     * @throws IllegalStateException If the properties are read-only 
     */
    public Object put (String name, Object value) throws JMSException;
    
    /**
     * Sets multiple properties.
     * Any serializable object can be set as a value.
     * @param map  A map of property name to property values.
     * @throws JMSException If any of the property names or values are not valid
     * @throws IllegalStateException If the properties are read-only 
     */
    public void putAll(Map<String, Object> map) throws JMSException;
    
    /**
     * Removes a property.
     * @param name The name of the property to remove
     * @return The previous value of the property, which can be null
     * @throws IllegalStateException If the properties are read-only 
     */
    public Object remove(String name);
    
    /**
     * Gets the number of properties in this object.
     * @return The number of properties in this object.
     */
    public int size();
    
    /**
     * Validates all properties for this object.
     * 
     * If no errors are found, a null String array is returned.
     * <p>
     * This form of validate is able to check that combinations of properties are correct.
     * @param nowarn Do not include warning messages in the array of messages returned. 
     * @return A set of ImaJmsException objects indicating the errors found. or null if no errors are found.
     */
    public ImaJmsException [] validate(boolean nowarn);
    
    /**
     * Validates the named property.
     * 
     * The error string consists of a message ID followed by a colon (:) followed by the message text.
     * <p>
     * This form of validate can be used to validate a single property.  It is not able to validate
     * any case two or more properties are invalid in combination.
     * @param name  The name of a property
     * @param nowarn  Do not include warning messages in the array of messages returned.
     * @throws JMSException With the error which was found.  
     *         This JMSException will always an instance of ImaJmsException.
     */
    public void validate(String name, boolean nowarn) throws JMSException; 
    

    /**
     * Validates the named property and value.
     * <p>
     * The error string consists of a message ID followed by a colon (:) followed by the message text.
     * <p>
     * This form of validate can be used to validate a property before it is set.  It can be used in 
     * an administration tool.
     * @param name  The name of a property
     * @param value The value to check
     * @param nowarn Do not include warning messages in the array of messages returned.
     * @throws JMSException With the error which was found.
     *         This JMSException will always an instance of ImaJmsException.
     */
    public void validate(String name, Object value, boolean nowarn) throws JMSException; 
    
    /**
     * Adds a property name to the set of properties to validate.
     * When the properties are validated using one of the validate() methods, it is an error to 
     * have an unknown property.  This method allows you to make the property known and
     * adds a simple validation rule.
     * When a property name is added as a valid property, it is allowed for all instances of the same
     * base object type.
     * <p>
     * The validation of property names is done in a case independent manner.  The name as specified
     * is considered the preferred form.
     * <p>
     * Added properties are not used by the IBM MessageSight JMS client, but are kept along with 
     * the system properties for use by an application.
     * <p>
     * If the name is already a valid property, this method has no effect.
     * <p>
     * The validator can be a string which begins with one of the characters:
     * <dl>
     * <dt>B  Boolean<dd>The property must be a boolean, or an integer, or a string which is one of: 
     *                    on, off, true, false, enable, disable, 0, 1.
     * <dt>I  Integer<dd>The property must be a Number or a String which can be converted to an integer.
     * <dt>U  Unsigned integer <dd>The property must be a Number or a String which can be converted 
     *                    an integer, and must not be negative.
     * <dt>S  String <dd>The property may be of any type, since all types can be converted to String.                                       
     * </dl>
     * @param name  The name of the property to add
     * @param validator The string indicating the type of the property
     * @throws IllegalStateException If the properties are read-only 
     */
    public void addValidProperty(String name, String validator);
}
