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


public class IsmConfigGetTopicMonitorRequest extends IsmConfigRequest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @JsonSerialize
    private String Item;

    @JsonSerialize(include = Inclusion.NON_NULL)
    private String TopicString;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ObjectIdField;

    public IsmConfigGetTopicMonitorRequest(String username) {
        this.Action = "Get";
        this.Component = "Engine";
        this.Item = "TopicMonitor";
        this.TopicString = null;
        this.User = username;
        this.ObjectIdField = "TopicString";
    }

    public IsmConfigGetTopicMonitorRequest(String topicString, String username) {
        this.Action = "Get";
        this.Component = "Engine";
        this.Item = "TopicMonitor";
        this.TopicString = topicString;
        this.User = username;
    }

    @Override
    public String toString() {
        return "IsmConfigGetTopicMonitorRequest [Item=" + Item + ", TopicString=" + TopicString
                + ", Action=" + Action + ", ObjectIdField=" + ObjectIdField + ", Component=" + Component + ", User="
                + User + "]";
    }
}
