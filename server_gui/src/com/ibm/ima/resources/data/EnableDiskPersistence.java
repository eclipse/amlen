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
package com.ibm.ima.resources.data;

import com.fasterxml.jackson.annotation.JsonProperty;


public class EnableDiskPersistence  {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @SuppressWarnings("unused")
    private final static String CLAS = EnableDiskPersistence.class.getCanonicalName();

    @JsonProperty("EnableDiskPersistence")
    public boolean EnableDiskPersistence = false;

    public EnableDiskPersistence() {
    }

    /**
     * @return the EnableDiskPersistence
     */
    @JsonProperty("EnableDiskPersistence")
    public boolean isEnableDiskPersistence() {
        return EnableDiskPersistence;
    }

    /**
     * @param enableDiskPersistence the enableDiskPersistence to set
     */
    @JsonProperty("EnableDiskPersistence")
    public void setEnableDiskPersistence(boolean enableDiskPersistence) {
    	EnableDiskPersistence = enableDiskPersistence;
    }


    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "EnableDiskPersistence [EnableDiskPersistence=" + EnableDiskPersistence + "]";
    }
}
