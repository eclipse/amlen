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

package com.ibm.ima.resources.data;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

/*
 * The REST representation of a server transport TopicMonitor object
 */
@JsonIgnoreProperties
public class MessagingResource extends AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private String subscriptionName;
    private String clientId;
    private int rc = 0;
    private String errorMessage;

    public MessagingResource() {
    }

    @JsonProperty("SubscriptionName")
    public String getSubscriptionName() {
        return subscriptionName;
    }

    @JsonProperty("SubscriptionName")
    public void setSubscriptionName(String subscriptionName) {
        this.subscriptionName = subscriptionName;
    }

    @JsonProperty("ClientID")
    public String getClientId() {
        return clientId;
    }

    @JsonProperty("ClientID")
    public void setClientId(String clientId) {
        this.clientId = clientId;
    }
    
    @JsonProperty("RC")
    public int getRc() {
        return rc;
    }

    @JsonProperty("RC")
    public void setRc(int rc) {
        this.rc = rc;
    }

    @JsonProperty("ErrorMessage")
    public String getErrorMessage() {
        return errorMessage;
    }

    @JsonProperty("ErrorMessage")
    public void setErrorMessage(String errorMessage) {
        this.errorMessage = errorMessage;
    }

    @Override
	public String toString() {
		return "MessagingResource [subscriptionName=" + subscriptionName
		        + ", clientId=" + clientId +  "]";
	}

    @Override
    public ValidationResult validate() {
        ValidationResult vr = new ValidationResult();
        vr.setResult(VALIDATION_RESULT.OK);
        return vr;
    }

}
