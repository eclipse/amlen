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

public class IsmConfigTestQueueManagerConnRequest extends IsmMQConnectivityConfigRequest {

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
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String QueueManagerName;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ConnectionName;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String ChannelName;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String SSLCipherSpec;

    public IsmConfigTestQueueManagerConnRequest(String username) {
        this.Action = "Test";
        this.User = username;
        this.Type = "Composite";
        this.Item = "QueueManagerConnection";
    }

    public void setFieldsFromQueueManagerConnection(QueueManagerConnection queueManagerConnection) {
        this.QueueManagerName = queueManagerConnection.getQueueManagerName();
        this.ConnectionName = queueManagerConnection.getConnectionName();
        this.ChannelName = queueManagerConnection.getChannelName();
        this.SSLCipherSpec = queueManagerConnection.getSSLCipherSpec();
    }

    @Override
    public String toString() {
        return "IsmConfigTestMQConnPropertiesRequest [Type=" + Type + ", Item=" + Item
                + ", QueueManagerName=" + QueueManagerName + ", ConnectionName=" + ConnectionName
                + ", ChannelName=" + ChannelName + ", SSLCipherSpec=" + SSLCipherSpec + "]";
    }
}
