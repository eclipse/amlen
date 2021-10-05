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
import com.fasterxml.jackson.databind.annotation.JsonSerialize.Inclusion;


public class IsmConfigGetProtocolRequest extends IsmConfigRequest {

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
	private String Name;
	
	
    public IsmConfigGetProtocolRequest(String username) {
        this.Action = "Get";
        this.Component = "Protocol";
        this.Item = "Protocol";
        this.Name = null;
        this.User = username;
    }

    public IsmConfigGetProtocolRequest(String name, String username) {
        this.Action = "Get";
        this.Component = "Protocol";
        this.Item = "Protocol";
        this.Name = name;
        this.User = username;
    }

    @Override
    public String toString() {
        return "IsmConfigGetProtocolRequest [Item=" + Item + ", Name=" + Name
                + ", Action=" + Action + ", Component=" + Component + ", User="
                + User + "]";
    }
}
