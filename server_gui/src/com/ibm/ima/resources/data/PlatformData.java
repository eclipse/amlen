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

import com.fasterxml.jackson.annotation.JsonAnySetter;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;


/**
 * The REST representation of PlatformData as returned by "imacli <locale> get PlatformData"
 *
 */
public class PlatformData {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


	
    private final static String CLAS = PlatformData.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
   
    @JsonProperty("IsVM")
    private String isVM;
    
    @JsonProperty("PlatformType")
    private String platformType;
    
    @JsonProperty("CurrentLicenseType")
    private String currentLicenseType;

    @JsonProperty("AllowedLicenseTypes")
    private String allowedLicenseTypes;
    
    @JsonProperty("PlatformSerialNumber")
    private String platformSerialNumber;
    
    /*
     * Handle any property we don't know about.
     */
    @JsonAnySetter
    public void handleUnknown(String key, Object value) {
        logger.trace(CLAS, "handleUnknown", "Unknown property " + key + " with value " + value + " received");
    }    
  
    @JsonProperty("IsVM")    
    public boolean getIsVM() {
    	boolean b = false;
    	if ("0".equals(isVM)) {
    		b = false;
    	} else if ("1".equals(isVM)) {
    		b = true;
    	} else {
    		b = Utils.getUtils().isVirtualAppliance();
    	}	
		return b;
	}

    @JsonProperty("IsVM")
	public void setIsVM(String isVM) {
		this.isVM = isVM;
	}

    @JsonProperty("PlatformType")    
	public String getPlatformType() {
		return platformType;
	}

    @JsonProperty("PlatformType")
	public void setPlatformType(String platformType) {
		this.platformType = platformType;
	}

    @JsonProperty("CurrentLicenseType")
	public String getCurrentLicenseType() {
		return currentLicenseType;
	}

    @JsonProperty("CurrentLicenseType")
	public void setCurrentLicenseType(String currentLicenseType) {
		this.currentLicenseType = currentLicenseType;
	}

    @JsonProperty("AllowedLicenseTypes")
	public String[] getAllowedLicenseTypes() {
    	if (allowedLicenseTypes == null || allowedLicenseTypes.isEmpty()) {
    		return new String[0];
    	}
		return allowedLicenseTypes.split(",");
	}

    @JsonProperty("AllowedLicenseTypes")
	public void setAllowedLicenseTypes(String allowedLicenseTypes) {
		this.allowedLicenseTypes = allowedLicenseTypes;
	}

    @JsonProperty("PlatformSerialNumber")
	public String getPlatformSerialNumber() {
		return platformSerialNumber;
	}

    @JsonProperty("PlatformSerialNumber")
	public void setPlatformSerialNumber(String platformSerialNumber) {
		this.platformSerialNumber = platformSerialNumber;
	}

	@Override
	public String toString() {
		return "PlatformData [IsVM=" + isVM + ", PlatformType=" + platformType +
				", CurrentLicenseType=" + currentLicenseType + 
				", AllowedLicenseTypes=" + allowedLicenseTypes +
				", PlatformSerialNumber=" + platformSerialNumber +  "]";
	}

}
