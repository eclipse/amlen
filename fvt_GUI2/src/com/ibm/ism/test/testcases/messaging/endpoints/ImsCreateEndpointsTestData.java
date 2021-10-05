package com.ibm.ism.test.testcases.messaging.endpoints;


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
import java.util.ArrayList;
import java.util.Arrays;

import com.ibm.ism.test.common.IsmTestData;

public class ImsCreateEndpointsTestData extends IsmTestData {

	public static final String KEY_MESSAGE_HUB = "MESSAGE_HUB";
	public static final String KEY_ENDPOINT_NAME = "ENDPOINT_NAME";
	public static final String KEY_ENDPOINT_DESC = "ENDPOINT_DESC";
	public static final String KEY_ENDPOINT_ENABLED = "ENDPOINT_ENABLED";
	public static final String KEY_ENDPOINT_PORT = "ENDPOINT_PORT";
	public static final String KEY_ENDPOINT_IPADDRESS = "ENDPOINT_IPADDRESS";
	public static final String KEY_ENDPOINT_PROTOCOL_JMS = "ENDPOINT_PROTOCOL_JMS";
	public static final String KEY_ENDPOINT_PROTOCOL_MQTT = "ENDPOINT_PROTOCOL_MQTT";
	public static final String KEY_ENDPOINT_MAXMESSAGESIZE = "ENDPOINT_MAXMESSAGESIZE";
	public static final String KEY_ENDPOINT_SECURITY_PROFILE = "ENDPOINT_SECURITY_PROFILE";
	public static final String KEY_ENDPOINT_CONNECTION_POLICY_LIST = "ENDPOINT_CONNECTION_POLICY_LIST";
	public static final String KEY_ENDPOINT_MESSAGING_POLICY_LIST = "ENDPOINT_MESSAGING_POLICY_LIST";
	
	public ImsCreateEndpointsTestData(String[] args) {
		super(args);
	}
	
	public String getEndpointName() {
		return getProperty(KEY_ENDPOINT_NAME);
	}
	
	public String getEndpointDescription() {
		return getProperty(KEY_ENDPOINT_DESC);
	}
	
	public boolean enableEndpoint() {
		boolean enable = true;
		if (getProperty(KEY_ENDPOINT_ENABLED) != null) {
			enable = getProperty(KEY_ENDPOINT_ENABLED).equalsIgnoreCase("true");
		}
		return enable;
	}
	
	public String getEndpointPort() {
		return getProperty(KEY_ENDPOINT_PORT);
	}
	
	public String getEndpointIPAddress() {
		return getProperty(KEY_ENDPOINT_IPADDRESS);
	}
	
	public String getEndpointProtocolJMS() {
		return getProperty(KEY_ENDPOINT_PROTOCOL_JMS);
	}
	
	public String getEndpointProtocolMQTT() {
		return getProperty(KEY_ENDPOINT_PROTOCOL_MQTT);
	}
	
	public String getEndpointMaxMessageSize() {
		return getProperty(KEY_ENDPOINT_MAXMESSAGESIZE);
	}
	
	public String getEndpointSecurityProfile() {
		return getProperty(KEY_ENDPOINT_SECURITY_PROFILE);
	}
	
	public String getMessageHub() {
		return getProperty(KEY_MESSAGE_HUB);
	}
	
	public ArrayList<String> getConnectionPolicyList() {
		String str = getProperty(KEY_ENDPOINT_CONNECTION_POLICY_LIST);
		if (str == null) {
			return null;
		}
		return new ArrayList<String>(Arrays.asList(str.split("\\s*,\\s*")));
	}
	
	public ArrayList<String> getMessagingPolicyList() {
		String str = getProperty(KEY_ENDPOINT_MESSAGING_POLICY_LIST);
		if (str == null) {
			return null;
		}
		return new ArrayList<String>(Arrays.asList(str.split("\\s*,\\s*")));
	}
	
}




