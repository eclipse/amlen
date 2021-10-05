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
 * The REST representation of a server Queue object
 */
@JsonIgnoreProperties
public class Queue extends AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private String name;
    private String description;
    private int maxMessages = 5000;
    private boolean allowSend = true;
    private boolean concurrentConsumers = true;
    private boolean DiscardMessages = false;

    @JsonIgnore
    private String keyName;

    public Queue() {
    }

    public Queue(String name) {
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

    @JsonProperty("Description")
    public String getDescription() {
        return description;
    }

    @JsonProperty("Description")
    public void setDescription(String description) {
        this.description = description;
    }

    @JsonProperty("MaxMessages")
    public int getMaxMessages() {
        return maxMessages;
    }

    @JsonProperty("MaxMessages")
    public void setMaxMessages(String maxMessages) {
        this.maxMessages = Integer.valueOf(maxMessages);
    }

    @JsonProperty("AllowSend")
    public boolean getAllowSend() {
        return allowSend;
    }

    @JsonProperty("AllowSend")
    public void setAllowSend(String allowSend) {
        this.allowSend = Boolean.valueOf(allowSend);
    }

    @JsonProperty("ConcurrentConsumers")
    public boolean getConcurrentConsumers() {
        return concurrentConsumers;
    }

    @JsonProperty("ConcurrentConsumers")
    public void setConcurrentConsumers(String concurrentConsumers) {
        this.concurrentConsumers = Boolean.valueOf(concurrentConsumers);
    }

    @JsonProperty("DiscardMessages")
    public boolean getDiscardMessages() {
        return DiscardMessages;
    }

    @JsonProperty("DiscardMessages")
    public void setDiscardMessages(boolean DiscardMessages) {
        this.DiscardMessages = DiscardMessages;
    }

    @Override
    public String toString() {
        return "Queue [name=" + name + ", description=" + description
                + ", maxMessages=" + maxMessages + ", allowSend=" + allowSend
                + ", concurrentConsumers=" + concurrentConsumers +"]";
    }

    @JsonIgnore
    public String getKeyName() {
        return keyName;
    }
    @JsonIgnore
    public void setKeyName(String keyName) {
        this.keyName = keyName;
    }

    @Override
    public ValidationResult validate() {
        ValidationResult result = super.validateName(name, "Name");
        if (!result.isOK()) {
            return result;
        }

        if (!checkUtfLength(description, DEFAULT_MAX_LENGTH)) {
        	result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG);
        	result.setParams(new Object[] {"Description", DEFAULT_MAX_LENGTH});
        }
    	return result;
    }


}
