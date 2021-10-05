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

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

import com.fasterxml.jackson.annotation.JsonAnySetter;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.ApplianceResource;
import com.ibm.ima.util.IsmLogger;


public class ServerStatusResponse {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    private final static String CLAS = AbstractIsmConfigObject.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    
    private int rc = ApplianceResource.SERVER_STOPPED;
    private String serverState = null;
    private String serverStateStr = null;
    private String modeChanged = null;
    private String modeChangedStr = null;
    private String pendingMode = null;
    private String response = null;
    private String error = null;
    private String adminStateRC = null;
    private String adminStateRCStr = null;
    private String harole = null;
    private String NewRole = null;
    private String OldRole = null;
    private String ActiveNodes = null;
    private String SyncNodes = null;
    private String ReasonCode = null;
    private String ReasonString = null;
    private String PrimaryLastTime = null;
    private String PctSyncCompletion = null;
    private String IsHAEnabled = null;
    private String ServerUPTime = null;
    private String ServerUPTimeStr;
    private String ConfiguredMode = null;
    private String CurrentMode = null;
    private String EnableDiskPersistence = null;
    private String ClusterState = null;
    private String ClusterName = null;
    private String ServerName = null;
    private String ConnectedServers = null;
    private String DisconnectedServers = null;

    public ServerStatusResponse() {

    }
    
    /*
     * Handle any property we don't know about.
     */
    @JsonAnySetter
    public void handleUnknown(String key, Object value) {
        logger.trace(CLAS, "handleUnknown", "Unknown property " + key + " with value " + value + " received");
    }

    @JsonProperty("RC")
    public int getRC() {
        return rc;
    }

    @JsonProperty("RC")
    public void setRC(int rc) {
        this.rc = rc;
    }

    @JsonProperty("ServerState")
    public String getServerState() {
        return serverState;
    }

    @JsonProperty("ServerState")
    public void setServerState(String serverState) {
        this.serverState = serverState;
        if (serverState != null) {
            rc = Integer.parseInt(serverState);
        }
    }

    @JsonProperty("ServerStateStr")
    public String getServerStateStr() {
        return serverStateStr;
    }

    @JsonProperty("ServerStateStr")
    public void setServerStateStr(String serverStateStr) {
        this.serverStateStr = serverStateStr;
    }

    @JsonProperty("ModeChanged")
    public String getModeChanged() {
        return modeChanged;
    }

    @JsonProperty("ModeChanged")
    public void setModeChanged(String modeChanged) {
        this.modeChanged = modeChanged;
    }

    @JsonProperty("ModeChangedStr")
    public String getModeChangedStr() {
        return modeChangedStr;
    }

    @JsonProperty("ModeChangedStr")
    public void setModeChangedStr(String modeChangedStr) {
        this.modeChangedStr = modeChangedStr;
    }

    @JsonProperty("PendingMode")
    public String getPendingMode() {
        return pendingMode;
    }

    @JsonProperty("PendingMode")
    public void setPendingMode(String pendingMode) {
        this.pendingMode = pendingMode;
    }

    /**
     * @return the response
     */
    @JsonProperty("Response")
    public String getResponse() {
        return response;
    }

    /**
     * @param response the response to set
     */
    public void setResponse(String response) {
        this.response = response;
    }

    public String getError() {
        return error;
    }

    public void setError(String error) {
        this.error = error;
    }

    /**
     * @return the adminStateRC
     */
    @JsonProperty("AdminStateRC")
    public String getAdminStateRC() {
        return adminStateRC;
    }

    /**
     * @param adminStateRC the adminStateRC to set
     */
    @JsonProperty("AdminStateRC")
    public void setAdminStateRC(String adminStateRC) {
        this.adminStateRC = adminStateRC;
    }

    /**
     * @return the adminStateRCStr
     */
    @JsonProperty("AdminStateRCStr")
    public String getAdminStateRCStr() {
        return adminStateRCStr;
    }

