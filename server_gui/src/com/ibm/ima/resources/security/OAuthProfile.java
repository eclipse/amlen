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

package com.ibm.ima.resources.security;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.data.AbstractIsmConfigObject;

/**
 * The REST representation of a LTPA Profile
 *
 */
public class OAuthProfile extends AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @SuppressWarnings("unused")
    private final static String CLAS = OAuthProfile.class.getCanonicalName();

    @JsonProperty("Name")
    private String name;

    @JsonProperty("ResourceURL")
    private String resourceUrl;
    
    @JsonProperty("KeyFileName")
    private String keyFilename;
    
    @JsonProperty("AuthKey")
    private String authKey = "access_token";

    @JsonProperty("UserInfoURL")
    private String userInfoURL;

    @JsonProperty("UserInfoKey")
    private String userInfoKey;

    @JsonProperty("GroupInfoKey")
    private String groupInfoKey;

    
    @JsonIgnore
    private String keyName;    

    public OAuthProfile() {

    }

    /**
     * @param name
     * @param certFilename
     * @param keyFilename
     */
    public OAuthProfile(String name, String keyFilename) {
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
     * @return the resourceUrl
     */
    @JsonProperty("ResourceURL")
    public String getResourceUrl() {
        return resourceUrl;
    }

    /**
     * @param resourceUrl the resourceUrl to set
     */
    @JsonProperty("ResourceURL")
    public void setResourceUrl(String resourceUrl) {
        this.resourceUrl = resourceUrl;
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
     * @return the authKey
     */
    @JsonProperty("AuthKey")
    public String getAuthKey() {
        return authKey;
    }

    /**
     * @param authKey the authKey to set
     */
    @JsonProperty("AuthKey")
    public void setAuthKey(String authKey) {
        this.authKey = authKey;
    }

    /**
     * @return the userInfoURL
     */
    @JsonProperty("UserInfoURL")    
    public String getUserInfoURL() {
        return userInfoURL;
    }

    /**
     * @param userInfoURL the userInfoURL to set
     */
    @JsonProperty("UserInfoURL")    
    public void setUserInfoURL(String userInfoURL) {
        this.userInfoURL = userInfoURL;
    }


    /**
     * @return the userInfoKey
     */
    @JsonProperty("UserInfoKey")
    public String getUserInfoKey() {
        return userInfoKey;
    }

    /**
     * @param userInfoKey the userInfoKey to set
     */
    @JsonProperty("UserInfoKey")
    public void setUserInfoKey(String userInfoKey) {
        this.userInfoKey = userInfoKey;
    }

    /**
     * @return the groupInfoKey
     */
    @JsonProperty("GroupInfoKey")
    public String getGroupInfoKey() {
        return groupInfoKey;
    }

    /**
     * @param groupInfoKey the groupInfoKey to set
     */
    @JsonProperty("GroupInfoKey")
    public void setGroupInfoKey(String groupInfoKey) {
        this.groupInfoKey = groupInfoKey;
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
        ValidationResult result = validateName(this.name, "Name");
        if (!result.isOK()) {
            return result;
        }
        if (!validateUrl(result, resourceUrl, "ResourceURL", true)) {
            return result;
        }
        if (!validateUrl(result, userInfoURL, "UserInfoURL", false)) {
            return result;
        }
        if (!validateProperty(result, authKey, "AuthKey", true)) {
            return result;
        }
        if (!validateProperty(result, userInfoKey, "UserInfoKey", false)) {
            return result;
        } 
        if (!validateProperty(result, groupInfoKey, "GroupInfoKey", false)) {
            return result;
        }                               
        return result;
    }

    private boolean validateUrl(ValidationResult result, String url, String propertyName, boolean required) {
        if (url == null || trimUtil(url).length() == 0) {
            if (required) {
                result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
                result.setParams(new Object[] {propertyName});            
                return false;
            } else {
                return true;
            }
        } 
        if (!checkUtfLength(url, 2048)) {
            result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG);
            result.setParams(new Object[] {propertyName, 2048});
            return false;
        }
        url = url.toLowerCase();
        if (!url.startsWith("http://") && !url.startsWith("https://")) {
            result.setResult(VALIDATION_RESULT.INVALID_URL_SCHEME);
            result.setParams(new Object[] {propertyName, "http, https"});
            return false;
        }
        return true;
    }
    
    private boolean validateProperty(ValidationResult result, String property, String propertyName, boolean required) {
        if (property == null || trimUtil(property).length() == 0) {
            if (required) {
                result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
                result.setParams(new Object[] {propertyName});
                return false;
            } else {
                return true;
            }
        }  
        
        result = validateName(property, propertyName);
        if (!result.isOK()) {
        	return false;
        }
        
        return true;
        
    }

    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "OAuthProfile [name=" + getName() + ", resourceUrl =" + getResourceUrl() +
                ", keyFilename=" + getKeyFilename() + ", authKey=" + getAuthKey() + 
                ", userInfoURL=" + getUserInfoURL() + ", userInfoKey=" + getUserInfoKey() + ", groupInfoKey=" + getGroupInfoKey() +"]";
    }

}
