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

import java.util.ArrayList;
import java.util.Arrays;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.exceptions.IsmRuntimeException;



/**
 * The REST representation of a the LicensedUsage
 *
 */
public class LicensedUsage extends  AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


	
    @SuppressWarnings("unused")
    private final static String CLAS = LicensedUsage.class.getCanonicalName();

    public static final String DEVELOPERS = "Developers";
    public static final String NON_PRODUCTION = "Non-Production";
    public static final String PRODUCTION = "Production";
    public static final ArrayList<String> LICENSED_USAGE_TYPES = new ArrayList<String>(Arrays.asList(DEVELOPERS, NON_PRODUCTION, PRODUCTION));

    
    @JsonProperty("LicensedUsage")
    private String licensedUsage = DEVELOPERS;
    
    /**
     * @return the LicensedUsage
     */
    @JsonProperty("LicensedUsage")
    public String getLicensedUsage() {
        return licensedUsage;
    }
    
    /**
     * @param licensedUsage the LicensedUsage to set
     */
    @JsonProperty("LicensedUsage")
    public void setLicensedUsage(String licensedUsage) {
        this.licensedUsage = licensedUsage;
    }
    
    @Override
	public String toString() {
		return "LicensedUsage [LicensedUsage=" + licensedUsage +  "]";
	}

    @Override
    @JsonIgnore
    public ValidationResult validate() throws IsmRuntimeException {
        ValidationResult vr = new ValidationResult();
        if (licensedUsage == null || trimUtil(licensedUsage).length() == 0) {
            vr.setResult(VALIDATION_RESULT.VALUE_EMPTY);
            return vr;
        }
        if (!LICENSED_USAGE_TYPES.contains(licensedUsage)) {
        	vr.setResult(VALIDATION_RESULT.INVALID_ENUM);
        	vr.setParams(new Object[] {licensedUsage, LICENSED_USAGE_TYPES.toString()});
            return vr;
        }
        
//        if (!Utils.getUtils().isVirtualAppliance()) {
//        	// not valid on a physical appliance
//            throw new IsmRuntimeException(Status.FORBIDDEN, "CWLNA5151", null);        	
//        }

        return vr;
    }

}
