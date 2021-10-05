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

package com.ibm.ima.admin.request;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize.Inclusion;

import com.ibm.ima.resources.data.ClusterMembership;

public class IsmConfigSetClusterMembershipRequest extends IsmConfigRequest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @JsonSerialize
    private String Type = "Composite";
    @JsonSerialize
    private String Item = "ClusterMembership";
    @JsonSerialize
    private String Name = "cluster";
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Update = "true";

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String EnableClusterMembership;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ClusterName;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String UseMulticastDiscovery;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MulticastDiscoveryTTL;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String DiscoveryServerList;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ControlAddress;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ControlPort;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ControlExternalAddress;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ControlExternalPort;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MessagingAddress;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MessagingPort;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MessagingExternalAddress;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MessagingExternalPort;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MessagingUseTLS;   
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String DiscoveryPort;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String DiscoveryTime;

    
    public IsmConfigSetClusterMembershipRequest(String username) {
        this.Action = "Set";
        this.User = username;
        this.Component = "Cluster";
    }

    public void setFieldsFromClusterMembership(ClusterMembership cm) {
    	this.EnableClusterMembership = cm.isEnableClusterMembership() ? "True" : "False";
    	this.ClusterName = cm.getClusterName();
    	this.UseMulticastDiscovery = cm.isUseMulticastDiscovery() ? "True" : "False";
    	this.DiscoveryPort = cm.getDiscoveryPort();
    	this.DiscoveryTime = cm.getDiscoveryTime() > 0 ? "" + cm.getDiscoveryTime() : null;
    	this.MulticastDiscoveryTTL = cm.getMulticastDiscoveryTTL() > 0 ? "" + cm.getMulticastDiscoveryTTL() : null;
    	this.DiscoveryServerList = cm.getDiscoveryServerList();
    	this.ControlAddress = cm.getControlAddress();
    	this.ControlPort = cm.getControlPort();
    	this.ControlExternalAddress = cm.getControlExternalAddress();
    	this.ControlExternalPort =  cm.getControlExternalPort();
    	this.MessagingAddress = cm.getMessagingAddress();
    	this.MessagingPort = cm.getMessagingPort();
    	this.MessagingExternalAddress = cm.getMessagingExternalAddress();
    	this.MessagingExternalPort =  cm.getMessagingExternalPort();
    	this.MessagingUseTLS = cm.isMessagingUseTLS() ? "True" : "False";
    } 

    @Override
    public String toString() {
        return "IsmConfigSetClusterMembershipRequest [Type=" + Type + ", Item=" + Item
                + ", Component=" + Component + ", Name=" + Name  + ", User=" + User 
                + ", Update=" + Update 
                + ", EnableClusterMembership=" + EnableClusterMembership
                        + ", ClusterName=" + ClusterName
                        + ", UseMulticastDiscovery=" + UseMulticastDiscovery 
                        + ", DiscoveryPort=" + DiscoveryPort
                        + ", DiscoveryTime=" + DiscoveryTime                        
                        + ", MulticastDiscoveryTTL=" + MulticastDiscoveryTTL
                        + ", DiscoveryServerList=" + DiscoveryServerList + ", ControlAddress=" 
                        + ControlAddress + ", ControlPort=" + ControlPort 
                        + ", ControlExternalAddress=" + ControlExternalAddress + ", ControlExternalPort=" 
                        + ControlExternalPort + ", MessagingAddress=" + MessagingAddress
                        + ", MessagingPort=" + MessagingPort + ", MessagingExternalAddress=" 
                        + MessagingExternalAddress + ", MessagingExternalPort=" + MessagingExternalPort 
                        + ", MessagingUseTLS=" + MessagingUseTLS +"]";
    }
}
