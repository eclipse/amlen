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

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.data.AbstractIsmConfigObject;

/**
 * The REST representation of a certificate response
 *
 */
public class CertificateProfile extends AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @SuppressWarnings("unused")
    private final static String CLAS = CertificateProfile.class.getCanonicalName();

    @JsonProperty("Name")
    private String name;

    @JsonProperty("Certificate")
    private String certFilename;

    @JsonProperty("Key")
    private String keyFilename;

    @JsonProperty("ExpirationDate")
    private String expirationDate;

    @JsonProperty("ExpirationMilliseconds")
    private long certificateExpiry = -1;

    @JsonIgnore
    private String keyName;

    public CertificateProfile() {

    }

    /**
     * @param name
     * @param certFilename
     * @param keyFilename
     */
    public CertificateProfile(String name, String certFilename, String keyFilename) {
        setName(name);
        setCertFilename(certFilename);
        setKeyFilename(keyFilename);
    }

    /**
     * @return the expirationDate
     */
    @JsonProperty("ExpirationDate")
    public String getExpirationDate() {
        return expirationDate;
    }

    /**
     * @param expirationDate the expirationDate to set
     */
    @JsonProperty("ExpirationDate")
    public void setExpirationDate(String expirationDate) {
        this.expirationDate = expirationDate;
        SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mmZ");
        try {
            Date date = df.parse(this.expirationDate);
            this.certificateExpiry = date.getTime();
        } catch (ParseException e) {}

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
     * @return the certificate
     */
    @JsonProperty("Certificate")
    public String getCertFilename() {
        return certFilename;
    }


    /**
     * @param certificate the certificate filename to set
     */
    @JsonProperty("Certificate")
    public void setCertFilename(String certificate) {
        certFilename = certificate;
    }


    /**
     * @return the key
     */
    @JsonProperty("Key")
    public String getKeyFilename() {
        return keyFilename;
    }


    /**
     * @param key the key filename to set
     */
    @JsonProperty("Key")
    public void setKeyFilename(String key) {
        this.keyFilename = key;
    }

    /**
     * @return the certificateExpiry
     */
    @JsonProperty("ExpirationMilliseconds")
    public long getCertificateExpiry() {
        return certificateExpiry;
    }

    /**
     * @param certificateExpiry the certificateExpiry to set
     */
    @JsonProperty("ExpirationMilliseconds")
    public void setCertificateExpiry(long certificateExpiry) {
        this.certificateExpiry = certificateExpiry;

        SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mmZ");
        Date date = new Date(certificateExpiry);
        this.expirationDate = df.format(date);

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
            // check to make sure the certificates exist
            if (certFilename == null || trimUtil(certFilename).length() == 0) {
                result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
                result.setParams(new Object[] {"certFilename"});
            } else if (keyFilename == null || trimUtil(keyFilename).length() == 0) {
                result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
                result.setParams(new Object[] {"keyFilename"});
            } else {
                // if the certificate extension indicates pkcs12, reject it
                String certName = certFilename.toLowerCase();
                if (certName.endsWith(".p12") || certName.endsWith(".pfx")) {
                    result.setResult(VALIDATION_RESULT.INVALID_CHARS);
                    result.setErrorMessageID("CWLNA5080");
                    result.setParams(new Object[] {certFilename});
                }
            }
        }

        return result;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "CertificateProfile [name=" + this.getName() + ", keyName =" + this.keyName +
                ", certFilename=" + this.getCertFilename() + ", keyFilename=" + this.getKeyFilename() +
                ", expirationDate=" + this.expirationDate +"]";
    }

}
