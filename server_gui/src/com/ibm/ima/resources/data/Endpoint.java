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

import java.util.HashMap;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.ProtocolResource;

/*
 * The REST representation of a server transport Endpoint object
 */
@JsonIgnoreProperties
public class Endpoint extends AbstractIsmConfigObject implements Comparable<Endpoint> {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private static final int PORT_MAX_VALUE = 65535;
    private static final int PORT_MIN_VALUE = 1;
    private static final int MAX_MESSAGE_SIZE_MIN_KB = 1;
    private static final int MAX_MESSAGE_SIZE_MAX_KB = 256 * 1024;

    private String name;
    private String description;
    private String enabled = "True";
    private Integer port;
    private String ipAddr = "all";
    private String protocol = "JMS,MQTT";
    private String security = "True";
    private String securityProfile;
    private String connectionPolicies;
    private String messagingPolicies;
    private String messageHub;
    private String maxMessageSize = "1024KB";

    @JsonIgnore
    private String keyName;
    @JsonIgnore
	private String serverInstance;

    public Endpoint() {
    }

    public Endpoint(String name, String description, String enabled, Integer port, String ipAddr, String protocol, String security, String securityProfile, String connectionPolicies, String messagingPolicies, String messageHub, String maxMessageSize) {
        setName(name);
        setDescription(description);
        setKeyName(name);
        setEnabled(enabled);
        setPort(port);
        setIpAddr(ipAddr);
        setProtocol(protocol);
        setSecurity(security);
        setSecurityProfile(securityProfile);
        setConnectionPolicies(connectionPolicies);
        setMessagingPolicies(messagingPolicies);
        setMessageHub(messageHub);
        setMaxMessageSize(maxMessageSize);
    }

    @JsonProperty("Security")
    public String getSecurity() {
        return security;
    }
    @JsonProperty("Security")
    public void setSecurity(String security) {
        this.security = security;
    }

    @JsonProperty("Name")
    public String getName() {
        return name;
    }
    @JsonProperty("Name")
    public void setName(String name) {
        this.name = name;
    }

    @JsonProperty("Description")
    public String getDescription() {
        return description;
    }
    @JsonProperty("Description")
    public void setDescription(String description) {
        this.description = description;
    }


    @JsonProperty("Enabled")
    public String getEnabled() {
        return enabled;
    }
    @JsonProperty("Enabled")
    public void setEnabled(String enabled) {
        this.enabled = enabled;
    }

    @JsonProperty("Port")
    public Integer getPort() {
        return port;
    }
    @JsonProperty("Port")
    public void setPort(Integer port) {
        this.port = port;
    }


    @JsonProperty("Interface")
    public String getIpAddr() {
        return ipAddr;
    }
    @JsonProperty("Interface")
    public void setIpAddr(String ipAddr) {
        this.ipAddr = ipAddr;
    }


    @JsonProperty("Protocol")
    public String getProtocol() {
        return protocol;
    }
    @JsonProperty("Protocol")
    public void setProtocol(String protocol) {
        this.protocol = protocol;
    }


    @JsonProperty("SecurityProfile")
    public String getSecurityProfile() {
        return securityProfile;
    }
    @JsonProperty("SecurityProfile")
    public void setSecurityProfile(String securityProfile) {
        this.securityProfile = securityProfile;
    }

    @JsonProperty("ConnectionPolicies")
    public String getConnectionPolicies() {
        return connectionPolicies;
    }
    @JsonProperty("ConnectionPolicies")
    public void setConnectionPolicies(String connectionPolicies) {
        this.connectionPolicies = connectionPolicies;
    }


    @JsonProperty("MessagingPolicies")
    public String getMessagingPolicies() {
        return messagingPolicies;
    }
    @JsonProperty("MessagingPolicies")
    public void setMessagingPolicies(String messagingPolicies) {
        this.messagingPolicies = messagingPolicies;
    }


    @JsonProperty("MessageHub")
    public String getMessageHub() {
        return messageHub;
    }
    @JsonProperty("MessageHub")
    public void setMessageHub(String messageHub) {
        this.messageHub = messageHub;
    }


    @JsonProperty("MaxMessageSize")
    public String getMaxMessageSize() {
        return maxMessageSize;
    }
    @JsonProperty("MaxMessageSize")
    public void setMaxMessageSize(String maxMessageSize) {
        this.maxMessageSize = normalizeMaxMessageSize(maxMessageSize);
    }


    @JsonIgnore
    public String getKeyName() {
        return keyName;
    }
    @JsonIgnore
    public void setKeyName(String keyName) {
        this.keyName = keyName;
    }
    
	/**
	 * @return the serverInstance
	 */
    @JsonIgnore
	public String getServerInstance() {
		return serverInstance;
	}

	/**
	 * @param serverInstance the serverInstance to set
	 */
    @JsonIgnore
	public void setServerInstance(String serverInstance) {
		this.serverInstance = serverInstance;
	}

    @Override
    public int compareTo(Endpoint l) {
        return this.getName().compareTo(l.getName());
    }

