/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ism.mqtt;

/**
 * MQTT Property checker.  
 * 
 * This is used to check that the values of an individual MQTT property is correct.
 * There are three separate but similar methods based on the type of the field.
 * If the value is not valid, the invoker should throw a RuntimeException and put 
 * information about what is not valid as the message data.
 * 
 * This meth
 */
public interface IsmMqttPropChecker {
    /**
     * Check one property with an integer value
     * @param ctx   The property context
     * @param fld   The property definition
     * @param value The integer value of the field 
     * @throws RuntimeException if the field value is not valid
     */
    void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, int value);
    
    /**
     * Check one property with a string value
     * @param ctx   The property context
     * @param fld   The property definition
     * @param value The string value of the field
     * @throws RuntimeException if the field value is not valid
     */
    void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, String str);
    
    /**
     * Check one property with an byte array value
     * @param ctx   The property context
     * @param fld   The property definition
     * @param value The byte array value of the field
     * @throws RuntimeException if the field value is not valid
     */
    void check(IsmMqttPropCtx cts, IsmMqttProperty fld, byte [] ba);
    
    /**
     * Check a user property
     * @param ctx    The property context
     * @param fld    The property definition 
     * @param name   The user property name
     * @param value  The user property value
     */
    void check(IsmMqttPropCtx ctx, IsmMqttProperty fld, String name, String value);
}
