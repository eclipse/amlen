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


public class IsmConfigProcessTransactionRequest extends IsmConfigRequest {

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
    private String Action;
    @JsonSerialize
    private String XID;

    public IsmConfigProcessTransactionRequest(String username, String action) {
        this.Action = action;
        this.Component = "Engine";
        this.User = username;
        this.Type = "Composite";
        this.Item = "Transaction";
    }

    public void setXID(String xID) {
		XID = xID;
	}

	@Override
    public String toString() {
        return "IsmConfigProcessTransactionRequest [Type=" + Type 
                + ", Item=" + Item
                + ", Action=" + Action
                + ", XID=" + XID +"]";
    }
}
