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

package com.ibm.ima.resources.data;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

/*
 * The REST representation of a server QueueManagerConnection object
 */
@JsonIgnoreProperties
public class QueueManagerConnection extends AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private String name;
    private String queueManagerName;
    private String connectionName;
    private String channelName;
    private String SSLCipherSpec;
    private boolean Force;

    @JsonIgnore
    private String keyName;
	private boolean nameRequired = true;

    public QueueManagerConnection() {
    }

    public QueueManagerConnection(String name) {
        setName(name);
    }

    @JsonProperty("Name")
    public String getName() {
        return name;
    }

    @JsonProperty("Name")
    public void setName(String name) {
        this.name = name;
    }

    @JsonProperty("QueueManagerName")
    public String getQueueManagerName() {
        return queueManagerName;
    }

    @JsonProperty("QueueManagerName")
    public void setQueueManagerName(String queueManagerName) {
        this.queueManagerName = queueManagerName;
    }

    @JsonProperty("ConnectionName")
    public String getConnectionName() {
        return connectionName;
    }

    @JsonProperty("ConnectionName")
    public void setConnectionName(String connectionName) {
        this.connectionName = connectionName;
    }

    @JsonProperty("ChannelName")
    public String getChannelName() {
        return channelName;
    }

    @JsonProperty("ChannelName")
    public void setChannelName(String channelName) {
        this.channelName = channelName;
    }

    @JsonProperty("SSLCipherSpec")
    public String getSSLCipherSpec() {
        return SSLCipherSpec;
    }

    @JsonProperty("SSLCipherSpec")
    public void setSSLCipherSpec(String SSLCipherSpec) {
        this.SSLCipherSpec = SSLCipherSpec;
    }

    @JsonProperty("Force")
    public boolean getForce() {
        return Force;
    }

    @JsonProperty("Force")
    public void setForce(String enabledString) {
        this.Force = Boolean.parseBoolean(enabledString);
    }

    @JsonIgnore
    public String getKeyName() {
        return keyName;
    }
    @JsonIgnore
    public void setKeyName(String keyName) {
        this.keyName = keyName;
    }

    /**
     * By default, name is required. For test connection purposes, it's not...
     * @param b
     */
    @JsonIgnore
	public void setNameRequired(boolean b) {
		this.nameRequired = b;
	}


    @Override
    public ValidationResult validate() {
    	if (nameRequired) {
    		return super.validateName(name, "Name");
    	}
    	return new ValidationResult(VALIDATION_RESULT.OK, null);
    }

    @Override
    public String toString() {
        return "QueueManagerConnection [name=" + name 
                + ", queueManagerName=" + queueManagerName + ", connectionName=" + connectionName
                + ", channelName=" + channelName + ", SSLCipherSpec=" + SSLCipherSpec
                + ", Force="+ Force + "]";
    }


}
