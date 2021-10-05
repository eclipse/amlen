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

/*
 * A MQTT Extension Field is a primitive structure matching the C mqtt_ext_field_t 
 */
package com.ibm.ism.mqtt;

/*
 * MQTT property
 */
public class IsmMqttProperty {
    public  int    id;
    public  int    type;
    public  int    version;
    public  int    valid;
    public  String name;
    
    /*
     * Types 
     */
    final static int Undef    = 0;
    final static int Int1     = 1;
    final static int Int2     = 2;
    final static int Int4     = 3;
    final static int String   = 4;
    final static int Bytes    = 5;
    final static int NamePair = 6;
    final static int Boolean  = 7;
    final static int VarInt   = 8;
    
    /*
     * Constructor for field
     */
    IsmMqttProperty(int id, int type, int version, int valid, String name) {
        this.id = id;
        this.type = type;
        this.version = version;
        this.valid = valid;
        this.name = name;
    }
}
