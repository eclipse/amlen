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

import com.fasterxml.jackson.annotation.JsonAnySetter;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.util.IsmLogger;


/**
 * The REST representation of a certificate response
 *
 */
public class CertificateProfileRequest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    @SuppressWarnings("unused")
    private final static String CLAS = CertificateProfileRequest.class.getCanonicalName();    
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
   
    @JsonProperty("Name")
    private String name;
    
    @JsonProperty("Certificate")
    private String certificate;
    
    @JsonProperty("Key")
    private String key;
    
    @JsonProperty("CertificatePassword")
    private String certificatePassword;
    
    @JsonProperty("KeyPassword")
    private String keyPassword;
        
    public CertificateProfileRequest() {

    }
    
    /*
     * Handle any property we don't know about.
     */
    @JsonAnySetter
    public void handleUnknown(String key, Object value) {
        logger.trace(this.getClass().getCanonicalName(), "handleUnknown", "Unknown property " + key + " with value " + value + " received");
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
     */
    @JsonProperty("Name")
    public void setName(String name) {
        this.name = name;
    }


    /**
     * @return the certicate
     */
    @JsonProperty("Certificate")
    public String getCertificate() {
        return certificate;
    }


    /**
     * @param certicate the certicate to set
     */
    @JsonProperty("Certificate")    
    public void setCertificate(String certicate) {
        this.certificate = certicate;
    }


    /**
     * @return the key
     */
    @JsonProperty("Key")    
    public String getKey() {
        return key;
    }


    /**
     * @param key the key to set
     */
    @JsonProperty("Key")    
    public void setKey(String key) {
        this.key = key;
    }


    /**
     * @return the certificatePassword
     */
    @JsonProperty("CertificatePassword")
    public String getCertificatePassword() {
        return certificatePassword;
    }


    /**
     * @param certificatePassword the certificatePassword to set
     */
    @JsonProperty("CertificatePassword")
    public void setCertificatePassword(String certificatePassword) {
        this.certificatePassword = certificatePassword;
    }


    /**
     * @return the keyPassword
     */
    @JsonProperty("KeyPassword")
    public String getKeyPassword() {
        return keyPassword;
    }


    /**
     * @param keyPassword the keyPassword to set
     */
    @JsonProperty("KeyPassword")
    public void setKeyPassword(String keyPassword) {
        this.keyPassword = keyPassword;
    }    

}
