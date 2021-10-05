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

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;


/**
 * The REST representation of a certificate request
 *
 */
public class LibertyCertificate {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    @SuppressWarnings("unused")
    private final static String CLAS = LibertyCertificate.class.getCanonicalName();    

    @SuppressWarnings("unused")
    @JsonIgnore
    private String Name;

    @JsonProperty("Certificate")
    private String certificate;
    
    @JsonProperty("Key")
    private String key;
    
    @JsonProperty("CertificatePassword")
    private String certificatePassword;
    
    @JsonProperty("KeyPassword")
    private String keyPassword;
    
    private String expirationDate = null;
    
    public LibertyCertificate() {

    }

    /**
     * @return the certificate
     */
    @JsonProperty("Certificate")
    public String getCertificate() {
        return certificate;
    }


    /**
     * @param certificate the certificate to set
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

    /**
     * @return the expirationDate
     */
    public String getExpirationDate() {
        return expirationDate;
    }
    
    /**
     * @param expirationDate Certificate expiration date to set
     */
    public void setExpirationDate(String expirationDate) {
        this.expirationDate = expirationDate;
    }
}
