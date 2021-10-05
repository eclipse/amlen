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

import java.util.Arrays;
import java.util.List;

import javax.ws.rs.core.Response.Status;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.HAPairResource;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.ima.util.Utils;

/*
 * The REST representation of a server HAPair object
 */
@JsonIgnoreProperties

public class HAObject extends AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private String startupMode;
    private String group;
    private String remoteDiscoveryNIC;
    private String localReplicationNIC;
    private String localDiscoveryNIC;
    private long discoveryTimeout;
    private long heartbeatTimeout;
    private boolean EnableHA = false;
    private boolean PreferredPrimary = false;
    private String AllowSingleNIC = "0";
    
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    private final static String CLAS = HAObject.class.getCanonicalName();
    
    public HAObject() {
    }

    @JsonProperty("StartupMode")
    public String getStartupMode() {
        return startupMode;
    }

    @JsonProperty("StartupMode")
    public void setStartupMode(String startupMode) {
        this.startupMode = startupMode;
    }
    
    @JsonProperty("Group")
    public String getGroup() {
		return group;
	}

    @JsonProperty("Group")
	public void setGroup(String group) {
		this.group = group;
	}

	@JsonProperty("RemoteDiscoveryNIC")
    public String getRemoteDiscoveryNIC() {
        return remoteDiscoveryNIC;
    }

    @JsonProperty("RemoteDiscoveryNIC")
    public void setRemoteDiscoveryNIC(String remoteDiscoveryNIC) {
        this.remoteDiscoveryNIC = remoteDiscoveryNIC;
    }

    @JsonProperty("LocalReplicationNIC")
    public String getLocalReplicationNIC() {
        return localReplicationNIC;
    }

    @JsonProperty("LocalReplicationNIC")
    public void setLocalReplicationNIC(String localReplicationNIC) {
        this.localReplicationNIC = localReplicationNIC;
    }

    @JsonProperty("LocalDiscoveryNIC")
    public String getLocalDiscoveryNIC() {
        return localDiscoveryNIC;
    }

    @JsonProperty("LocalDiscoveryNIC")
    public void setLocalDiscoveryNIC(String localDiscoveryNIC) {
        this.localDiscoveryNIC = localDiscoveryNIC;
    }
    @JsonProperty("DiscoveryTimeout")
    public long getDiscoveryTimeout() {
        return discoveryTimeout;
    }

    @JsonProperty("DiscoveryTimeout")
    public void setDiscoveryTimeout(String discoveryTimeout) {
        this.discoveryTimeout = Long.valueOf(discoveryTimeout);
    }

    @JsonProperty("HeartbeatTimeout")
    public long getHeartbeatTimeout() {
        return heartbeatTimeout;
    }

    @JsonProperty("HeartbeatTimeout")
    public void setHeartbeatTimeout(String heartbeatTimeout) {
        this.heartbeatTimeout = Long.valueOf(heartbeatTimeout);
    }

    @JsonProperty("EnableHA")
    public boolean isEnabled() {
        return EnableHA;
    }

    @JsonProperty("EnableHA")
    public void setEnabled(String enabledString) {
        this.EnableHA = Boolean.parseBoolean(enabledString);
    }
    
    @JsonProperty("PreferredPrimary")
    public boolean isPrimary() {
        return PreferredPrimary;
    }

    @JsonProperty("PreferredPrimary")
    public void setPrimary(String primaryString) {
        this.PreferredPrimary = Boolean.parseBoolean(primaryString);
    }

    @JsonProperty("AllowSingleNIC")
    public boolean isAllowSingleNIC() {
        if (this.AllowSingleNIC == null || !this.AllowSingleNIC.equals("1")) {
            return false;
        }
        return true;
    }

    @JsonProperty("AllowSingleNIC")
    public void setAllowSingleNIC(String allowSingleNIC) {
        this.AllowSingleNIC = allowSingleNIC;
    }

    
    @Override
    public String toString() {
        return "HAPair [startupMode=" + startupMode
                + ", remoteDiscoveryNIC=" + remoteDiscoveryNIC + ", localReplicationNIC=" + localReplicationNIC
                + ", localDiscoveryNIC=" + localDiscoveryNIC + ", discoveryTimeout=" + discoveryTimeout
                + ", heartbeatTimeout=" + heartbeatTimeout + ", EnableHA=" + EnableHA + ", Group=" + group 
                + ", PreferredPrimary=" +PreferredPrimary +"]";
    }

    @Override
    public ValidationResult validate() {
        ValidationResult result = new ValidationResult();
        result.setResult(VALIDATION_RESULT.OK);
        
        // check to make sure the Group is not empty when EnableHA is true
        if ((EnableHA == true) && (group == null || trimUtil(group).length() == 0)) {
            result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
            result.setParams(new Object[] {"Group"});
            return result;
        } 

        if (!checkUtfLength(this.group, 128)) {
        	logger.log(LogLevel.LOG_ERR, CLAS, "validate", "CWLNA5131");
        	throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5131", new Object[] {});
        }
        
        if (this.localDiscoveryNIC != null && this.localReplicationNIC != null && this.remoteDiscoveryNIC != null) {
        	List<String> ipList = Arrays.asList(this.localDiscoveryNIC,this.localReplicationNIC,this.remoteDiscoveryNIC);
            Utils.validateIPAddresses(ipList, false);
        } else if (this.localDiscoveryNIC == null && this.localReplicationNIC == null && this.remoteDiscoveryNIC == null) {
        	// all can be null - do nothing...
        } else {
        	logger.log(LogLevel.LOG_ERR, CLAS, "validate", "CWLNA5130");
        	throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5130", new Object[] {});
        }
       
        if ((this.discoveryTimeout < HAPairResource.MIN_DISCOVERY_TIMEOUT)||(this.discoveryTimeout > HAPairResource.MAX_TIMEOUT)) {
            result.setResult(VALIDATION_RESULT.VALUE_OUT_OF_RANGE);
            result.setParams(new Object[] {"Discovery Timeout", HAPairResource.MIN_DISCOVERY_TIMEOUT, HAPairResource.MAX_TIMEOUT});
        }
        if ((this.heartbeatTimeout < HAPairResource.MIN_HEARTBEAT_TIMEOUT)||(this.heartbeatTimeout > HAPairResource.MAX_TIMEOUT)) {
            result.setResult(VALIDATION_RESULT.VALUE_OUT_OF_RANGE);
            result.setParams(new Object[] {"Heartbeat Timeout", HAPairResource.MIN_HEARTBEAT_TIMEOUT, HAPairResource.MAX_TIMEOUT});
        }
        return result;
    }

}