    /**
     * @param adminStateRCStr the adminStateRCStr to set
     */
    @JsonProperty("AdminStateRCStr")
    public void setAdminStateRCStr(String adminStateRCStr) {
        this.adminStateRCStr = adminStateRCStr;
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

    /**
     * @return the primaryLastTime
     */
    @JsonProperty("PrimaryLastTime")
    public long getPrimaryLastTime() {
        long primaryLastTimeMillis = 0;
        if (PrimaryLastTime != null && PrimaryLastTime.length() > 0) 
        {
            try {
                SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ssX");
                Date date = formatter.parse(PrimaryLastTime);
                primaryLastTimeMillis = date.getTime();
            } catch (ParseException e) {
                logger.trace(CLAS, "getPrimaryLastTime", "Couldn't parse PrimaryLastTime " + PrimaryLastTime);
            }
        }
        return primaryLastTimeMillis;
    }

    /**
     * @param primaryLastTime the primaryLastTime to set
     */
    @JsonProperty("PrimaryLastTime")
    public void setPrimaryLastTime(String primaryLastTime) {        
        PrimaryLastTime = primaryLastTime;
    }
    
    @JsonProperty("PctSyncCompletion")
    public void setPctSyncCompletion(String pctSyncCompletion) {
    	PctSyncCompletion = pctSyncCompletion;
    }
    
    @JsonProperty("PctSyncCompletion")
    public int getPctSyncCompletion() {
    	int pctSyncCompletion = -1;
    	if (PctSyncCompletion != null) {
    		try {
    			pctSyncCompletion = Integer.parseInt(PctSyncCompletion);
    		} catch (NumberFormatException nfe) {
                logger.trace(CLAS, "getPctSyncCompletion", "Couldn't parse PctSyncCompletion " + PctSyncCompletion);    			
    		}
    	}
    	return pctSyncCompletion;
    }
    
    @JsonProperty("IsHAEnabled") 
    public void setIsHAEnabled(String enabled) {
    	IsHAEnabled = enabled;
    }
    
    @JsonProperty("IsHAEnabled") 
    public boolean isHAEnabled() {
    	return "1".equals(IsHAEnabled);
    }
    
    /**
     * @return the ServerUPTime
     */
    @JsonProperty("ServerUPTime")
    public long getServerUPTime() {
        long serverUpTimeSeconds = 0;
        if (ServerUPTime != null && ServerUPTime.length() > 0) 
        {
    		try {
    			serverUpTimeSeconds = Integer.parseInt(ServerUPTime);
    		} catch (NumberFormatException nfe) {
                logger.trace(CLAS, "getServerUPTime", "Couldn't parse ServerUPTime " + ServerUPTime);    			
    		}
        }
        return serverUpTimeSeconds;
    }

    /**
     * @param serverUPTime the ServerUPTime to set
     */
    @JsonProperty("ServerUPTime")
    public void setServerUPTime(String serverUPTime) {        
    	ServerUPTime = serverUPTime;
    }

    @JsonProperty("ServerUPTimeStr")
    public void setServerUPTimeStr(String s) {
        ServerUPTimeStr = s;
    }
    
    @JsonProperty("ServerUPTimeStr")
    public String getServerUPTimeStr() {
        return ServerUPTimeStr;
    }
    
    @JsonProperty("ConfiguredMode") 
    public void setConfiguredMode(String mode) {
        ConfiguredMode = mode;
    }
    
    @JsonProperty("ConfiguredMode")
    public String setConfiguredMode( ) {
        return ConfiguredMode;
    }

    @JsonProperty("CurrentMode") 
    public void setCurrentMode(String mode) {
        CurrentMode = mode;
    }
    
    @JsonProperty("CurrentMode")
    public String setCurrentMode( ) {
        return CurrentMode;
    }

    @JsonProperty("EnableDiskPersistence")
    public String getEnableDiskPersistence() {
		return EnableDiskPersistence;
	}

    @JsonProperty("EnableDiskPersistence")
	public void setEnableDiskPersistence(String enableDiskPersistence) {
		EnableDiskPersistence = enableDiskPersistence;
	}

	/**
	 * @return the clusterState
	 */
    @JsonProperty("ClusterState")
	public String getClusterState() {
		return ClusterState;
	}

	/**
	 * @param clusterState the clusterState to set
	 */
    @JsonProperty("ClusterState")
    public void setClusterState(String clusterState) {
		ClusterState = clusterState;
	}

	/**
	 * @return the clusterName
	 */
    @JsonProperty("ClusterName")    
	public String getClusterName() {
		return ClusterName;
	}

	/**
	 * @param clusterName the clusterName to set
	 */
    @JsonProperty("ClusterName")    
	public void setClusterName(String clusterName) {
		ClusterName = clusterName;
	}

	/**
	 * @return the serverName
	 */
    @JsonProperty("ServerName")    
	public String getServerName() {
		return ServerName;
	}

	/**
	 * @param serverName the serverName to set
	 */
    @JsonProperty("ServerName")    
	public void setServerName(String serverName) {
		ServerName = serverName;
	}

	/**
	 * @return the connectedServers
	 */
    @JsonProperty("ConnectedServers")    
	public String getConnectedServers() {
		return ConnectedServers;
	}

	/**
	 * @param connectedServers the connectedServers to set
	 */
    @JsonProperty("ConnectedServers")        
	public void setConnectedServers(String connectedServers) {
		ConnectedServers = connectedServers;
	}

	/**
	 * @return the disconnectedServers
	 */
    @JsonProperty("DisconnectedServers")        
    public String getDisconnectedServers() {
		return DisconnectedServers;
	}

	/**
	 * @param disconnectedServers the disconnectedServers to set
	 */
    @JsonProperty("DisconnectedServers")            
	public void setDisconnectedServers(String disconnectedServers) {
		DisconnectedServers = disconnectedServers;
	}

	@Override
    public String toString() {
        return "ServerStatusResponse [ServerState=" + serverState +", ServerStateStr=" + serverStateStr +
                ", ModeChanged=" + modeChanged + ", ModeChangedStr=" + modeChangedStr + ", PendingMode=" + pendingMode +
                ", AdminStateRC=" + adminStateRC + ", AdminStateRCStr=" + adminStateRCStr 
                + ", response=" + response + ", error=" + error + ", harole=" + harole +", NewRole=" + NewRole +", OldRole=" + OldRole +
                ", ActiveNodes=" + ActiveNodes + ", SyncNodes=" + SyncNodes + ", ReasonCode=" + ReasonCode
                + ", ReasonString=" + ReasonString + ", PrimaryLastTime=" + PrimaryLastTime + ", ServerUPTime=" + ServerUPTime +
                ", ConfiguredMode=" + ConfiguredMode + ", CurrentMode=" + CurrentMode + ", EnableDiskPersistence=" + EnableDiskPersistence + 
                ", ClusterState=" + ClusterState + ", ClusterName="+ ClusterName + ",ServerName=" + ServerName + 
                ", ConnectedServers=" + ConnectedServers + ", DisconnectedServers=" + DisconnectedServers+"]";
    }

}
