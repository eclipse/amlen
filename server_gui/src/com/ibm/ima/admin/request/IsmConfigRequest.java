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
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize.Inclusion;

import com.ibm.ima.admin.IsmClientType;

public abstract class IsmConfigRequest extends IsmBaseRequest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @JsonSerialize(include = Inclusion.NON_NULL)
    protected String Component;
	@JsonIgnore
    protected String maskTarget = null; // sensitive data mask
    @JsonIgnore
    protected String maskReplacement = null; // sensitive data mask    
    
    @JsonIgnore
    protected boolean isUpdateAllowedInStandby = false;
    
    public IsmConfigRequest() {
    	super.setClientType(IsmClientType.ISM_ClientType_Configuration);
    }

    /**
     * Set a mask to use to sanitize a string representation of the json request
     * sent to the IsmServer
     * 
     * @param target
     *            the string to match
     * @param replacement
     *            the string to replace it with
     */
    @JsonIgnore
    public void setMask(String target, String replacement) {
        this.maskTarget = target;
        this.maskReplacement = replacement;
    }

    /**
     * Apply the mask that is set to the string representation of the json request. 
     * This is simple string replaceAll. For anything more complicated, override it.
     * 
     * @param source
     * @return sanitized source
     */
    @JsonIgnore
    public String applyMask(String source) {
        if (hasMask() && source != null) {
            return source.replaceAll(maskTarget, maskReplacement);
       }
        return source;
    }

    /**
     * Determines if a mask is set
     * 
     * @return true if both maskTarget and maskReplacement are not null
     */
    @JsonIgnore
    public boolean hasMask() {
        return maskTarget != null && maskReplacement != null;
    }

    @JsonIgnore
    public boolean isUpdateAllowedInStandby() {
        return isUpdateAllowedInStandby;
    }

    @JsonIgnore
    public void setUpdateAllowedInStandby(boolean isUpdateAllowedInStandby) {
        this.isUpdateAllowedInStandby = isUpdateAllowedInStandby;
    }

}
