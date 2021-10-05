/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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


public class IsmConfigGetEnableDiskPersistenceRequest extends IsmConfigRequest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @JsonSerialize
    private String Item;


    public IsmConfigGetEnableDiskPersistenceRequest(String userId) {
        this(userId, null);
    }

    public IsmConfigGetEnableDiskPersistenceRequest(String userId, String name) {
        this.Action = "GET";
        this.Component = "Store";
        this.Item = "EnableDiskPersistence";
        this.User = userId;
    }


    @Override
    public String toString() {
        return "IsmConfigGetEnableDiskPersistenceRequest [Item=" + Item + ",  Action=" + Action + ", Component="
                + Component + ", User=" + User + "]";
    }
}
