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

import com.ibm.ima.resources.security.CertificateProfile;

public class IsmConfigSetCertificateProfileRequest extends IsmConfigRequest {

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
    
    @JsonSerialize
    private String Certificate;

    @JsonSerialize
    private String Key;

    @JsonSerialize
    private String ExpirationDate;



    public IsmConfigSetCertificateProfileRequest(String userId) {
        this(userId, null);
    }

    /**
     * @param certificateProfile
     */
    public IsmConfigSetCertificateProfileRequest(String userId, CertificateProfile certificateProfile) {
        this.Action = "SET";
        this.Component = "Transport";
        this.User = userId;
        this.Type = "Composite";
        this.Item = "CertificateProfile";
        setFieldsFromCertificateProfile(certificateProfile);
    }

    /**
     * Set fields from the provided certificate profile
     */
    public void setFieldsFromCertificateProfile(CertificateProfile certificateProfile) {
        if (certificateProfile != null) {
            if (certificateProfile.getKeyName() != null && !certificateProfile.getKeyName().equals(certificateProfile.getName())) {
                this.NewName = certificateProfile.getName();
                this.Name = certificateProfile.getKeyName();
            } else {
                this.Name = certificateProfile.getName();
            }

            this.Certificate = certificateProfile.getCertFilename();
            this.Key = certificateProfile.getKeyFilename();
            this.ExpirationDate = certificateProfile.getExpirationDate();
        }
    }

    public void setUpdate() {
        this.Update = "true";
    }


    @Override
    public String toString() {
        return "IsmConfigSetCertificateProfileRequest [Type=" + Type + ", Item=" + Item
                + ", Update=" + Update
                + ", Name=" + Name + ", NewName=" + NewName + ", Certificate="
                + Certificate + ", Key=" + Key + ", ExpirationDate="+ ExpirationDate +"]";
    }

}
