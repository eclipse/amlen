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
package com.ibm.ima.resources.data;

import java.io.File;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.LDAPResource;

public class LDAPConnection extends AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private static final int TIMEOUT_MAX_VALUE = 60;
    private static final int TIMEOUT_MIN_VALUE = 1;
    private static final int CACHETTL_MAX_VALUE = 60;
    private static final int CACHETTL_MIN_VALUE = 1;
    private static final int GROUPCACHETTL_MAX_VALUE = 86400;
    private static final int GROUPCACHETTL_MIN_VALUE = 1;

    private String name = "ldapconfig";
    private String url;
    private String newName;
    private String certificate = null;
    private String ignoreCase = "true";
    private String baseDN;
    private String bindDN;
    private String bindPassword;
    private String userSuffix;
    private String groupSuffix;
    private String userIdMap;
    private String groupIdMap;
    private String groupMemberIdMap;
    private int timeout = -1;
    private int cacheTimeout = -1;
    private int groupcachetimeout = -1;
    private String enabled = "false";
    private String enableCache = "true";
    private String nestedGroupSearch = "false";

    @JsonProperty("Name")
    public String getName() {
        return name;
    }

    @JsonProperty("Name")
    public void setName(String name) {
        this.name = name;
    }

    @JsonIgnore
    public String getNewName() {
        return newName;
    }

    @JsonIgnore
    public void setNewName(String newName) {
        this.newName = newName;
    }

    @JsonProperty("URL")
    public String getUrl() {
        return url;
    }

    @JsonProperty("URL")
    public void setUrl(String url) {
        this.url = url;
    }

    @JsonProperty("Certificate")
    public String getCertificate() {
        if (certificate == null) {
            // set the certificate if it exists in the keystore
            File cert = new File(LDAPResource.LDAP_KEYSTORE, LDAPResource.CERT_NAME);
            if (cert.exists()) {
                certificate = LDAPResource.CERT_NAME;
            }
        }

        return certificate;
    }

    @JsonProperty("Certificate")
    public void setCertificate(String certificate) {
        this.certificate = certificate;
    }

    @JsonProperty("IgnoreCase")
    public boolean getIgnoreCase() {
        if (ignoreCase.toLowerCase().equals("true")) {
            return true;
        }
        return false;    
    }

    @JsonProperty("IgnoreCase")
    public void setIgnoreCase(String ignoreCase) {
        this.ignoreCase = ignoreCase;
    }

    @JsonProperty("BaseDN")
    public String getBaseDN() {
        return baseDN;
    }

    @JsonProperty("BaseDN")
    public void setBaseDN(String baseDN) {
        this.baseDN = baseDN;
    }

   @JsonProperty("BindDN")
    public String getBindDN() {
        return bindDN;
    }

    @JsonProperty("BindDN")
    public void setBindDN(String bindDN) {
        this.bindDN = bindDN;
    }

    @JsonProperty("BindPassword")
    public String getBindPassword() {
        return bindPassword;
    }

    @JsonProperty("BindPassword")
    public void setBindPassword(String bindPassword) {
        this.bindPassword = bindPassword;
    }

    @JsonProperty("UserSuffix")
    public String getUserSuffix() {
        return userSuffix;
    }

    @JsonProperty("UserSuffix")
    public void setUserSuffix(String userSuffix) {
        this.userSuffix = userSuffix;
    }

    @JsonProperty("GroupSuffix")
    public String getGroupSuffix() {
        return groupSuffix;
    }

    @JsonProperty("GroupSuffix")
    public void setGroupSuffix(String groupSuffix) {
        this.groupSuffix = groupSuffix;
    }

    @JsonProperty("UserIdMap")
    public String getUserIdMap() {
        return userIdMap;
    }

    @JsonProperty("UserIdMap")
    public void setUserIdMap(String userIdMap) {
        this.userIdMap = userIdMap;
    }


    @JsonProperty("GroupIdMap")
    public String getGroupIdMap() {
        return groupIdMap;
    }

    @JsonProperty("GroupIdMap")
    public void setGroupIdMap(String groupIdMap) {
        this.groupIdMap = groupIdMap;
    }

    @JsonProperty("GroupMemberIdMap")
    public String getGroupMemberIdMap() {
        return groupMemberIdMap;
    }

    @JsonProperty("GroupMemberIdMap")
    public void setGroupMemberIdMap(String groupMemberIdMap) {
        this.groupMemberIdMap = groupMemberIdMap;
    }


    @JsonProperty("Timeout")
    public int getTimeout() {
        return timeout;
    }

    @JsonProperty("Timeout")
    public void setTimeout(int timeout) {
        this.timeout = timeout;
    }

    @JsonProperty("CacheTimeout")
    public int getCacheTimeout() {
        return cacheTimeout;
    }

    @JsonProperty("CacheTimeout")
    public void setCacheTtl(int cacheTimeout) {
        this.cacheTimeout = cacheTimeout;
    }

    @JsonProperty("GroupCacheTimeout")
    public int getGroupCacheTimeout() {
        return groupcachetimeout;
    }

    @JsonProperty("GroupCacheTimeout")
    public void setGroupCacheTimeout(int groupcachetimeout) {
        this.groupcachetimeout = groupcachetimeout;
    }

    @JsonProperty("Enabled")
    public boolean getEnabled() {
        if (enabled.toLowerCase().equals("true")) {
            return true;
        }
        return false;
    }

    @JsonProperty("Enabled")
    public void setEnabled(String enabled) {
        this.enabled = enabled;
    }

    @JsonProperty("EnableCache")
    public boolean getEnableCache() {
        if (enableCache.toLowerCase().equals("true")) {
            return true;
        }
        return false;
    }

    @JsonProperty("EnableCache")
    public void setEnableCache(String enableCache) {
        this.enableCache = enableCache;
    }

    @JsonProperty("NestedGroupSearch")
    public boolean getNestedGroupSearch() {
        if (nestedGroupSearch.toLowerCase().equals("true")) {
            return true;
        }
        return false;
    }

    @JsonProperty("NestedGroupSearch")
    public void setNestedGroupSearch(String nestedGroupSearch) {
        this.nestedGroupSearch = nestedGroupSearch;
    }


    @Override
    public ValidationResult validate() {
        ValidationResult result = super.validateName(name, "Name");
        if (!result.isOK()) {
            return result;
        }

        // validate timeout is in range
        if (timeout < TIMEOUT_MIN_VALUE || timeout > TIMEOUT_MAX_VALUE) {
            result.setResult(VALIDATION_RESULT.VALUE_OUT_OF_RANGE);
            result.setParams(new Object[] { "Timeout", TIMEOUT_MIN_VALUE, TIMEOUT_MAX_VALUE });
            return result;
        }

        // validate cachettl is in range
        if (cacheTimeout < CACHETTL_MIN_VALUE || cacheTimeout > CACHETTL_MAX_VALUE) {
            result.setResult(VALIDATION_RESULT.VALUE_OUT_OF_RANGE);
            result.setParams(new Object[] { "CacheTimeout", CACHETTL_MIN_VALUE, CACHETTL_MAX_VALUE });
            return result;
        }

        // validate groupcachettl is in range
        if (groupcachetimeout < GROUPCACHETTL_MIN_VALUE || groupcachetimeout > GROUPCACHETTL_MAX_VALUE) {
            result.setResult(VALIDATION_RESULT.VALUE_OUT_OF_RANGE);
            result.setParams(new Object[] { "GroupCacheTimeout", GROUPCACHETTL_MIN_VALUE, GROUPCACHETTL_MAX_VALUE });
            return result;
        }

        return result;
    }
}
