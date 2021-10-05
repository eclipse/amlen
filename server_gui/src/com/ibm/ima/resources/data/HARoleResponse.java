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

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.HARoleResource;


public class HARoleResponse {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    public static final String PRIMARY = "PRIMARY";
    public static final String STANDBY = "STANDBY";
    
    private int rc = HARoleResource.ROLE_REQUEST_FAILED;
    private String harole = null;
    private String NewRole = null;
    private String OldRole = null;
    private String ActiveNodes = null;
    private String SyncNodes = null;
    private String ReasonCode = null;
    private String ReasonString = null;
    private String LocalReplicationNIC = null;
    private String LocalDiscoveryNIC = null;
    private String RemoteDiscoveryNIC = null;
    private String RemoteWebUI = null;

    /**
     * @param rc
     * @param harole
     */
    public HARoleResponse(int rc, String harole) {
        this.rc = rc;
        this.harole = harole;
    }

    public HARoleResponse() {

    }

    @JsonProperty("RC")
    public int getRC() {
        return rc;
    }

    public void setRC(int rc) {
        this.rc = rc;
    }

    /**
     * @return the HA Role
     */
    public String getHARole() {
        return harole;
    }

    /**
     * @param harole the HA Role to set
     */
    public void setHARole(String harole) {
        this.harole = harole;
    }

    @JsonProperty("NewRole")
    public String getNewRole() {
        return NewRole;
    }

    @JsonProperty("NewRole")
    public void setNewRole(String newRole) {
        NewRole = newRole;
    }

    @JsonProperty("OldRole")
    public String getOldRole() {
        return OldRole;
    }

    @JsonProperty("OldRole")
    public void setOldRole(String oldRole) {
        OldRole = oldRole;
    }

    @JsonProperty("ActiveNodes")
    public String getActiveNodes() {
        return ActiveNodes;
    }

    @JsonProperty("ActiveNodes")
    public void setActiveNodes(String activeNodes) {
        ActiveNodes = activeNodes;
    }

    @JsonProperty("SyncNodes")
    public String getSyncNodes() {
        return SyncNodes;
    }

    @JsonProperty("SyncNodes")
    public void setSyncNodes(String syncNodes) {
        SyncNodes = syncNodes;
    }

    @JsonProperty("ReasonCode")
    public String getReasonCode() {
        return ReasonCode;
    }

    @JsonProperty("ReasonCode")
    public void setReasonCode(String errorCode) {
        ReasonCode = errorCode;
    }

    @JsonProperty("ReasonString")
    public String getReasonString() {
        return ReasonString;
    }

    @JsonProperty("ReasonString")
    public void setReasonString(String errorString) {
        ReasonString = errorString;
    }
    
    @JsonProperty("LocalReplicationNIC")
    public String getLocalReplicationNIC() {
		return LocalReplicationNIC;
	}

    @JsonProperty("LocalReplicationNIC")
	public void setLocalReplicationNIC(String localReplicationNIC) {
		LocalReplicationNIC = localReplicationNIC;
	}

    @JsonProperty("LocalDiscoveryNIC")
	public String getLocalDiscoveryNIC() {
		return LocalDiscoveryNIC;
	}

    @JsonProperty("LocalDiscoveryNIC")
	public void setLocalDiscoveryNIC(String localDiscoveryNIC) {
		LocalDiscoveryNIC = localDiscoveryNIC;
	}

    @JsonProperty("RemoteDiscoveryNIC")
	public String getRemoteDiscoveryNIC() {
		return RemoteDiscoveryNIC;
	}

    @JsonProperty("RemoteDiscoveryNIC")
	public void setRemoteDiscoveryNIC(String remoteDiscoveryNIC) {
		RemoteDiscoveryNIC = remoteDiscoveryNIC;
	}
    
    
    @JsonProperty("RemoteWebUI")
	public String getRemoteWebUI() {
		return RemoteWebUI;
	}

    @JsonProperty("RemoteWebUI")
	public void setRemoteWebUI(String remoteWebUI) {
		RemoteWebUI = remoteWebUI;
	}

	/**
     * Returns true if the NewRole is PRIMARY, regardless of case. 
     * If the role is not set, or is not PRIMARY, false will be returned.
     */
    @JsonIgnore
    public boolean isPrimary() {
        if (NewRole != null && NewRole.toUpperCase().equals(PRIMARY)) {
            return true;
        }
        return false;
    }

    /**
     * Returns true if the NewRole is STANDBY, regardless of case. 
     * If the role is not set, or is not STANDBY, false will be returned.
     */
    @JsonIgnore
    public boolean isStandby() {
        if (NewRole != null && NewRole.toUpperCase().equals(STANDBY)) {
            return true;
        }
        return false;
    }

    @Override
    public String toString() {
        return "HARoleResponse [harole=" + harole +", NewRole=" + NewRole +", OldRole=" + OldRole +
                ", ActiveNodes=" + ActiveNodes + ", SyncNodes=" + SyncNodes + ", ReasonCode=" + ReasonCode
                + ", ReasonString=" + ReasonString + ", LocalReplicationNIC=" + LocalReplicationNIC + ", LocalDiscoveryNIC=" + LocalDiscoveryNIC 
                + ", RemoteDiscoveryNIC=" + RemoteDiscoveryNIC  + ", RemoteWebUI=" + RemoteWebUI + "]";
    }
}
