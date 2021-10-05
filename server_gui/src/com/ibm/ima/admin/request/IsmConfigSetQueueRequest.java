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

import com.ibm.ima.resources.data.Queue;

public class IsmConfigSetQueueRequest extends IsmConfigRequest {

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
    private int MaxMessages;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String AllowSend;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ConcurrentConsumers;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String DiscardMessages = "false";

    @SuppressWarnings("unused")
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String NewName;

    public IsmConfigSetQueueRequest(String username) {
        this.Action = "Set";
        this.User = username;
        this.Type = "Composite";
        this.Item = "Queue";
        this.Component = "Engine";
    }

    public void setFieldsFromQueue(Queue queue) {
        if (queue.getKeyName() != null && !queue.getKeyName().equals(queue.getName())) {
            this.NewName = queue.getName();
            this.Name = queue.getKeyName();
        } else {
            this.Name = queue.getName();
        }
        this.Description = queue.getDescription() != null ? queue.getDescription() : "";
        this.MaxMessages = queue.getMaxMessages();
        this.AllowSend = queue.getAllowSend() ? "True" : "False";
        this.ConcurrentConsumers = queue.getConcurrentConsumers() ? "True" : "False";
        this.DiscardMessages = Boolean.toString(queue.getDiscardMessages());
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
    public void setDiscardMessages() {
        this.DiscardMessages = "true";
    }
    
    @Override
    public String toString() {
        return "IsmConfigSetQueueRequest [Type=" + Type + ", Item=" + Item
                + ", Name=" + Name + ", Description=" + Description + ", Delete=" + Delete
                + ", Update=" + Update
                + ", MaxDepth=" + MaxMessages + ", AllowPut=" + AllowSend
                + ", AllowGet=" + ConcurrentConsumers
                + ", DiscardMessages=" + DiscardMessages + "]";
    }
}
