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

import com.ibm.ima.resources.data.QueueManagerConnection;

public class IsmConfigSetQueueManagerConnRequest extends IsmMQConnectivityConfigRequest {

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
    private String QueueManagerName;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ConnectionName;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ChannelName;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Delete;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Update;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String SSLCipherSpec;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Force = "false";

    @SuppressWarnings("unused")
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String NewName;


    public IsmConfigSetQueueManagerConnRequest(String username) {
        this.Action = "Set";
        this.User = username;
        this.Type = "Composite";
        this.Item = "QueueManagerConnection";
    }

    public void setFieldsFromQueueManagerConnection(QueueManagerConnection queueManagerConnection) {
        if (queueManagerConnection.getKeyName() != null && !queueManagerConnection.getKeyName().equals(queueManagerConnection.getName())) {
            this.NewName = queueManagerConnection.getName();
            this.Name = queueManagerConnection.getKeyName();
        } else {
            this.Name = queueManagerConnection.getName();
        }
        this.QueueManagerName = queueManagerConnection.getQueueManagerName();
        this.ConnectionName = queueManagerConnection.getConnectionName();
        this.ChannelName = queueManagerConnection.getChannelName();
        this.SSLCipherSpec = queueManagerConnection.getSSLCipherSpec();
        this.Force = Boolean.toString(queueManagerConnection.getForce());
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
    public void setForce() {
        this.Force = "true";
    }


    @Override
    public String toString() {
        return "IsmConfigSetMQConnPropertiesRequest [Type=" + Type + ", Item=" + Item
                + ", Name=" + Name + ", Delete=" + Delete
                + ", Update=" + Update
                + ", QueueManagerName=" + QueueManagerName + ", ConnectionName=" + ConnectionName
                + ", ChannelName=" + ChannelName + ", SSLCipherSpec=" + SSLCipherSpec
                + ", Force=" + Force + "]";
    }
}
