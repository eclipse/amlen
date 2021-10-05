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

import java.util.regex.Pattern;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.data.AbstractIsmConfigObject;
import com.ibm.ima.util.IsmLogger;

/**
 * The REST representation of a certificate response
 *
 */
public class SecurityProfile extends AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @SuppressWarnings("unused")
    private final static String CLAS = SecurityProfile.class.getCanonicalName();
    @SuppressWarnings("unused")
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    protected final static Pattern NOT_UNICODE_ALPHANUMERIC_CHAR_PATTERN = Pattern.compile("[^\\p{N}\\p{L}]", Pattern.CANON_EQ);
    protected final static Pattern STARTS_WITH_UNICODE_NUMBER_PATTERN = Pattern.compile("^\\p{N}.*", Pattern.CANON_EQ);


    @JsonProperty("Name")
    private String name;

    @JsonProperty("MinimumProtocolMethod")
    private String minimumProtocolMethod = "TLSv1";

    @JsonProperty("UseClientCertificate")
    private String clientCertificateUsed = "False";

    @JsonProperty("Ciphers")
    private String ciphers = "Medium";

    @JsonProperty("CertificateProfile")
    private String certificateProfileName;

    @JsonProperty("UseClientCipher")
    private String clientCipherUsed = "False";

    @JsonProperty("UsePasswordAuthentication")
    private String usePasswordAuthentication = "True";
    
    @JsonProperty("TLSEnabled")
    private String tlsEnabled = "True";

    @JsonIgnore
    private String keyName;
    
    @JsonProperty("LTPAProfile")
	private String ltpaProfile = null;

    @JsonProperty("OAuthProfile")
    private String oAuthProfile = null;

    
    public SecurityProfile() {
    }


    /**
     * @param name
     * @param minimumProtocolMethod
     * @param useClientCertificate
     * @param ciphers
     * @param certificateProfileName
     */
    public SecurityProfile(String name, String minimumProtocolMethod, String useClientCertificate,
            String ciphers, String certificateProfileName) {
        super();
        this.name = name;
        this.minimumProtocolMethod = minimumProtocolMethod;
        this.setClientCertificateUsed(useClientCertificate);
        this.ciphers = ciphers;
        this.certificateProfileName = certificateProfileName;
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
     * @throws IllegalArgumentException if the name is invalid
     * @see #validateName(String, String)
     */
    @JsonProperty("Name")
    public void setName(String name)  {
        this.name = name;
    }


    /**
     * @return the minimumProtocolMethod
     */
    @JsonProperty("MinimumProtocolMethod")
    public String getMinimumProtocolMethod() {
        return minimumProtocolMethod;
    }


    /**
     * @param minimumProtocolMethod the minimumProtocolMethod to set
     */
    @JsonProperty("MinimumProtocolMethod")
    public void setMinimumProtocolMethod(String minimumProtocolMethod) {
        this.minimumProtocolMethod = minimumProtocolMethod;
    }


    /**
     * @return the clientCertificateUsed
     */
    @JsonIgnore
    public String isClientCertificateUsed() {
        return clientCertificateUsed;
    }



    /**
     * @return the clientCertificateUsed
     */
    @JsonProperty("UseClientCertificate")
    public String getClientCertificateUsed() {
        return clientCertificateUsed;
    }

    /**
     * @param clientCertificateUsed the clientCertificateUsed to set
     */
    @JsonProperty("UseClientCertificate")
    public void setClientCertificateUsed(String clientCertificateUsed) {
        this.clientCertificateUsed = clientCertificateUsed;
    }

    @JsonProperty("MissingTrustedCertificates")
    public boolean getMissingTrustedCertificates() {
        // if client certificate used, check for uploaded certs
        if (getName() != null && Boolean.parseBoolean(isClientCertificateUsed())) {
            if (!SecurityProfilesResource.hasTrustedCertificates(getName())) {
                return true;
            }
        }
        return false;
    }
    

    /**
     * @return the ciphers
     */
    @JsonProperty("Ciphers")
    public String getCiphers() {
        return ciphers;
    }


    /**
     * @param ciphers the ciphers to set
     */
    @JsonProperty("Ciphers")
    public void setCiphers(String ciphers) {
        this.ciphers = ciphers;
    }


    /**
     * @return the certificateProfileName
     */
    @JsonProperty("CertificateProfile")
    public String getCertificateProfileName() {
        return certificateProfileName;
    }


    /**
     * @param certificateProfileName the certificateProfileName to set
     */
    @JsonProperty("CertificateProfile")
    public void setCertificateProfileName(String certificateProfileName) {
        this.certificateProfileName = certificateProfileName;
    }

    @JsonProperty("LTPAProfile")
    public String getLTPAProfile() {
		return ltpaProfile;
	}
    
    @JsonProperty("LTPAProfile")
    public void setLTPAProfile(String ltpaProfile) {
		this.ltpaProfile = ltpaProfile;
	}

    /**
     * @return the oAuthProfile
     */
    @JsonProperty("OAuthProfile")
    public String getoAuthProfile() {
        return oAuthProfile;
    }


    /**
     * @param oAuthProfile the oAuthProfile to set
     */
    @JsonProperty("OAuthProfile")
    public void setoAuthProfile(String oAuthProfile) {
        this.oAuthProfile = oAuthProfile;
    }


    /**
     * @return the clientCipherUsed
     */
    @JsonIgnore
    public String isClientCipherUsed() {
        return clientCipherUsed;
    }


    /**
     * @return the clientCipherUsed
     */
    @JsonProperty("UseClientCipher")
    public String getClientCipherUsed() {
        return clientCipherUsed;
    }

    /**
     * @param clientCipherUsed the clientCipherUsed to set
     */
    @JsonProperty("UseClientCipher")
    public void setClientCipherUsed(String clientCipherUsed) {
        this.clientCipherUsed = clientCipherUsed;
    }

    /**
     * @return the usePasswordAuthentication
     */
    @JsonProperty("UsePasswordAuthentication")
    public String getUsePasswordAuthentication() {
        return usePasswordAuthentication;
    }

    /**
     * @param usePasswordAuthentication the usePasswordAuthentication to set
     */
    @JsonProperty("UsePasswordAuthentication")
    public void setUsePasswordAuthentication(String usePasswordAuthentication) {
        this.usePasswordAuthentication = usePasswordAuthentication;
    }
    
    /**
     * @return the tlsEnabled
     */
    @JsonProperty("TLSEnabled")
    public String getTLSEnabled() {
        return tlsEnabled;
    }

    /**
     * @param tlsEnabled the tlsEnabled to set
     */
    @JsonProperty("TLSEnabled")
    public void setTLSEnabled(String tlsEnabled) {
        this.tlsEnabled = tlsEnabled;
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
        if (!result.isOK()) {
            return result;
        }
        // check to make sure a certificateProfile is specified and is a valid name
        if (tlsEnabled.equalsIgnoreCase("true")) {
            if (certificateProfileName == null || trimUtil(certificateProfileName).length() == 0) {
                result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
                result.setParams(new Object[] {"CertificateProfile"});
                return result;
            }
            result = super.validateName(certificateProfileName, "CertificateProfile");
            if (!result.isOK()) {
                return result;
            }
        }
        
        // if an ltpaProfile is defined, then usePasswordAuthentication must be true and OAuthProfile must 
        // not be set
        if (ltpaProfile != null && !ltpaProfile.isEmpty()) {
            if (!Boolean.parseBoolean(usePasswordAuthentication)) {
            	result.setResult(VALIDATION_RESULT.UNEXPECTED_ERROR);
            	result.setErrorMessageID("CWLNA5112");
            	return result;
            } else if (oAuthProfile != null && !oAuthProfile.isEmpty()) {
                result.setResult(VALIDATION_RESULT.UNEXPECTED_ERROR);
                result.setErrorMessageID("CWLNA5134");
                return result;                
            }
        } 

        // if an oAuth is defined, then usePasswordAuthentication must be true and LTPAProfile must 
        // not be set
        if (oAuthProfile != null && !oAuthProfile.isEmpty()) {
            if (!Boolean.parseBoolean(usePasswordAuthentication)) {
                result.setResult(VALIDATION_RESULT.UNEXPECTED_ERROR);
                result.setErrorMessageID("CWLNA5135");
                return result;
            // this check is probably overkill, don't think we'd ever get here
            } else if (ltpaProfile != null && !ltpaProfile.isEmpty()) {
                result.setResult(VALIDATION_RESULT.UNEXPECTED_ERROR);
                result.setErrorMessageID("CWLNA5136");
                return result;                
            }
        }        
        
        return result;
    }

    /**
     * Validates the name. If the name is null, contains only blanks,
     * is too long, or contains invalid characters, returns a ValidationResult object.
     * @param name The name to validate
     * @param propertyName The name of the property being validated
     * @return ValidationResult
     */
    @Override
    public ValidationResult validateName(String name, String propertyName)  {
        ValidationResult result = new ValidationResult();
        if (propertyName == null) {
            propertyName = "name";
        }
        String trimmedName = trimUtil(name);
        if (trimmedName == null || trimmedName.length() == 0) {
            result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
            result.setParams(new Object[] {propertyName});
        } else if (!trimmedName.equals(name)) {
            result.setResult(VALIDATION_RESULT.LEADING_OR_TRAILING_BLANKS);
            result.setParams(new Object[] {propertyName});
        } else if (!AbstractIsmConfigObject.checkUtfLength(name, 32)) {
            result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG);
            result.setParams(new Object[] {propertyName, 32});
        } else if (!isFirstCharValid(name, NAME_MINIMUM_INITIAL_CHAR) || matches(name, STARTS_WITH_UNICODE_NUMBER_PATTERN)){
            result.setResult(VALIDATION_RESULT.INVALID_START_CHAR);
            result.setParams(new Object[] {propertyName, ""});
        } else {
            String invalidChar = containsChars(name, NOT_UNICODE_ALPHANUMERIC_CHAR_PATTERN);
            if (invalidChar != null) {
                result.setResult(VALIDATION_RESULT.INVALID_CHARS);
                result.setParams(new Object[] {propertyName, invalidChar});
            }
        }

        return result;
    }


    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "SecurityProfile [name=" + this.getName() + ", keyName =" + this.keyName +
                ", certificateProfileName=" + this.certificateProfileName + 
                ", ltpaProfile=" + this.ltpaProfile + ", oAuthProfile=" + this.oAuthProfile +", ciphers=" +
                this.ciphers + ", clientCertificateUsed=" + this.clientCertificateUsed +
                ", clientCipherUsed=" + this.clientCipherUsed + ", UsePasswordAuthentication="
                + this.usePasswordAuthentication + ", minimumProtocolMethod="
                + this.minimumProtocolMethod + ", TLSEnabled=" + this.tlsEnabled +"]";
    }

}