    @Override
    public ValidationResult validate() {
        ValidationResult result = super.validateName(name, "Name");
        if (!result.isOK()) {
            return result;
        }

        if (!checkUtfLength(description, DEFAULT_MAX_LENGTH)) {
        	result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG);
        	result.setParams(new Object[] {"Description", DEFAULT_MAX_LENGTH});
        	return result;
        }

        // validate port is in range
        if (port < PORT_MIN_VALUE || port > PORT_MAX_VALUE) {
            result.setResult(VALIDATION_RESULT.VALUE_OUT_OF_RANGE);
            result.setParams(new Object[] {"Port", PORT_MIN_VALUE, PORT_MAX_VALUE});
            return result;
        }

        // validate Max Message size is in range
        if (maxMessageSize == null || maxMessageSize.trim().length() == 0) {
            result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
            result.setParams(new Object[] {"MaxMessageSize"});
            return result;
        }

        int multiplier = 1;
        String sSize = maxMessageSize.toUpperCase();
        int indexOfUnit = sSize.indexOf("K");
        if (indexOfUnit < 0) {
            multiplier = 1024;
            indexOfUnit = sSize.indexOf("M");
        }
        if (indexOfUnit < 0) {
            result.setResult(VALIDATION_RESULT.UNIT_INVALID);
            result.setParams(new Object[] {"MaxMessageSize","KB, MB, K, M"});
            return result;
        }
        sSize = maxMessageSize.substring(0, indexOfUnit);
        int value = 0;
        try {
            value = Integer.parseInt(sSize);
        } catch (NumberFormatException e) {
            result.setResult(VALIDATION_RESULT.NOT_AN_INTEGER);
            result.setParams(new Object[] {"MaxMessageSize"});
            return result;
        }

        value = value * multiplier;

        if (value < MAX_MESSAGE_SIZE_MIN_KB || value > MAX_MESSAGE_SIZE_MAX_KB) {
            result.setResult(VALIDATION_RESULT.VALUE_OUT_OF_RANGE);
            result.setParams(new Object[] {"MaxMessageSize", MAX_MESSAGE_SIZE_MIN_KB, MAX_MESSAGE_SIZE_MAX_KB});
            return result;
        }
        
        // validate Protocol
        String allKnownProtocols = null;
        try {
        	if (protocol != null && !protocol.isEmpty()) {
       
        		// get a map of all known protocols
        		ProtocolResource pr = new ProtocolResource();
                HashMap<String, Protocol> protocolMap  = pr.getProtocolMap(serverInstance);
            	
                // make a nice string to display if there is an error...
            	StringBuilder allProtocolsBuffer = new StringBuilder();
            	allProtocolsBuffer.append("All, ");
            	for(Protocol p : protocolMap.values()) {
            		allProtocolsBuffer.append(p.getLabel());
            		allProtocolsBuffer.append(", ");
            	}	
            	allKnownProtocols =  allProtocolsBuffer.length() > 0 ? allProtocolsBuffer.substring(0, allProtocolsBuffer.length() - 2) : "";
            	
            	// protocols passed that should be saved for policy
            	String[] protocols = protocol.split(",");
            	// track any unknown protocols
            	StringBuilder unknownProtocolsBuffer = new StringBuilder();
            	
                for (String pName : protocols) {
                	
                	if (pName.toLowerCase().equals("all")) continue;
                	
                	Protocol pObj = protocolMap.get(pName.toLowerCase());
                	
                	if (pObj == null) {
                		// the protocol is unknown
                		unknownProtocolsBuffer.append(pName + ", ");
                	}
                  
                	// fail validation for any unknown protocols first
                	if (unknownProtocolsBuffer.length() > 0) {
                		String unknownProtocols =  unknownProtocolsBuffer.substring(0, unknownProtocolsBuffer.length() - 2);
                        result.setResult(VALIDATION_RESULT.INVALID_ENUM);
                        result.setParams(new Object[] {unknownProtocols, allKnownProtocols});
                        return result;
                	}
                	
                }
            }
        } catch (Exception e) {
            result.setResult(VALIDATION_RESULT.INVALID_ENUM);
            result.setParams(new Object[] {getProtocol(), allKnownProtocols});
            return result;
        }  

        return result;
    }

    private String normalizeMaxMessageSize(String size) {

        int multiplier = 1;
        String sSize = size.toUpperCase();
        int indexOfUnit = sSize.indexOf("K");
        if (indexOfUnit < 0) {
            multiplier = 1024;
            indexOfUnit = sSize.indexOf("M");
        }
        if (indexOfUnit < 0) {
            return "";
        }
        sSize = size.substring(0, indexOfUnit);
        int value = 0;
        try {
            value = Integer.parseInt(sSize);
        } catch (NumberFormatException e) {
            return "";
        }

        value = value * multiplier;
        return ""+value+"KB";

    }

    @Override
    public String toString() {
        return "Endpoint [name=" + name + ", description=" + description + ", enabled=" + enabled + ", port="
                + port + ", ipAddr=" + ipAddr + ", protocol=" + protocol
                + ", security=" + security + ", securityProfile="
                + securityProfile + ", connectionPolicies=" + connectionPolicies + ", messagingPolicies=" +
                messagingPolicies + ", messageHub=" + messageHub
                + ", maxMessageSize=" + maxMessageSize + ", keyName=" + keyName + "]";
    }
}
