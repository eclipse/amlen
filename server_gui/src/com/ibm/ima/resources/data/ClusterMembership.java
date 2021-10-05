/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

/*
 * The REST representation of a server HAPair object
 */
@JsonIgnoreProperties
public class ClusterMembership extends AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private boolean enableClusterMembership = false;
    private String clusterName;
    private boolean useMulticastDiscovery = true;
    private int multicastDiscoveryTTL = 1;
    private String discoveryPort = "9104";
    private int discoveryTime = 10;
    private String joinClusterWithMembers;
    private String controlAddress;
    private String controlPort = "9102";
    private String controlExternalAddress;
    private String controlExternalPort;
    private String messagingAddress;
    private String messagingPort = "9103";
    private String messagingExternalAddress;
    private String messagingExternalPort;
    private boolean messagingUseTLS = false;
    private String serverUID;
    
    public ClusterMembership() {
    }

    /**
	 * @return the enableClusterMembership
	 */
    @JsonProperty("EnableClusterMembership")
	public boolean isEnableClusterMembership() {
		return enableClusterMembership;
	}

	/**
	 * @param enableClusterMembership the enableClusterMembership to set
	 */
    @JsonProperty("EnableClusterMembership")
    public void setEnableClusterMembership(String enableClusterMembership) {
		this.enableClusterMembership = Boolean.parseBoolean(enableClusterMembership);
	}

	/**
	 * @return the clusterName
	 */
    @JsonProperty("ClusterName")
	public String getClusterName() {
		return clusterName;
	}

	/**
	 * @param clusterName the clusterName to set
	 */
    @JsonProperty("ClusterName")
	public void setClusterName(String clusterName) {
		this.clusterName = clusterName;
	}

	/**
	 * @return the useMulticastDiscovery
	 */
    @JsonProperty("UseMulticastDiscovery")
	public boolean isUseMulticastDiscovery() {
		return useMulticastDiscovery;
	}

	/**
	 * @param useMulticastDiscovery the useMulticastDiscovery to set
	 */
    @JsonProperty("UseMulticastDiscovery")
	public void setUseMulticastDiscovery(String useMulticastDiscovery) {
		this.useMulticastDiscovery = Boolean.parseBoolean(useMulticastDiscovery);
	}

    /**
	 * @return the discoveryPort
	 */
    @JsonProperty("DiscoveryPort")
	public String getDiscoveryPort() {
		return discoveryPort;
	}

	/**
	 * @param discoveryPort the discoveryPort to set
	 */
    @JsonProperty("DiscoveryPort")
	public void setDiscoveryPort(String discoveryPort) {
		this.discoveryPort = discoveryPort;
	}   
    
	/**
	 * @return the DiscoveryTime
	 */
    @JsonProperty("DiscoveryTime")
	public int getDiscoveryTime() {
		return discoveryTime;
	}

	/**
	 * @param DiscoveryTime the DiscoveryTime to set
	 */
    @JsonProperty("DiscoveryTime")
	public void setDiscoveryTime(int discoveryTime) {
		this.discoveryTime = discoveryTime;
	}    

	/**
	 * @return the multicastDiscoveryTTL
	 */
    @JsonProperty("MulticastDiscoveryTTL")
	public int getMulticastDiscoveryTTL() {
		return multicastDiscoveryTTL;
	}

	/**
	 * @param multicastDiscoveryTTL the multicastDiscoveryTTL to set
	 */
    @JsonProperty("MulticastDiscoveryTTL")
	public void setMulticastDiscoveryTTL(int multicastDiscoveryTTL) {
		this.multicastDiscoveryTTL = multicastDiscoveryTTL;
	}

	/**
	 * @return the joinClusterWithMembers
	 */
    @JsonProperty("DiscoveryServerList")
	public String getDiscoveryServerList() {
		return joinClusterWithMembers;
	}

	/**
	 * @param joinClusterWithMembers the joinClusterWithMembers to set
	 */
    @JsonProperty("DiscoveryServerList")
	public void setDiscoveryServerList(String joinClusterWithMembers) {
		this.joinClusterWithMembers = joinClusterWithMembers;
	}

	/**
	 * @return the controlAddress
	 */
    @JsonProperty("ControlAddress")
	public String getControlAddress() {
		return controlAddress;
	}

	/**
	 * @param controlAddress the controlAddress to set
	 */
    @JsonProperty("ControlAddress")
	public void setControlAddress(String controlAddress) {
		this.controlAddress = controlAddress;
	}

	/**
	 * @return the controlPort
	 */
    @JsonProperty("ControlPort")
	public String getControlPort() {
		return controlPort;
	}

	/**
	 * @param controlPort the controlPort to set
	 */
    @JsonProperty("ControlPort")
	public void setControlPort(String controlPort) {
		this.controlPort = controlPort;
	}

	/**
	 * @return the controlExternalAddress
	 */
    @JsonProperty("ControlExternalAddress")
	public String getControlExternalAddress() {
		return controlExternalAddress;
	}

	/**
	 * @param controlExternalAddress the controlExternalAddress to set
	 */
    @JsonProperty("ControlExternalAddress")
	public void setControlExternalAddress(
			String controlExternalAddress) {
		this.controlExternalAddress = controlExternalAddress;
	}

	/**
	 * @return the controlExternalPort
	 */
    @JsonProperty("ControlExternalPort")
	public String getControlExternalPort() {
		return controlExternalPort;
	}

	/**
	 * @param controlExternalPort the controlExternalPort to set
	 */
    @JsonProperty("ControlExternalPort")
	public void setControlExternalPort(String controlExternalPort) {
		this.controlExternalPort = controlExternalPort;
	}

	/**
	 * @return the messagingAddress
	 */
    @JsonProperty("MessagingAddress")
	public String getMessagingAddress() {
		return messagingAddress;
	}

	/**
	 * @param messagingAddress the messagingAddress to set
	 */
    @JsonProperty("MessagingAddress")
	public void setMessagingAddress(String messagingAddress) {
		this.messagingAddress = messagingAddress;
	}

	/**
	 * @return the messagingPort
	 */
    @JsonProperty("MessagingPort")
	public String getMessagingPort() {
		return messagingPort;
	}

	/**
	 * @param messagingPort the messagingPort to set
	 */
    @JsonProperty("MessagingPort")
	public void setMessagingPort(String messagingPort) {
		this.messagingPort = messagingPort;
	}

	/**
	 * @return the messagingExternalAddress
	 */
    @JsonProperty("MessagingExternalAddress")
	public String getMessagingExternalAddress() {
		return messagingExternalAddress;
	}

	/**
	 * @param messagingExternalAddress the messagingExternalAddress to set
	 */
    @JsonProperty("MessagingExternalAddress")
	public void setMessagingExternalAddress(
			String messagingExternalAddress) {
		this.messagingExternalAddress = messagingExternalAddress;
	}

	/**
	 * @return the messagingExternalPort
	 */
    @JsonProperty("MessagingExternalPort")
	public String getMessagingExternalPort() {
		return messagingExternalPort;
	}

	/**
	 * @param messagingExternalPort the messagingExternalPort to set
	 */
    @JsonProperty("MessagingExternalPort")
	public void setMessagingExternalPort(String messagingExternalPort) {
		this.messagingExternalPort = messagingExternalPort;
	}

	/**
	 * @return the messagingUseTLS
	 */
    @JsonProperty("MessagingUseTLS")
	public boolean isMessagingUseTLS() {
		return messagingUseTLS;
	}

	/**
	 * @param messagingUseTLS the messagingUseTLS to set
	 */
    @JsonProperty("MessagingUseTLS")
	public void setMessagingUseTLS(String messagingUseTLS) {
		this.messagingUseTLS = Boolean.parseBoolean(messagingUseTLS);
	}
    
    /**
	 * @return the serverUID
	 */
    @JsonProperty("ServerUID")
	public String getServerUID() {
		return serverUID;
	}

	/**
	 * @param serverUID the serverUID to set
	 */
    @JsonProperty("ServerUID")
	public void setServerUID(String serverUID) {
		this.serverUID = serverUID;
		if (serverUID != null && !serverUID.isEmpty()) {
			String address = getControlExternalAddress();
			String port = getControlExternalPort();
			if (address == null || address.isEmpty()) {
				address = getControlAddress();
			}
			if (port == null || port.isEmpty()) {
				port = getControlPort();
			}
			if (address != null && !address.isEmpty() && port != null && !port.isEmpty()) {
				if (address.indexOf(":") > -1) {
					address = "[" + address + "]";
				}
			}
		}
	}

    @Override
    public String toString() {
        return "ClusterMembership [enableClusterMembership=" + enableClusterMembership
                + ", clusterName=" + clusterName
                + ", useMulticastDiscovery=" + useMulticastDiscovery + ", multicastDiscoveryTTL=" + multicastDiscoveryTTL
                + ", joinClusterWithMembers=" + joinClusterWithMembers + ", controlAddress=" 
                + controlAddress + ", controlPort=" + controlPort 
                + ", controlExternalAddress=" + controlExternalAddress + ", controlExternalPort=" 
                + controlExternalPort + ", messagingAddress=" + messagingAddress
                + ", messagingPort=" + messagingPort + ", messagingExternalAddress=" 
                + messagingExternalAddress + ", messagingExternalPort=" + messagingExternalPort 
                + ", messagingUseTLS=" + messagingUseTLS +"]";
    }

    @Override
    public ValidationResult validate() {
        ValidationResult result = new ValidationResult();
        result.setResult(VALIDATION_RESULT.OK);

        // Validation is now handled by the admin component, so just do some basic sanity checking to prevent
        // malicious requests
        
        if (!checkUtfLength(clusterName, NAME_MAX_LENGTH, "ClusterName", result)) return result;
        if (!checkUtfLength(joinClusterWithMembers, 65535, "DiscoveryServerList", result)) return result;
        if (!checkUtfLength(controlAddress, IP_ADDRESS_MAX_LENGTH, "ControlAddress", result)) return result;
        if (!checkRange(controlPort, PORT_MIN, PORT_MAX, "ControlPort", result)) return result;
        if (!checkUtfLength(controlExternalAddress, FQ_HOSTNAME_MAX_LENGTH, "ControlExternalAddress", result)) return result;
        if (!checkRange(controlExternalPort, PORT_MIN, PORT_MAX, "ControlExternalPort", result)) return result;
        if (!checkUtfLength(messagingAddress, IP_ADDRESS_MAX_LENGTH, "MessagingAddress", result)) return result;
        if (!checkRange(messagingPort, PORT_MIN, PORT_MAX, "MessagingPort", result)) return result;
        if (!checkUtfLength(messagingExternalAddress, FQ_HOSTNAME_MAX_LENGTH, "MessagingExternalAddress", result)) return result;
        if (!checkRange(messagingExternalPort, PORT_MIN, PORT_MAX, "MessagingExternalPort", result)) return result;       
        if (!checkRange(discoveryPort, PORT_MIN, PORT_MAX, "DiscoveryPort", result)) return result;
        if (!checkRange(""+discoveryTime, 0, 2147483647, "DiscoveryTime", result)) return result;
        
        return result;
    }

}
