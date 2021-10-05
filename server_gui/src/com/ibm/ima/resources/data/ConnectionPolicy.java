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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.ProtocolResource;
import com.ibm.ima.util.Utils;

/*
 * The REST representation of a Connection Policy
 */
@JsonIgnoreProperties
public class ConnectionPolicy extends AbstractIsmConfigObject implements Comparable<ConnectionPolicy> {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private String name;
    private String description;
    @JsonIgnore
    private final String PolicyType = "Connection";
    private String clientID;
    private String clientAddress;
    private String UserID;
    private String GroupID;
    private String commonNames;
    private String protocol;
    private boolean allowDurable = true;
    private boolean allowPersistentMessages = true;
    @JsonIgnore
    private String serverInstance;

    @JsonIgnore
    public String keyName;

    public ConnectionPolicy() { }

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


    @JsonProperty("ClientID")
    public String getClientID() {
        return clientID;
    }
    @JsonProperty("ClientID")
    public void setClientID(String clientID) {
        this.clientID = clientID;
    }


    @JsonProperty("ClientAddress")
    public String getClientAddress() {
        return clientAddress;
    }
    @JsonProperty("ClientAddress")
    public void setClientAddress(String clientAddress) {
        this.clientAddress = clientAddress;
    }


    @JsonProperty("UserID")
    public String getUserID() {
        return UserID;
    }
    @JsonProperty("UserID")
    public void setUserID(String UserID) {
        this.UserID = UserID;
    }
   
    @JsonProperty("GroupID")
    public String getGroupID() {
        return GroupID;
    }

     @JsonProperty("GroupID")
    public void setGroupID(String GroupID) {
        this.GroupID = GroupID;
    }

    @JsonProperty("CommonNames")
    public String getCommonNames() {
        return commonNames;
    }
    @JsonProperty("CommonNames")
    public void setCommonNames(String commonNames) {
        this.commonNames = commonNames;
    }


    @JsonProperty("Protocol")
    public String getProtocol() {
        return protocol;
    }
    @JsonProperty("Protocol")
    public void setProtocol(String protocol) {
        this.protocol = protocol;
    }


    /**
	 * @return the allowDurable
	 */
    @JsonProperty("AllowDurable")
	public boolean isAllowDurable() {
		return allowDurable;
	}

	/**
	 * @param allowDurable the allowDurable to set
	 */
    @JsonProperty("AllowDurable")
    public void setAllowDurable(String allowDurable) {
		this.allowDurable = Boolean.parseBoolean(allowDurable);
	}

	/**
	 * @return the allowPersistentMessages
	 */
    @JsonProperty("AllowPersistentMessages")    
	public boolean isAllowPersistentMessages() {
		return allowPersistentMessages;
	}

	/**
	 * @param allowPersistentMessages the allowPersistentMessages to set
	 */
    @JsonProperty("AllowPersistentMessages")    
    public void setAllowPersistentMessages(String allowPersistentMessages) {
		this.allowPersistentMessages = Boolean.parseBoolean(allowPersistentMessages);
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
    public int compareTo(ConnectionPolicy l) {
        return this.name.compareTo(l.name);
    }

    @Override
    public String toString() {
        return "ConnectionPolicy [name=" + name
                + ", description=" + description
                + ", policyType=" + PolicyType
                + ", clientID=" + clientID
                + ", clientAddress=" + clientAddress
                + ", UserID=" + UserID
                + ", GroupID=" + GroupID
                + ", commonNames=" + commonNames
                + ", protocol=" + protocol
                + ", keyName=" + keyName 
                + ", allowDurable=" + allowDurable 
                + ", allowPersistentMessages=" + allowPersistentMessages + "]";
    }

    @Override
    public ValidationResult validate() {
        // validate IP addresses if any are set
    	
        ValidationResult result = super.validateName(name, "Name");
        if (!result.isOK()) {
            return result;
        } 
               
        if (!checkUtfLength(description, DEFAULT_MAX_LENGTH)) {
        	result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG);
        	result.setParams(new Object[] {"Description", DEFAULT_MAX_LENGTH});
        	return result;
        }
        
        boolean foundFilter = false;

        if (clientAddress != null && !clientAddress.isEmpty()) {        	
        	foundFilter = true;
        	if (!clientAddress.trim().equals("*")) {
        		String[] addresses = clientAddress.split(",");
        		ArrayList<String> addressesToCheck = new ArrayList<String>();
        		for (String address : addresses) {
        			// does it contain a range?
        			int dashIndex = address.indexOf("-");
        			if (dashIndex > 0) {
        				addressesToCheck.add(address.substring(0, dashIndex));
        				addressesToCheck.add(address.substring(dashIndex+1));                
        			} else {
        				addressesToCheck.add(address);
        			}
        		}
        		Utils.validateIPAddresses(addressesToCheck, true);
        	}
        } 
        
        // validate Protocol
        String allKnownProtocols = null;
        try {
        	if (protocol != null && !protocol.isEmpty()) {
        	    foundFilter = true;
        		// get a map of all known protocols
        		ProtocolResource pr = new ProtocolResource();
                HashMap<String, Protocol> protocolMap  = pr.getProtocolMap(serverInstance);
            	
                // make a nice string to display if there is an error...
            	StringBuilder allProtocolsBuffer = new StringBuilder();
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
        
        // validate at least one filter was provided
        String[] filterValues = {getClientID(), getCommonNames(), getGroupID(), getUserID()};
        String[] filterNames = {"ClientID","CommonName", "GroupID", "UserID" };
        for (int i = 0; i < filterValues.length; i++) {
            String value = filterValues[i];
            String var;
            if (value != null && !value.trim().isEmpty()) {
                foundFilter = true;
                // check length
                if (!checkUtfLength(value, DEFAULT_MAX_LENGTH)) {
                	result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG);
                	result.setParams(new Object[] {filterNames[i], DEFAULT_MAX_LENGTH});
                	return result;
                }
                Pattern pattern = filterNames[i].equals("ClientID") ? INVALID_CP_CLIENTID_FILTER_VARIABLES : INVALID_FILTER_VARIABLES; 
                Matcher m = pattern.matcher(value);
                if (m.find()) {                    
                    var = value.substring(m.start(), m.end());
                    result.setResult(VALIDATION_RESULT.INVALID_VARIABLES);
                    result.setParams(new Object[] {filterNames[i], var});
                    return result;
                }
            }
        }
        if (!foundFilter) {
            result.setResult(VALIDATION_RESULT.UNEXPECTED_ERROR);
            result.setErrorMessageID("CWLNA5129");
        }

        
        return result;
    }
}
