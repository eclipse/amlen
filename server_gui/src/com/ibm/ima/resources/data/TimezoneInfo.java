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
package com.ibm.ima.resources.data;


@SuppressWarnings({ "rawtypes" })
public class TimezoneInfo implements Comparable {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    public static final String IDENTIFIER = "timezone";
    public static final String LABEL = "dispalyName";

    String timezone = null;
    String displayName = null;
     
    
    /**
     * @param timezone
     * @param displayName
     */
    public TimezoneInfo(String timezone, String displayName) {
        super();
        this.timezone = timezone;
        this.displayName = displayName;
    }
    /**
     * @return the timezone
     */
    public String getTimezone() {
        return timezone;
    }
    /**
     * @param timezone the timezone to set
     */
    public void setTimezone(String timezone) {
        this.timezone = timezone;
    }
    /**
     * @return the displayName
     */
    public String getDisplayName() {
        return displayName;
    }
    /**
     * @param displayName the displayName to set
     */
    public void setDisplayName(String displayName) {
        this.displayName = displayName;
    }
    
    @Override
    public int compareTo(Object o) {
        if (o instanceof TimezoneInfo) {
            String a = displayName;
            String b = ((TimezoneInfo)o).getDisplayName();
            if (a != null && b!= null) {
                return a.compareTo(b);
            }             
        }
        return 0;
    }
    
    /* (non-Javadoc)
     * @see java.lang.Object#equals(java.lang.Object)
     */
    @Override
    public boolean equals(Object o) {
        if (o != null && o instanceof TimezoneInfo) {
            if (this.compareTo((TimezoneInfo)o) == 0) {
                return true;
            }
        }
        return false;
    }
    
    @Override
    public int hashCode() {
        if (displayName == null) {
            return 0;
        }
        return displayName.hashCode();
    }
    
    
    
    
}
