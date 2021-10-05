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

package com.ibm.ima.admin.request;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize.Inclusion;

import com.ibm.ima.admin.IsmClientType;

@JsonSerialize(include = Inclusion.NON_NULL)
public class IsmMonitoringRequest extends IsmBaseRequest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private String Type;
    private String Option;
    
    // set to "mqconn_monitoring" to use the MQ Connectivity monitoring IsmClientType
    private String MonitoringType = "ima_monitoring";

    // the following params are optional
    private String Number;
    private String Endpoint;

    private String NumPoints;
    private String Duration;
    private String Name;

    private String ClientID;
    private String MessagingPolicy;
    private String TopicString;
    private String ConnectionName;
    private String SubType;
    private String SubName;
    private String ResultCount;
    private String StatType;
    private String RuleType;
    private String Association;
    

    public IsmMonitoringRequest() {
    	super.setClientType(IsmClientType.ISM_ClientType_Monitoring);
    }
    

    @JsonProperty("Type")
    public String getType() {
        return Type;
    }

    @JsonProperty("Type")
    public void setType(String type) {
        this.Type = type;
    }

    @JsonProperty("Option")
    public String getOption() {
        return Option;
    }

    @JsonProperty("Option")
    public void setOption(String option) {
        this.Option = option;
    }
    
	@JsonProperty("MonitoringType")
    public String getMonitoringType() {
        return MonitoringType;
    }

    @JsonProperty("MonitoringType")
    public void setMonitoringType(String monitoringType) {
        this.MonitoringType = monitoringType;
        if (monitoringType != null && monitoringType.equals("mqconn_monitoring")) {
        	super.setClientType(IsmClientType.ISM_ClientType_MQConnectivityMonitoring);
        }
    }

    @JsonProperty("Number")
    public String getNumber() {
        return Number;
    }

    @JsonProperty("Number")
    public void setNumber(String number) {
        this.Number = number;
    }

    @JsonProperty("Endpoint")
    public String getEndpoint() {
        return Endpoint;
    }

    @JsonProperty("Endpoint")
    public void setEndpoint(String endpoint) {
        this.Endpoint = endpoint;
    }

    @JsonProperty("NumPoints")
    public String getNumPoints() {
        return NumPoints;
    }

    @JsonProperty("NumPoints")
    public void setNumPoints(String numPoints) {
        this.NumPoints = numPoints;
    }

    @JsonProperty("Duration")
    public String getDuration() {
        return Duration;
    }

    @JsonProperty("Duration")
    public void setDuration(String duration) {
        this.Duration = duration;
    }

    @JsonProperty("Name")
    public String getName() {
        return Name;
    }

    @JsonProperty("Name")
    public void setName(String name) {
        this.Name = name;
    }

    @JsonProperty("ClientID")
    public String getClientID() {
        return ClientID;
    }

    @JsonProperty("ClientID")
    public void setClientID(String clientId) {
        this.ClientID = clientId;
    }

    @JsonProperty("MessagingPolicy")
    public String getMessagingPolicy() {
        return MessagingPolicy;
    }

    @JsonProperty("MessagingPolicy")
    public void setMessagingPolicy(String messagingPolicy) {
        this.MessagingPolicy = messagingPolicy;
    }

    @JsonProperty("TopicString")
    public String getTopicString() {
        return TopicString;
    }

    @JsonProperty("TopicString")
    public void setTopicString(String topicString) {
        this.TopicString = topicString;
    }

    @JsonProperty("RuleType")
    public String getRuleType() {
        return this.RuleType;
    }

    @JsonProperty("RuleType")
    public void setRuleType(String ruleType) {
        this.RuleType = ruleType;
    }

    @JsonProperty("Association")
    public String getAssociation() {
        return Association;
    }

    @JsonProperty("Association")
    public void setAssociation(String association) {
        this.Association = association;
    }

    @JsonProperty("StatType")
    public String getStatType() {
        return StatType;
    }

    @JsonProperty("StatType")
    public void setStatType(String statType) {
    	// No allowed values are larger than 512 characters.  This query
    	// could only occur if a user modified the Javascript to submit
    	// a malicious monitoring request.
    	if (statType != null && statType.length() > 512) {
    		statType = statType.substring(0, 512);
    	}
        this.StatType = statType;
    }

    @JsonProperty("ResultCount")
    public String getResultCount() {
        return ResultCount;
    }

    @JsonProperty("ResultCount")
    public void setResultCount(String resultCount) {
        this.ResultCount = resultCount;
    }

    @JsonProperty("ConnectionName")
    public String getConnectionName() {
        return ConnectionName;
    }

    @JsonProperty("ConnectionName")
    public void setConnectionName(String connectionName) {
        this.ConnectionName = connectionName;
    }

    @JsonProperty("SubType")
    public String getSubType() {
        return SubType;
    }

    @JsonProperty("SubType")
    public void setSubType(String subType) {
        this.SubType = subType;
    }

    @JsonProperty("SubName")
    public String getSubName() {
        return SubName;
    }

    @JsonProperty("SubName")
    public void setSubName(String subName) {
        this.SubName = subName;
    }

    @Override
    public String toString() {
        return "IsmMonitoringRequest [Action=" + Action 
                + ", Assocation=" + Association
                + ", ConnectionName=" + ConnectionName
                + ", Duration=" + Duration
                + ", Endpoint=" + Endpoint 
                + ", Locale=" + Locale 
                + ", MonitoringType=" + MonitoringType 
                + ", Name=" + Name 
                + ", Number=" + Number 
                + ", NumPoints=" + NumPoints 
                + ", Option=" + Option 
                + ", ResultCount=" + ResultCount
                + ", RuleType=" + RuleType 
                + ", StatType=" + StatType 
                + ", SubName=" + SubName 
                + ", SubType=" + SubType
                + ", TopicString=" + TopicString 
                + ", ClientID=" + ClientID
                + ", MessagingPolicy=" + MessagingPolicy
                + ", Type=" + Type                 
                + ", User=" + User +"]";
    }

}
