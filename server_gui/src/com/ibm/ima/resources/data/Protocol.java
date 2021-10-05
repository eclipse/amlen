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

import com.fasterxml.jackson.annotation.JsonProperty;

public class Protocol extends AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private String name;
    private int capabilities;
    private boolean useTopic = false;
    private boolean useShared = false;
    private boolean useQueue = false;
    private boolean useBrowse = false;
    
    private static final int USETOPIC = 0x01;
    private static final int USESHARED = 0x02;
    private static final int USEQUEUE = 0x04;
    private static final int USEBROWSE = 0x08;
    
    public Protocol() {
    }

    public Protocol(String name, int capabilities ) {
    	this.name = name;
    	setCapabilities(capabilities);
    }

    @JsonProperty("Name")
    public String getName() {
        return name;
    }

    @JsonProperty("Name")
    public void setName(String name) {
        this.name = name;
    }

    @JsonProperty("Topic")
    public boolean isUseTopic() {
		return useTopic;
	}

    @JsonProperty("Shared")
	public boolean isUseShared() {
		return useShared;
	}

    @JsonProperty("Queue")
	public boolean isUseQueue() {
		return useQueue;
	}
    
    @JsonProperty("Browse")
	public boolean isUseBrowse() {
		return useBrowse;
	}
    
	@JsonProperty("Capabilities")
    public int getCapabilities() {
		return capabilities;
	}

	@JsonProperty("Capabilities")
    public void setCapabilities(int capabilities) {
		this.capabilities = capabilities;
		
        if ((USETOPIC & capabilities) == USETOPIC) {
        	this.useTopic = true;
        }
        
        if ((USESHARED & capabilities) == USESHARED) {
        	this.useShared = true;
        }
        
        if ((USEQUEUE & capabilities) == USEQUEUE) {
        	this.useQueue = true;
        }
        
        if ((USEBROWSE & capabilities) == USEBROWSE) {
        	this.useBrowse = true;
        }
    }
	
	/**
	 * Simple method to get appropriate label for MQTT and JMS
	 * 
	 * @return  The protocol name - if jms or mqtt it will be uppercase
	 */
	public String getLabel() {
		return (name.equals("mqtt") || name.equals("jms")) ? name.toUpperCase() : name;
	}
	
    @Override
    public String toString() {
        return "Protocol [name=" + name + ", capabilities=" + capabilities
                + ", useTopic=" + Boolean.valueOf(useTopic) + ", useQueue=" + Boolean.valueOf(useQueue)
                + ", useShared=" + Boolean.valueOf(useShared) + ", useBrowse=" + Boolean.valueOf(useBrowse) +"]";
    }


    @Override
    public ValidationResult validate() {
        ValidationResult result = super.validateName(name, "Name");
        if (!result.isOK()) {
            return result;
        }

        return result;
    }
}
