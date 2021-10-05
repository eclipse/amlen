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

import com.ibm.ima.resources.data.LDAPConnection;

@SuppressWarnings("unused")
public class IsmConfigSetLDAPConnRequest extends IsmConfigRequest {

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
    private String Name = "ldapconfig";
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Delete;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Update;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String NewName;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String URL;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String IgnoreCase;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String BaseDN;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String BindDN;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String BindPassword;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String UserSuffix;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String GroupSuffix;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String UserIdMap;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String GroupIdMap;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String GroupMemberIdMap;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private int Timeout;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private int CacheTimeout;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private int GroupCacheTimeout;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Enabled;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String EnableCache;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String SearchUserDN;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String NestedGroupSearch;
    

    public IsmConfigSetLDAPConnRequest(String username) {
        this.Action = "Set";
        this.Component = "Security";
        this.User = username;
        this.Type = "Composite";
        this.Item = "LDAP";
    }

    public void setFieldsFromLDAPConnection(LDAPConnection ldapConnection) {
        this.URL = ldapConnection.getUrl();
        this.IgnoreCase = ldapConnection.getIgnoreCase() ? "true" : "false";
        this.BaseDN = ldapConnection.getBaseDN();
        this.BindDN = ldapConnection.getBindDN();
        this.BindPassword = ldapConnection.getBindPassword();
        this.UserSuffix = ldapConnection.getUserSuffix();
        this.GroupSuffix = ldapConnection.getGroupSuffix();
        this.UserIdMap = ldapConnection.getUserIdMap();
        this.GroupIdMap = ldapConnection.getGroupIdMap();
        this.GroupMemberIdMap = ldapConnection.getGroupMemberIdMap();
        this.Timeout = ldapConnection.getTimeout();
        this.CacheTimeout = ldapConnection.getCacheTimeout();
        this.GroupCacheTimeout = ldapConnection.getGroupCacheTimeout();
        this.Enabled = ldapConnection.getEnabled() ? "true" : "false";
        this.EnableCache = ldapConnection.getEnableCache() ? "true" : "false";
        this.NestedGroupSearch = ldapConnection.getNestedGroupSearch() ? "true" : "false";

        if (this.BindPassword != null && this.BindPassword.length() > 0 ) {
            this.setMask("BindPassword\":\"" + this.BindPassword, "BindPassword\":\"*******");
        }
    }

    public void setDelete() {
        this.Delete = "true";
    }

    public void setUpdate() {
        this.Update = "true";
    }

    @Override
    public String toString() {
        return "IsmConfigSetLDAPConnRequest [Type=" + Type + ", Item=" + Item + ", Name=" + Name + ", Delete=" + Delete + ", Update=" + Update + ", LDAPConnectionName=" + Name + "]";
    }
}
