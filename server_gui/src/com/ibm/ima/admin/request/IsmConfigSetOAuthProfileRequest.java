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

package com.ibm.ima.admin.request;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize.Inclusion;

import com.ibm.ima.resources.security.OAuthProfile;

public class IsmConfigSetOAuthProfileRequest extends IsmConfigRequest {

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
    private String ResourceURL;
  
    
    @JsonSerialize
    private String KeyFileName;

    @JsonSerialize
    private String AuthKey;
    
    @JsonSerialize
    private String UserInfoURL;
    
    @JsonSerialize
    private String UserInfoKey;
    
    @JsonSerialize
    private String GroupInfoKey;


    public IsmConfigSetOAuthProfileRequest(String userId) {
        this(userId, null);
    }

    /**
     * @param oAuthProfile
     */
    public IsmConfigSetOAuthProfileRequest(String userId, OAuthProfile oAuthProfile) {
        this.Action = "SET";
        this.Component = "Security";
        this.User = userId;
        this.Type = "Composite";
        this.Item = "OAuthProfile";
        setFieldsFromOAuthProfile(oAuthProfile);
    }

    /**
     * Set fields from the provided OAuth profile
     */
    public void setFieldsFromOAuthProfile(OAuthProfile oAuthProfile) {
        if (oAuthProfile != null) {
            if (oAuthProfile.getKeyName() != null && !oAuthProfile.getKeyName().equals(oAuthProfile.getName())) {
                this.NewName = oAuthProfile.getName();
                this.Name = oAuthProfile.getKeyName();
            } else {
                this.Name = oAuthProfile.getName();
            }

            this.ResourceURL = oAuthProfile.getResourceUrl();
            this.KeyFileName = oAuthProfile.getKeyFilename();
            this.AuthKey = oAuthProfile.getAuthKey();
            this.UserInfoURL = oAuthProfile.getUserInfoURL();
            this.UserInfoKey = oAuthProfile.getUserInfoKey();
            this.GroupInfoKey = oAuthProfile.getGroupInfoKey();
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
        return "IsmConfigSetOAuthProfileRequest [Type=" + Type + ", Item=" + Item
                + ", Update=" + Update + ", Delete=" + Delete 
                + ", Name=" + Name + ", NewName=" + NewName 
                + ", ResourceURL=" + ResourceURL + ", AuthKey=" + AuthKey
                + ", UserInfoURL=" + UserInfoURL + ", UserInfoKey=" + UserInfoKey
                + ", GroupInfoKey=" + GroupInfoKey
                + ", KeyFileName=" + KeyFileName + "]";
    }
}
