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

import com.ibm.ima.resources.data.Endpoint;

public class IsmConfigSetEndpointRequest extends IsmConfigRequest {

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
    private String Description;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Delete;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Update;

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String NewName;

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Enabled;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private Integer Port;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Protocol;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Interface;

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Security;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String SecurityProfile;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ConnectionPolicies;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MessagingPolicies;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MessageHub;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MaxMessageSize;


    public IsmConfigSetEndpointRequest(String username) {
        this.Action = "Set";
        this.Component = "Transport";
        this.User = username;

        this.Type = "Composite";
        this.Item = "Endpoint";
    }

    public void setFieldsFromEndpoint(Endpoint endpoint) {
        if (endpoint.getKeyName() != null && !endpoint.getKeyName().equals(endpoint.getName())) {
            this.NewName = endpoint.getName();
            this.Name = endpoint.getKeyName();
        } else {
            this.Name = endpoint.getName();
        }
        this.Description = endpoint.getDescription() != null ? endpoint.getDescription() : "";
        this.Enabled = endpoint.getEnabled();
        this.Port = endpoint.getPort();
        this.Protocol = endpoint.getProtocol();
        this.Interface = endpoint.getIpAddr();
        this.SecurityProfile = endpoint.getSecurityProfile()!= null ? endpoint.getSecurityProfile() : "";
        if (this.SecurityProfile == null || this.SecurityProfile.isEmpty()) {
            this.Security = "false";
        } else {
            this.Security = "true";
        }
        this.ConnectionPolicies = endpoint.getConnectionPolicies() != null ? endpoint.getConnectionPolicies() : "";
        this.MessagingPolicies = endpoint.getMessagingPolicies() != null ? endpoint.getMessagingPolicies() : "";
        this.MessageHub = endpoint.getMessageHub();
        this.MaxMessageSize = endpoint.getMaxMessageSize();
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
        return "IsmConfigSetEndpointRequest [Type=" + Type + ", Item=" + Item
                + ", Name=" + Name + ", Description=" + Description + ", Delete=" + Delete
                + ", Update=" + Update
                + ", NewName=" + NewName + ", Enabled=" + Enabled + ", Port=" + Port + ", Protocol=" + Protocol
                + ", Interface=" + Interface + ", Security=" + Security
                + ", SecurityProfile=" + SecurityProfile
                + ", MessageHub=" + MessageHub
                + ", MaxMessageSize=" + MaxMessageSize
                + ", ConnectionPolicies=" + ConnectionPolicies
                + ", MessagingPolicies=" + MessagingPolicies +"]";
    }
}
