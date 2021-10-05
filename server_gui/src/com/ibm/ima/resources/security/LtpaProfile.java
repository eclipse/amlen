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

package com.ibm.ima.resources.security;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.data.AbstractIsmConfigObject;

/**
 * The REST representation of a LTPA Profile
 *
 */
public class LtpaProfile extends AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @SuppressWarnings("unused")
    private final static String CLAS = LtpaProfile.class.getCanonicalName();

    @JsonProperty("Name")
    private String name;

    @JsonProperty("KeyFileName")
    private String keyFilename;
    
    @JsonIgnore
    private String password;

    @JsonIgnore
    private String keyName;    

    public LtpaProfile() {

    }

    /**
     * @param name
     * @param certFilename
     * @param keyFilename
     */
    public LtpaProfile(String name, String keyFilename) {
        setName(name);
        setKeyFilename(keyFilename);
    }

    /**
     * @return the name
     */
    @JsonProperty("Name")
    public String getName() {
        return name;
    }

    /**
     * @param name the name to set
     * @see #validateName(String, String)
     */
    @JsonProperty("Name")
    public void setName(String name) {
        this.name = name;
    }

    /**
     * @return the key
     */
    @JsonProperty("KeyFileName")
    public String getKeyFilename() {
        return keyFilename;
    }

    /**
     * @param key the key filename to set
     */
    @JsonProperty("KeyFileName")
    public void setKeyFilename(String key) {
        this.keyFilename = key;
    }
    
    /**
	 * @return the password
	 */
    @JsonIgnore
	public String getPasswordPrivate() {
		return password;
	}
    
	/**
	 * @param password the password to set
	 */
    @JsonProperty("Password")
	public void setPassword(String password) {
		this.password = password;
	}

	/**
     * @return the keyName
     */
    @JsonIgnore
    public String getKeyName() {
        return keyName;
    }

    /**
     * @param keyName the keyName to set
     */
    @JsonIgnore
    public void setKeyName(String keyName) {
        this.keyName = keyName;
    }

    /* (non-Javadoc)
     * @see com.ibm.ima.resources.data.AbstractIsmConfigObject#validate()
     */
    @Override
    public ValidationResult validate() {
        // check the name
        ValidationResult result = validateName(this.name, "Name");
        if (result.isOK()) {
            // check to make sure the keyfile exists
            if (keyFilename == null || trimUtil(keyFilename).length() == 0) {
                result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
                result.setParams(new Object[] {"KeyFileName"});
            } 
        }

        return result;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "LtpaProfile [name=" + this.getName() + ", keyName =" + this.keyName +
                ", keyFilename=" + this.getKeyFilename() + "]";
    }

}
