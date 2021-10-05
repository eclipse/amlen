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

package com.ibm.ima.resources.security;

import com.fasterxml.jackson.annotation.JsonProperty;


/**
 * The REST representation of a certificate response
 *
 */
public class Fips  {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @SuppressWarnings("unused")
    private final static String CLAS = Fips.class.getCanonicalName();

    @JsonProperty("FIPS")
    public boolean FIPS = false;

    public Fips() {
    }

    /**
     * @return the fIPS
     */
    @JsonProperty("FIPS")
    public boolean isFIPS() {
        return FIPS;
    }

    /**
     * @param fIPS the fIPS to set
     */
    @JsonProperty("FIPS")
    public void setFIPS(boolean fIPS) {
        FIPS = fIPS;
    }




    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "Fips [FIPS=" + FIPS + "]";
    }

}
