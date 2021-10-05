/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

import java.io.IOException;

import com.fasterxml.jackson.annotation.JsonAnySetter;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize.Inclusion;

import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.json.java.JSONObject;


public class NetworkInterface {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    private final static String CLAS = AbstractIsmConfigObject.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    
    private final static int OPSTATE_UNKNOWN = -1;
    private final static int OPSTATE_UP = 0;
    private final static int OPSTATE_DOWN = 1;
    
    private int opState = OPSTATE_UNKNOWN;
    private String[] members;
    private String[] ipaddress;
    private String gateway;
    private String ipv6gateway;
    private String adminState = "enabled";   // assume it's true because such a notion doesn't exist in CCI
    
    public NetworkInterface() {
    }
    
    /*
     * Handle any property we don't know about.
     */
    @JsonAnySetter
    public void handleUnknown(String key, Object value) {
        logger.trace(CLAS, "handleUnknown", "Unknown property " + key + " with value " + value + " received");
    }        
    
    /**
     * @return the opState
     */
    public int getOpState() {
        return opState;
    }

    /**
     * @param opState the opState to set
     */
    @JsonProperty("OpState")
    public void setOpState(String opState) {
        this.opState = "UP".equalsIgnoreCase(opState) ? OPSTATE_UP : OPSTATE_DOWN;
    }
    
    @JsonProperty("opState")    
    public void setOpState(int opState) {
        this.opState = opState;
    }

    /**
     * @return the members
     */
    @JsonSerialize(include = Inclusion.NON_NULL)
    public String[] getMembers() {
    	return members;
    }
    
    /**
     * @param members the members to set
     */
    public void setMembers(String[] members) {
    	this.members = members;
    }
    
    /**
     * @return the ipaddress
     */
    @JsonSerialize(include = Inclusion.NON_NULL)
    public String[] getIpaddress() {
        return ipaddress;
    }

    /**
     * @param ipaddress the ipaddress to set
     */
    public void setIpaddress(String[] ipaddress) {
        this.ipaddress = ipaddress;
    }

    /**
     * @return the gateway
     */
    @JsonSerialize(include = Inclusion.NON_NULL)
    public String getGateway() {
        return gateway;
    }

    /**
     * @param gateway the gateway to set
     */
    public void setGateway(String gateway) {
        this.gateway = gateway;
    }

    /**
     * @return the ipv6gateway
     */
    @JsonSerialize(include = Inclusion.NON_NULL)
    public String getIpv6gateway() {
        return ipv6gateway;
    }

    /**
     * @param ipv6gateway the ipv6gateway to set
     */
    public void setIpv6gateway(String ipv6gateway) {
        this.ipv6gateway = ipv6gateway;
    }

    /**
     * @return the adminState
     */
    @JsonSerialize(include = Inclusion.NON_NULL)
    public String getAdminState() {
        return adminState;
    }

    /**
     * @param adminState the adminState to set
     */
    public void setAdminState(String adminState) {
        this.adminState = adminState;
    }

    @Override
    public String toString() {
        return this.asJsonString();
    }

    /**
     * Serialize this object to JSON
     * @return A serialized json object
     */
    public String asJsonString() {
        String jsonString = "";
        try {
            ObjectMapper mapper = new ObjectMapper();
            jsonString = mapper.writeValueAsString(this);
        } catch (Exception e) {
            logger.log(LogLevel.LOG_WARNING, CLAS, "asJsonString", "CWLNA5003", e);
        }
        return jsonString;
    }
    
    /**
     * Build a JSONObject that represents this POJO
     * @return the JSONObject
     */
    public JSONObject asJsonObject() {        
        JSONObject json = null;
        try {
            json = JSONObject.parse(this.asJsonString());
        } catch (IOException e) {
            logger.log(LogLevel.LOG_WARNING, CLAS, "asJsonObject", "CWLNA5003", e);
        }        
        return json;
    }

}
