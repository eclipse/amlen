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

import com.ibm.ima.resources.data.EnableDiskPersistence;

public class IsmConfigSetEnableDiskPersistenceRequest extends IsmConfigRequest {

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
    private boolean CleanStore;
    @JsonSerialize
    private String Value = "False";

    public IsmConfigSetEnableDiskPersistenceRequest(String userId) {
        this(userId, null);
    }

    /**
     * @param certificateProfile
     */
    public IsmConfigSetEnableDiskPersistenceRequest(String userId, EnableDiskPersistence persistence) {
        this.Action = "SET";
        this.Component = "Store";
        this.CleanStore = true;
        this.User = userId;
        this.Type = "Boolean";
        this.Item = "EnableDiskPersistence";
        if (persistence != null && persistence.EnableDiskPersistence) {
            this.Value = "True";
        }
    }

    @Override
    public String toString() {
        return "IsmConfigSetEnableDiskPersistenceRequest [Type=" + Type + ", Item=" + Item
                + ", Value=" + Value + "]";
    }

}
