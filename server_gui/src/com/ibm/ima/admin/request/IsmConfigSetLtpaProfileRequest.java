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

package com.ibm.ima.admin.request;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize.Inclusion;

import com.ibm.ima.resources.security.LtpaProfile;

public class IsmConfigSetLtpaProfileRequest extends IsmConfigRequest {

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
    private String Delete;


    @JsonSerialize
    private String KeyFileName;

    @JsonSerialize
    private String Password;


    public IsmConfigSetLtpaProfileRequest(String userId) {
        this(userId, null);
    }

    /**
     * @param ltpaProfile
     */
    public IsmConfigSetLtpaProfileRequest(String userId, LtpaProfile ltpaProfile) {
        this.Action = "SET";
        this.Component = "Security";
        this.User = userId;
        this.Type = "Composite";
        this.Item = "LTPAProfile";
        setFieldsFromLtpaProfile(ltpaProfile);
    }

    /**
     * Set fields from the provided LTPA profile
     */
    public void setFieldsFromLtpaProfile(LtpaProfile ltpaProfile) {
        if (ltpaProfile != null) {
            if (ltpaProfile.getKeyName() != null && !ltpaProfile.getKeyName().equals(ltpaProfile.getName())) {
                this.NewName = ltpaProfile.getName();
                this.Name = ltpaProfile.getKeyName();
            } else {
                this.Name = ltpaProfile.getName();
            }

            this.KeyFileName = ltpaProfile.getKeyFilename();
            this.Password = ltpaProfile.getPasswordPrivate();
            
            if (this.Password != null && this.Password.length() > 0 ) {
                this.setMask("Password\":\"" + this.Password, "Password\":\"*******");
            }

        }
    }

	public void setName(String name) {
		this.Name = name;
	}

    public void setUpdate() {
        this.Update = "true";
    }


	/**
	 * @param delete the delete to set
	 */
	public void setDelete() {
		Delete = "true";
	}

	@Override
    public String toString() {
        return "IsmConfigSetLtpaProfileRequest [Type=" + Type + ", Item=" + Item
                + ", Update=" + Update + ", Delete=" + Delete 
                + ", Name=" + Name + ", NewName=" + NewName 
                + ", KeyFileName=" + KeyFileName + "]";
    }
}
