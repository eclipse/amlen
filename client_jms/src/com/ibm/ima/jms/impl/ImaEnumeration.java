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

/**
 * Define a set of enumerated names and values used in IMA properties.
 */
public class ImaEnumeration {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    boolean   allowValue;
    int       count;
    String [] names;
    int []    values;
    String    namestr;
    
    /* 
     * A value to indicate that the enumeration is unknown.
     */
    public static final int UNKNOWN = -99999;

    
    /*
     *  Define an enumeration.
     *  @param allowvalue Allow the name to be a string parsable to the integer value
     *  @param def  A string array with alternating name and value string.  
     *              The value must be parseable to an integer.
     */
    public ImaEnumeration(boolean allowvalue, String [] def) {
        int  i;
        
        allowValue = allowvalue;
        count = def.length / 2;
        names = new String[count];
        values = new int[count];
        for (i=0; i<count; i++) {
            names[i] = def[i*2];
            namestr = i==0 ? names[i] : namestr + ", " + names[i];
            values[i] = Integer.parseInt(def[i*2+1]);
        }
    }
    
    public ImaEnumeration(String [] def) {
        this(true, def);
    }
    
    /*
     *  Get the name of an enumeration given its value.
     *  @param The enumerated value
     *  @return the name of the enumeration, or null to indicate that the value is not known.
     *          If a value has multiple names, the first one defined is returned.
     */
    public String getName(int value) {
        int  i;
        for (i=0; i<count; i++) {
            if (value == values[i])
                return names[i];
        }
        return null;
    }
    
    /*
     *  Get the value of an enumeration given its name.
     *  @param The enumerated name
     *  @return the value of the enumeration, or ImaEnumeration.UNKNOWN to indicate that the name 
     *          is not known.
     */
    public int getValue(String name) {
        int  i;
        
        if (name == null)
            return UNKNOWN;
        if (allowValue && name.length() > 0) {
            char ch = name.charAt(0);
            if (ch >= '0' && ch <= '9') {
                try {
                    int val = Integer.parseInt(name);
                    if (getName(val) != null)
                        return val;
                } catch (Exception e) { }    
            }
        }
        for (i=0; i<count; i++) {
            if (name.equalsIgnoreCase(names[i]))
                return values[i];
        }   
        return UNKNOWN;
    }
    
    /*
     * Return the names for the enumeration as a comma separated list of names
     */
    public String getNameString() {
        return namestr;
    }
}

