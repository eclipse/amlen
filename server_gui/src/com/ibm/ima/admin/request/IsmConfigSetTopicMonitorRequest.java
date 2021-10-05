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

import com.ibm.ima.resources.data.TopicMonitor;

public class IsmConfigSetTopicMonitorRequest extends IsmConfigRequest {

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
    private String ObjectIdField;
    @JsonSerialize
    private String Item;
    @JsonSerialize
    private String TopicString;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Delete;

    public IsmConfigSetTopicMonitorRequest(String username) {
        this.Action = "Set";
        this.Component = "Engine";
        this.User = username;

        this.Type = "Composite";
        this.ObjectIdField = "TopicString";
        this.Item = "TopicMonitor";
    }

    public void setFieldsFromTopicMonitor(TopicMonitor topicMonitor) {
        this.TopicString = topicMonitor.getTopicString();
    }
    public void setTopicString(String topicString) {
    	this.TopicString = topicString;
    }
    public void setDelete() {
        this.Delete = "true";
    }

    @Override
    public String toString() {
        return "IsmConfigSetTopicMonitorRequest [Type=" + Type 
                + ", ObjectIdField=" + ObjectIdField
                + ", Item=" + Item
                + ", TopicString=" + TopicString 
                + ", Delete=" + Delete +"]";
    }
}
