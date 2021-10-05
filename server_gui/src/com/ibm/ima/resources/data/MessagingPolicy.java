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
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.regex.Matcher;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.ProtocolResource;
import com.ibm.ima.util.Utils;

/*
 * The REST representation of a Messaging Policy
 */
@JsonIgnoreProperties
public class MessagingPolicy extends AbstractIsmConfigObject implements Comparable<MessagingPolicy> {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private static final int MIN_MESSAGE_COUNT_VALUE = 1;
    private static final int MAX_MESSAGE_COUNT_VALUE = 20000000;
    private static final String UNLIMITED = "unlimited";
    private static final int MIN_TTL_VALUE = 1;
    private static final int MAX_TTL_VALUE = 2147483647;  

    /**
     * Valid DestinationTypes
     *
     */
    public enum DestinationTypes {
        Topic (Arrays.asList("Publish", "Subscribe")),
        Subscription (Arrays.asList("Receive", "Control")),
        Queue(Arrays.asList("Send", "Browse", "Receive"));        
        
        private ArrayList<String> actionList;
        private String actionListString;
        
        DestinationTypes(List<String> actionList) {
            this.actionList = new ArrayList<String>(actionList);
            actionListString = this.actionList.toString();
            actionListString = actionListString.substring(1, actionListString.length()-2).trim();
        }
        
        public String getActionListString() {
            return actionListString;
        }

        public boolean isActionAllowed(String action) {
            return actionList.contains(action);
        }
    }
    
    /**
     * Valid MaxMessagesBehaviors available
     *
     */
    public enum MaxMessagesBehaviors {       
        RejectNewMessages,
        DiscardOldMessages;
    }

    /**
     * Valid Protocols
     */
    public enum Protocols {
        JMS,
        MQTT;
    }

    private String name;
    private String description;
    private String destination;
    private String destinationType;
    private String maxMessages = "";
    private String maxMessagesBehavior = "";
    private String action;
    @JsonIgnore
    private final String PolicyType = "Messaging";
    private String clientID;
    private String clientAddress;
    private String UserID;
    private String GroupID;
    private String commonNames;
    private String protocol;
    private String disconnectedClientNotification = "False";
    private String maxMessageTimeToLive = "unlimited";
    private String useCount;  
    private String pendingAction;

    @JsonIgnore
    public String keyName;
    @JsonIgnore
	private String serverInstance;

    public MessagingPolicy() { }

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

    @JsonProperty("Destination")
    public String getDestination() {
        return destination;
    }
    @JsonProperty("Destination")
    public void setDestination(String destination) {
        this.destination = destination;
    }

    @JsonProperty("DestinationType")
    public String getDestinationType() {
        return destinationType;
    }
    @JsonProperty("DestinationType")
    public void setDestinationType(String destinationType) {
        this.destinationType = destinationType;
    }


    @JsonProperty("MaxMessages")
    public String getMaxMessages() {
        if (maxMessages == null) {
            maxMessages = "";
        }
        if (destinationType != null && maxMessages.isEmpty() && !destinationType.toLowerCase().equals("queue")) {
            maxMessages = "5000";
        }

        return maxMessages;
    }
    @JsonProperty("MaxMessages")
    public void setMaxMessages(String maxMessages) {
        this.maxMessages = maxMessages;
    }

    @JsonProperty("MaxMessagesBehavior")
    public String getMaxMessagesBehavior() {
        if (maxMessagesBehavior == null) {
            maxMessagesBehavior = "";
        }
        if (destinationType != null && maxMessagesBehavior.isEmpty() && !destinationType.toLowerCase().equals("queue")) {
            maxMessagesBehavior = MaxMessagesBehaviors.RejectNewMessages.name();
        }

        return maxMessagesBehavior;
    }
    @JsonProperty("MaxMessagesBehavior")
    public void setMaxMessagesBehavior(String maxMessagesBehavior) {
        this.maxMessagesBehavior = maxMessagesBehavior;
    }
    

