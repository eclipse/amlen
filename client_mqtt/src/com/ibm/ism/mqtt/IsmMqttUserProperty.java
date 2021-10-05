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

public class IsmMqttUserProperty {
    private final String  name;
    private final String  value;
   
    /**
     * Create a property
     * @param name  The name of the property
     * @param value The value of the property
     */
    public IsmMqttUserProperty(String name, String value) {
        this.name = name;
        this.value = value;
    }
    
    /**
     * Return the name of the property
     * @return The name of the property
     */
    public String name() {
        return name;
    }
    
    /**
     * Return the value of the property
     * @return The value of the property
     */
    public String value() {
        return value;
    }
}
