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

package com.ibm.ima.admin.request;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize.Inclusion;

import com.ibm.ima.resources.security.SecurityProfile;

public class IsmConfigSetSecurityProfileRequest extends IsmConfigRequest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @JsonSerialize
    private String Type;
    @JsonSerialize
    private String Item;
    @JsonSerialize
    private String Name;

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String NewName;

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Update;

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MinimumProtocolMethod = "TLSv1.2";

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String UseClientCertificate = "false";

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Ciphers = "Fast";

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String CertificateProfile;

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String LTPAProfile;
    
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String OAuthProfile;

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String UseClientCipher = "false";

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String UsePasswordAuthentication = "True";
    
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String TLSEnabled = "True";


    public IsmConfigSetSecurityProfileRequest(String userId) {
        this(userId, null);
    }

    /**
     * @param securityProfile
     */
    public IsmConfigSetSecurityProfileRequest(String userId, SecurityProfile securityProfile) {
        this.Action = "SET";
        this.Component = "Transport";
        this.User = userId;
        this.Type = "Composite";
        this.Item = "SecurityProfile";
        setFieldsFromSecurityProfile(securityProfile);
    }

    /**
     * Set fields from the provided certificate profile
     */
    public void setFieldsFromSecurityProfile(SecurityProfile securityProfile) {
        if (securityProfile != null) {
            if (securityProfile.getKeyName() != null && !securityProfile.getKeyName().equals(securityProfile.getName())) {
                this.NewName = securityProfile.getName();
                this.Name = securityProfile.getKeyName();
            } else {
                this.Name = securityProfile.getName();
            }

            this.CertificateProfile = securityProfile.getCertificateProfileName();
            this.Ciphers = securityProfile.getCiphers();
            this.MinimumProtocolMethod = securityProfile.getMinimumProtocolMethod().toString();
            this.UseClientCertificate = securityProfile.isClientCertificateUsed();
            this.UseClientCipher = securityProfile.isClientCipherUsed();
            this.UsePasswordAuthentication = securityProfile.getUsePasswordAuthentication();
            this.TLSEnabled = securityProfile.getTLSEnabled();
            this.LTPAProfile = securityProfile.getLTPAProfile();
            this.OAuthProfile = securityProfile.getoAuthProfile();
        }
    }

    public void setUpdate() {
        this.Update = "true";
    }


    @Override
    public String toString() {
        return "IsmConfigSetSecurityProfileRequest [Type=" + Type + ", Item=" + Item
                + ", Name=" + Name + ", NewName=" + NewName
                + ", Update=" + Update + ", CertificateProfile="+ CertificateProfile 
                + ", LTPAProfile="+ LTPAProfile + ", OAuthProfile="+ OAuthProfile + ", Ciphers=" + Ciphers 
                + ", MinimumProtocolMethod=" + MinimumProtocolMethod + 
                ", UseClientCertificate=" + UseClientCertificate + ",UseClientCipher=" 
                + UseClientCipher + "UsePasswordAuthenticcation=" + UsePasswordAuthentication + "TLSEnabled=" + TLSEnabled +"]";
    }


}