    @JsonProperty("ActionList")
    public String getAction() {
        return action;
    }
    @JsonProperty("ActionList")
    public void setAction(String action) {
        this.action = action;
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
    
    @JsonProperty("DisconnectedClientNotification")
    public void setDisconnectedClientNotification(String notify) {
        this.disconnectedClientNotification = notify;
    }
    
    @JsonProperty("DisconnectedClientNotification") 
    public boolean getDisconnectedClientNotification() {       
        return Boolean.parseBoolean(this.disconnectedClientNotification);
    }
    
    @JsonProperty("MaxMessageTimeToLive")
    public void setMaxMessageTimeToLive(String ttl) {
        this.maxMessageTimeToLive = ttl;
    }
    
    @JsonProperty("MaxMessageTimeToLive") 
    public String getMaxMessageTimeToLive() {       
        if (maxMessageTimeToLive == null || maxMessageTimeToLive.isEmpty() || maxMessageTimeToLive.equalsIgnoreCase(MessagingPolicy.UNLIMITED)) {
            maxMessageTimeToLive = UNLIMITED;
        }

        return maxMessageTimeToLive;
    }


    /**
     * @return the useCount
     */
    @JsonProperty("UseCount") 
    public String getUseCount() {
        return useCount;
    }

    /**
     * @param useCount the useCount to set
     */
    @JsonProperty("UseCount") 
    public void setUseCount(String useCount) {
            this.useCount = useCount;
    }
    
    /**
     * @return the pendingAction
     */
    @JsonProperty("PendingAction") 
    public String getPendingAction() {
        return pendingAction;
    }

    /**
     * @param pendingAction the pendingAction to set
     */
    @JsonProperty("PendingAction") 
    public void setStatus(String pendingAction) {
        this.pendingAction = pendingAction;
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
    public int compareTo(MessagingPolicy l) {
        return this.name.compareTo(l.name);
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
       
        // validate destinationType
        DestinationTypes type = null;
        try {
            type = DestinationTypes.valueOf(getDestinationType());            
        } catch (Exception e) {
            result.setResult(VALIDATION_RESULT.INVALID_ENUM);
            result.setParams(new Object[] {getDestinationType(), Arrays.toString(DestinationTypes.values())});
            return result;
        }
        
        // validate destination
        if (destination == null || destination.isEmpty()) {
            result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
            result.setParams(new Object[] {"Destination"});
            return result;
        }
        if (!checkUtfLength(destination, DEFAULT_MAX_LENGTH)) {
        	result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG);
        	result.setParams(new Object[] {"Destination", NAME_MAX_LENGTH});
        	return result;
        }
       
        
        
        // validate actionList
        String actionList = getAction();
        if (actionList == null || actionList.isEmpty()) {
            result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
            result.setParams(new Object[] {"ActionList"});
            return result;            
        }
        String[] actions = actionList.split(",");
        for (String action : actions) {
            if (!type.isActionAllowed(action)) {
                result.setResult(VALIDATION_RESULT.INVALID_ENUM);
                result.setParams(new Object[] {action, type.getActionListString()});
            }
        }
        
        // validate DisconnectedClientNotification
        if (type == DestinationTypes.Topic) {
            if (disconnectedClientNotification != null && !disconnectedClientNotification.isEmpty()) {
                String dcn = disconnectedClientNotification.toLowerCase();
                if (!dcn.equals("true") && !dcn.equals("false")) {
                    result.setResult(VALIDATION_RESULT.INVALID_ENUM);
                    result.setParams(new Object[] {disconnectedClientNotification, "True, False"});
                    return result;
                }
            }
        } else if (disconnectedClientNotification != null && !disconnectedClientNotification.isEmpty() 
                && !disconnectedClientNotification.toLowerCase().equals("false")) { // allow false since that's what get serialized out on a get
            result.setResult(VALIDATION_RESULT.MUST_BE_EMPTY);
            result.setParams(new Object[] {"DisconnectedClientNotification"});
            return result;
        }
        
        // validate ClientID is allowed
        if (type == DestinationTypes.Subscription && clientID != null && !clientID.isEmpty()) {
            result.setResult(VALIDATION_RESULT.MUST_BE_EMPTY);
            result.setParams(new Object[] {"ClientID"});
            return result;
        }
        
        boolean foundFilter = false;

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
            	// track any incompatible protocols
            	StringBuilder incompatableProtocolsBuffer = new StringBuilder();
            	
                for (String pName : protocols) {
                	Protocol pObj = protocolMap.get(pName.toLowerCase());
                
                	// check for protocols that are incompatible with destination type
                	if (pObj != null) {
                		
                	    switch (type) {
                	    	case Queue:
                	    		if (!pObj.isUseQueue()) incompatableProtocolsBuffer.append(pObj.getLabel() + ", ");
                	    		break;
                	    	case Topic:
                	    		if (!pObj.isUseTopic()) incompatableProtocolsBuffer.append(pObj.getLabel() + ", ");
                	    		break;
                	    	case Subscription:
                	    		if (!pObj.isUseShared()) incompatableProtocolsBuffer.append(pObj.getLabel()+ ", ");
                	    		break;
                	    	default:
                	    		// should not get here...
                	    		break;
                	    	}
                			
                	} else {
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
                	
                	if (incompatableProtocolsBuffer.length() > 0) {
                		String incompatableProtocols =  incompatableProtocolsBuffer.substring(0, incompatableProtocolsBuffer.length() - 2);
                	     result.setResult(VALIDATION_RESULT.INCOMPATIBLE_PROTOCOL);
                         result.setParams(new Object[] {incompatableProtocols, getDestinationType()});
                         return result;
                	}
                }
            }
        } catch (Exception e) {
            result.setResult(VALIDATION_RESULT.INVALID_ENUM);
            result.setParams(new Object[] {getProtocol(), allKnownProtocols});
            return result;
        }        

        // validate Max Message count is in range except for queues
        if (type != DestinationTypes.Queue) {
            String maxMessages = getMaxMessages();
            if (maxMessages == null || maxMessages.trim().length() == 0) {
                result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
                result.setParams(new Object[] {"MaxMessages"});
                return result;
            }

            int value = 0;
            try {
                value = Integer.parseInt(maxMessages);
            } catch (NumberFormatException e) {
                result.setResult(VALIDATION_RESULT.NOT_AN_INTEGER);
                result.setParams(new Object[] {"MaxMessages"});
                return result;
            }

            if (value < MIN_MESSAGE_COUNT_VALUE || value > MAX_MESSAGE_COUNT_VALUE) {
                result.setResult(VALIDATION_RESULT.VALUE_OUT_OF_RANGE);
                result.setParams(new Object[] {"MaxMessages", MIN_MESSAGE_COUNT_VALUE, MAX_MESSAGE_COUNT_VALUE});
                return result;
            }
            // validate MaxMessagesBehavior
            try {
                MaxMessagesBehaviors.valueOf(getMaxMessagesBehavior());
            } catch (Exception e) {
                result.setResult(VALIDATION_RESULT.INVALID_ENUM);
                result.setParams(new Object[] {getMaxMessagesBehavior(), Arrays.toString(MaxMessagesBehaviors.values())});
                return result;
            }

        } else {
            if (maxMessages != null && !maxMessages.isEmpty()) {
                result.setResult(VALIDATION_RESULT.MUST_BE_EMPTY);
                result.setParams(new Object[] {"MaxMessages"});
                return result;
            }
            if (maxMessagesBehavior != null && !maxMessagesBehavior.isEmpty()) {
                result.setResult(VALIDATION_RESULT.MUST_BE_EMPTY);
                result.setParams(new Object[] {"MaxMessagesBehavior"});
                return result;
            }                
        }
        
        // validate MaxMessageTimeToLive is not set for Subscriptions and is either "unlimited" or "1-2147483647" for other types
        String ttl = getMaxMessageTimeToLive();
        if (ttl == null || ttl.isEmpty()){
            setMaxMessageTimeToLive(UNLIMITED);
        } else if (!ttl.equalsIgnoreCase(UNLIMITED)) {        
            // must be 1-2147483647
            try {
                Long _ttl = Long.parseLong(ttl);
                if (_ttl < MIN_TTL_VALUE || _ttl > MAX_TTL_VALUE) {
                    result.setResult(VALIDATION_RESULT.VALUE_OUT_OF_RANGE);
                    result.setParams(new Object[] {"MaxMessageTimeToLive", MIN_TTL_VALUE, MAX_TTL_VALUE});
                    return result;
                }
            } catch (NumberFormatException nfe) {
                result.setResult(VALIDATION_RESULT.NOT_AN_INTEGER);
                result.setParams(new Object[] {"MaxMessageTimeToLive"});
                return result;
            }
        }
                
        // validate IP addresses if any are set
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
        
        // validate at least one filter was provided
        String[] filterValues = {getClientID(), getCommonNames(), getGroupID(), getUserID()};
        String[] filterNames = {"ClientID","CommonName", "GroupID", "UserID" };
        for (int i = 0; i < filterValues.length; i++) {
            String value = filterValues[i];
            // check length
            if (!checkUtfLength(value, DEFAULT_MAX_LENGTH)) {
            	result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG);
            	result.setParams(new Object[] {filterNames[i], NAME_MAX_LENGTH});
            	return result;
            }

            String var;
            if (value != null && !value.trim().isEmpty()) {
                foundFilter = true;
                Matcher m = INVALID_FILTER_VARIABLES.matcher(value);
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
    



    @Override
    public String toString() {
        return "MessagingPolicy [name=" + name
                + ", description=" + description
                + ", destination=" + destination
                + ", maxMessages=" + getMaxMessages()
                + ", maxMessagesBehavior=" + getMaxMessagesBehavior()                
                + ", action=" + action
                + ", policyType=" + PolicyType
                + ", clientID=" + clientID
                + ", clientAddress=" + clientAddress
                + ", UserID=" + UserID
                + ", GroupID=" + GroupID
                + ", commonNames=" + commonNames
                + ", protocol=" + protocol
                + ", keyName=" + keyName
                + ", NotifyOnArrrival=" + disconnectedClientNotification
                + ", maxMessageTimeToLive=" + getMaxMessageTimeToLive()
                + ", useCount=" + getUseCount() 
                + ", pendingAction=" + getPendingAction()
                + "]";
    }
}
