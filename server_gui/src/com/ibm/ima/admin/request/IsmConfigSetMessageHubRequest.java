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

import com.ibm.ima.resources.data.MessageHub;

public class IsmConfigSetMessageHubRequest extends IsmConfigRequest {

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

    @SuppressWarnings("unused")
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String NewName;


    public IsmConfigSetMessageHubRequest(String username) {
        this.Action = "Set";
        this.Component = "Transport";
        this.User = username;
        this.Type = "Composite";
        this.Item = "MessageHub";
    }

    public void setFieldsFromMessageHub(MessageHub messageHub) {
        if (messageHub.getKeyName() != null && !messageHub.getKeyName().equals(messageHub.getName())) {
            this.NewName = messageHub.getName();
            this.Name = messageHub.getKeyName();
        } else {
            this.Name = messageHub.getName();
        }
        this.Description = messageHub.getDescription() != null ? messageHub.getDescription() : "";;
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
        return "IsmConfigSetMessageHubRequest [Type=" + Type + ", Item=" + Item
                + ", Name=" + Name + ", Description=" + Description + ", Delete=" + Delete
                + ", Update=" + Update
                + "]";
    }
}
