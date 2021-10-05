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

import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize.Inclusion;

import com.ibm.ima.resources.data.MessagingPolicy;

public class IsmConfigSetMessagingPolicyRequest extends IsmConfigRequest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @JsonSerialize
    private String Type;
    @JsonSerialize
    private String Item;
    @JsonSerialize
    private String Name;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Delete;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Update;

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String NewName;

    @JsonSerialize
    private final String PolicyType = "Messaging";

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Description;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Destination;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String DestinationType;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ActionList;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MaxMessages;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MaxMessagesBehavior;    
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ClientID;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ClientAddress;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String UserID;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String GroupID;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String CommonNames;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Protocol;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String DisconnectedClientNotification;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MaxMessageTimeToLive;

    public IsmConfigSetMessagingPolicyRequest(String username) {
        this.Action = "Set";
        this.Component = "Security";
        this.User = username;

        this.Type = "Composite";
        this.Item = "MessagingPolicy";
    }

    public void setFieldsFromMessagingPolicy(MessagingPolicy policy) {
        if (policy.keyName != null && !policy.keyName.equals(policy.getName())) {
            this.NewName = policy.getName();
            this.Name = policy.keyName;
        } else {
            this.Name = policy.getName();
        }

        this.Description = policy.getDescription() != null ? policy.getDescription() : "";
        this.Destination = policy.getDestination();
        this.DestinationType = policy.getDestinationType();
        this.ActionList = policy.getAction();
        this.MaxMessages = policy.getMaxMessages();
        this.MaxMessagesBehavior = policy.getMaxMessagesBehavior();
        this.ClientID = policy.getClientID() != null ? policy.getClientID() : "";
        this.ClientAddress = policy.getClientAddress() != null ? policy.getClientAddress() : "";
        this.UserID = policy.getUserID() != null ? policy.getUserID() : "";
        this.GroupID = policy.getGroupID() != null ? policy.getGroupID() : "";
        this.CommonNames = policy.getCommonNames() != null ? policy.getCommonNames() : "";
        this.Protocol = policy.getProtocol() != null ? policy.getProtocol() : "";
        this.DisconnectedClientNotification = policy.getDisconnectedClientNotification() ? "True" : "False";
        this.MaxMessageTimeToLive = policy.getMaxMessageTimeToLive();
    }

    public void setName(String name) {
        this.Name = name;
    }
    public void setDelete() {
        this.Delete = "true";
    }
    public void setUpdate() {
        this.Update = "true";
    }


    @Override
    public String toString() {
        return "IsmConfigSetMessagingPolicyRequest [Type=" + Type + ", Item=" + Item
                + ", Name=" + Name + ", Description=" + Description + ", Delete=" + Delete
                + ", Update=" + Update + ", PolicyType=" + PolicyType + ", NewName=" + NewName
                + ", Destination=" + Destination + ", DestinationType=" + DestinationType + ", ActionList=" + ActionList
                + "MaxMessages=" + MaxMessages + "MaxMessagesBehavior=" + MaxMessagesBehavior + ", ClientID=" + ClientID
                + ", ClientAddress=" + ClientAddress + ", UserID=" + UserID + ", GroupID=" + GroupID
                + ", Protocol=" + Protocol + ", CommonNames=" + CommonNames 
                + ", DisconnectedClientNotification=" + DisconnectedClientNotification 
                + ", MaxMessageTimeToLive=" + MaxMessageTimeToLive 
                + "]";
    }

}
