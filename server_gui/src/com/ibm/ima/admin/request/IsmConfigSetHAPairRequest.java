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

import com.ibm.ima.resources.HAPairResource;
import com.ibm.ima.resources.data.HAObject;

public class IsmConfigSetHAPairRequest extends IsmConfigRequest {

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
    @JsonSerialize
    private String StartupMode;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Group;
    @JsonSerialize
    private String RemoteDiscoveryNIC;
    @JsonSerialize
    private String LocalReplicationNIC;
    @JsonSerialize
    private String LocalDiscoveryNIC;
    @JsonSerialize
    private long DiscoveryTimeout;
    @JsonSerialize
    private long HeartbeatTimeout;
    @JsonSerialize
    private String Update;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String EnableHA;
    @JsonSerialize
    private String PreferredPrimary;

    public IsmConfigSetHAPairRequest(String username) {
        this.Action = "Set";
        this.User = username;
        this.Name = "haconfig";
        this.Type = "Composite";
        this.Item = "HighAvailability";
        this.Component = "HA";
        setUpdateAllowedInStandby(true);
    }

    public void setFieldsFromHAObject(HAObject haObject) {
        this.StartupMode = haObject.getStartupMode() != null ? haObject.getStartupMode() : HAPairResource.DEFAULT_STARTUP_MODE;
        this.Group = haObject.getGroup()  != null ? haObject.getGroup() : "";
        this.RemoteDiscoveryNIC = haObject.getRemoteDiscoveryNIC() != null ? haObject.getRemoteDiscoveryNIC() : "";
        this.LocalReplicationNIC = haObject.getLocalReplicationNIC() != null ? haObject.getLocalReplicationNIC() : "";
        this.LocalDiscoveryNIC = haObject.getLocalDiscoveryNIC() != null ? haObject.getLocalDiscoveryNIC() : "";
        this.DiscoveryTimeout = haObject.getDiscoveryTimeout() != 0 ? haObject.getDiscoveryTimeout() : HAPairResource.DEFAULT_DISCOVERY_TIMEOUT;
        this.HeartbeatTimeout = haObject.getHeartbeatTimeout() != 0 ? haObject.getHeartbeatTimeout() : HAPairResource.DEFAULT_HEARTBEAT_TIMEOUT;
        this.EnableHA = haObject.isEnabled() ? "True" : "False";
        this.PreferredPrimary = haObject.isPrimary() ? "True" : "False";
    } 

    public void setUpdate() {
        this.Update = "true";    
    }
    
    public void setPrimary() {
        this.PreferredPrimary = "true";    
    }
    
    public void setEnabled() {
        this.EnableHA = "true";    
    }

    @Override
    public String toString() {
        return "IsmConfigSetHAPairRequest [Type=" + Type + ", Item=" + Item
                + ", Component=" + Component + ", Name=" + Name 
                + ", User=" + User + ", StartupMode=" + StartupMode + ", Group=" + Group
                + ", RemoteDiscoveryNIC=" + RemoteDiscoveryNIC + ", LocalReplicationNIC=" + LocalReplicationNIC 
                + ", LocalDiscoveryNIC=" + LocalDiscoveryNIC + ", DiscoveryTimeout=" + DiscoveryTimeout 
                + ", HeartbeatTimeout=" + HeartbeatTimeout + ", EnableHA=" + EnableHA 
                + ", PreferredPrimary=" + PreferredPrimary + ", Update=" + Update 
                + "]";
    }
}
