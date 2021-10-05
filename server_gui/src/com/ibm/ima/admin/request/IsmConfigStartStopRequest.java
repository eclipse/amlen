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

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize.Inclusion;

public class IsmConfigStartStopRequest extends IsmConfigRequest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @JsonSerialize
    private String Name;
    @JsonSerialize(include = Inclusion.NON_NULL)
	private String Mode = null;

    public IsmConfigStartStopRequest(String action, String processName, String username) {
        this.Action = action;
        this.User = username;
        this.setName(processName);
    }

    /**
     * @return the name
     */
    @JsonProperty("Name")
    public String getName() {
        return Name;
    }

    /**
     * @param name the name to set
     */
    @JsonProperty("Name")
    public void setName(String name) {
        Name = name;
    }
    
    @JsonIgnore
	public void setRestart(boolean b) {
		Mode = "1";
	}

    @JsonIgnore
	public boolean getRestart() {
		return Mode != null && Mode.equals("1");
	}

    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "IsmConfigStartStopRequest [Name=" + Name
                + ", Action=" + Action + ", User="+User + ", restart=" + getRestart() + "]";
    }



}
