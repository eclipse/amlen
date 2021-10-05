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
package com.ibm.ima.resources.data;

import com.ibm.ima.util.Utils;
import com.fasterxml.jackson.databind.JsonNode;

public class UserSecurityAndPrefsResponse {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private String username;
    private String[] userRoles;
    private JsonNode preferences;
    private int licenseStatus;
    private JsonNode localeInfo;
    private boolean virtual = false;
    private boolean cci = false;
    private boolean docker = false;
    private String defaultAdminServerAddress = "127.0.0.1";
    private String defaultAdminServerPort = "9089";

    public UserSecurityAndPrefsResponse(String username, String[] userRoles) {
        this.username = username;
        this.userRoles = userRoles;
        Utils utils = Utils.getUtils();
        setVirtual(utils.isVirtualAppliance());
        setCci(utils.isCCIEnvironment());
        setDocker(utils.isDockerContainer());
        setDefaultAdminServerAddress(utils.getDefaultAdminServerAddress());
        setDefaultAdminServerPort(utils.getDefaultAdminServerPort());
    }

    public String getUsername() {
        return this.username;
    }

    public String[] getUserRoles() {
        return this.userRoles;
    }

    public JsonNode getPreferences() {
        return preferences;
    }

    public void setPreferences(JsonNode preferences) {
        this.preferences = preferences;
    }

    public int getLicenseStatus() {
        return licenseStatus;
    }

    public void setLicenseStatus(int licenseStatus) {
        this.licenseStatus = licenseStatus;
    }

    public void setLocaleInfo(JsonNode localeInfo) {
        this.localeInfo = localeInfo;
    }

    public JsonNode getLocaleInfo() {
        return localeInfo;
    }

    /**
     * @return the virtual
     */
    public boolean isVirtual() {
        return virtual;
    }

    /**
     * @param virtual the virtual to set
     */
    public void setVirtual(boolean virtual) {
        this.virtual = virtual;
    }

    /**
     * @return true if running in cci, false otherwise
     */
    public boolean isCci() {
        return cci;
    }

    /**
     * @param cci the cci setting
     */
    public void setCci(boolean cci) {
        this.cci = cci;
    }
    
    /**
     * @return the docker setting
     */
    public boolean isDocker() {
        return docker;
    }

    /**
     * @param docker the docker to set
     */
    public void setDocker(boolean docker) {
        this.docker = docker;
    }

	public String getDefaultAdminServerAddress() {
		return defaultAdminServerAddress;
	}

	public void setDefaultAdminServerAddress(String defaultAdminServerAddress) {
		this.defaultAdminServerAddress = defaultAdminServerAddress;
	}

	/**
	 * @return the defaultAdminServerPort
	 */
	public String getDefaultAdminServerPort() {
		return defaultAdminServerPort;
	}

	/**
	 * @param defaultAdminServerPort the defaultAdminServerPort to set
	 */
	public void setDefaultAdminServerPort(String defaultAdminServerPort) {
		this.defaultAdminServerPort = defaultAdminServerPort;
	}


}
