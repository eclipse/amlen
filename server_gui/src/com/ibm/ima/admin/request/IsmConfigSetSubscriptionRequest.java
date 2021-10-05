/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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


public class IsmConfigSetSubscriptionRequest extends IsmConfigRequest {

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
    private String ClientID;
    @JsonSerialize
    private String SubscriptionName;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Delete;

    public IsmConfigSetSubscriptionRequest(String username) {
        this.Action = "Set";
        this.Component = "Engine";
        this.User = username;

        this.Type = "Composite";
        this.Item = "Subscription";
        this.Name = "Subscription";
    }

    public void setClientId(String clientId) {
    	this.ClientID = clientId;
    }
    public void setSubscriptionName(String subscriptionName) {
    	this.SubscriptionName = subscriptionName;
    }
    public void setDelete() {
        this.Delete = "true";
    }

    @Override
    public String toString() {
        return "IsmConfigSetSubscriptionRequest [Type=" + Type 
                + ", Item=" + Item
                + ", ClientID=" + ClientID
                + ", Name=" + Name
                + ", SubscriptionName=" + SubscriptionName 
                + ", Delete=" + Delete +"]";
    }
}
