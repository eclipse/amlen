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

package com.ibm.ima.resources.data;

import java.util.Arrays;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * REST representation of run modes. Used to set or get change
 * the run mode of the server.
 *
 */
public class RunMode extends  AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

	
    @SuppressWarnings("unused")
    private final static String CLAS = RunMode.class.getCanonicalName();

    /**
     * Valid modes available
     *
     */
    public enum ValidModes {
    	
    	PRODUCTION("0"),
    	MAINTENANCE("1"),
    	CLEAN_STORE("2");
    	
		private String modeNumber;


		ValidModes(String modeNumber) {
			this.modeNumber = modeNumber;
		}


		public String getModeNumber() {
			return this.modeNumber;
		}
		
    }

    @JsonProperty("Mode")
    private String Mode;
   

    /**
     * Get the runmode value that will be passed to the
     * server.
     * 
     * @return the run mode
     */
    @JsonProperty("Mode")
    public String getMode() {
    	ValidModes modeValue = ValidModes.valueOf(Mode);
        return modeValue.getModeNumber();
    }
    

    /**
     * Get the string representation of the runmode.
     * 
     * @return A string value that represents the runmode
     */
    @JsonIgnore
    public String getModeString() {
    	return Mode;
    }
    
    /**
     * Set the runmode value
     * 
     * @param Mode the run mode
     */
    @JsonProperty("Mode")
    public void setMode(String Mode) {
        this.Mode = Mode;
    }
    
    
    @Override
	public String toString() {
		return "RunMode [Mode=" + Mode + "]";
	}

    @Override
    @JsonIgnore
    public ValidationResult validate() {
    	
    	// must be valid mode and not empty
        ValidationResult vr = new ValidationResult();
        if (Mode == null || trimUtil(Mode).length() == 0) {
            vr.setResult(VALIDATION_RESULT.VALUE_EMPTY);
            return vr;
        }
        try {
            ValidModes.valueOf(Mode);
        } catch (Exception e) {
            vr.setResult(VALIDATION_RESULT.INVALID_ENUM);
            vr.setParams(new Object[] {Mode, Arrays.toString(ValidModes.values())});
        }

        return vr;
    }

}
