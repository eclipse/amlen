/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package com.ibm.ism.test.testcases.cluster.membership;

import com.ibm.ism.test.common.IsmTestData;

public class IsmClusterMembershipData extends IsmTestData {
	
	public static final String KEY_SERVER_NAME = "SERVER_NAME";
	public static final String KEY_CLUSTER_NAME = "CLUSTER_NAME";
	public static final String KEY_USE_MULTICAST_DISCOVERY = "USE_MULTICAST_DISCOVERY";
	public static final String KEY_MULTICAST_TIME_TO_LIVE = "MULTICAST_TIME_TO_LIVE";
	public static final String KEY_JOIN_CLUSTER_WITH_MEMBERS = "JOIN_CLUSTER_WITH_MEMBERS";
	public static final String KEY_CONTROL_INTERFACE_ADDRESS = "CONTROL_INTERFACE_ADDRESS";
	public static final String KEY_CONTROL_INTERFACE_PORT = "CONTROL_INTERFACE_PORT";
	public static final String KEY_CONTROL_INTERFACE_EXTERNAL_ADDRESS = "CONTROL_INTERFACE_EXTERNAL_ADDRESS";
	public static final String KEY_CONTROL_INTERFACE_EXTERNAL_PORT = "CONTROL_INTERFACE_EXTERNAL_PORT";
	public static final String KEY_MESSAGING_INTERFACE_ADDRESS = "MESSAGING_INTERFACE_ADDRESS";
	public static final String KEY_MESSAGING_INTERFACE_PORT = "MESSAGING_INTERFACE_PORT";
	public static final String KEY_MESSAGING_INTERFACE_EXTERNAL_ADDRESS = "MESSAGING_INTERFACE_EXTERNAL_ADDRESS";
	public static final String KEY_MESSAGING_INTERFACE_EXTERNAL_PORT = "MESSAGING_INTERFACE_EXTERNAL_PORT";
	public static final String KEY_USE_TLS = "USE_TLS";
	
	public IsmClusterMembershipData(String[] args) {
		// Set default values before setting them to user defined values
		super(args);
	}
	
	public String getServerName() {
		return getProperty(KEY_SERVER_NAME);
	}
	
	public String getClusterName() {
		return getProperty(KEY_CLUSTER_NAME);
	}
	
	public String getUseMulticastDiscovery() {
		return getProperty(KEY_USE_MULTICAST_DISCOVERY);
	}
	
	public String getMulticastTimeToLive() {
		return getProperty(KEY_MULTICAST_TIME_TO_LIVE);
	}
	
	public String getJoinClusterWithMembers() {
		return getProperty(KEY_JOIN_CLUSTER_WITH_MEMBERS);
	}
	
	public String getControlInterfaceAddress() {
		return getProperty(KEY_CONTROL_INTERFACE_ADDRESS);
	}
	
	public String getControlInterfacePort() {
		return getProperty(KEY_CONTROL_INTERFACE_PORT);
	}
	
	public String getControlInterfaceExternalAddress() {
		return getProperty(KEY_CONTROL_INTERFACE_EXTERNAL_ADDRESS);
	}
	
	public String getControlInterfaceExternalPort() {
		return getProperty(KEY_CONTROL_INTERFACE_EXTERNAL_PORT);
	}
	
	public String getMessagingInterfaceAddress() {
		return getProperty(KEY_MESSAGING_INTERFACE_ADDRESS);
	}
	
	public String getMessagingInterfacePort() {
		return getProperty(KEY_MESSAGING_INTERFACE_PORT);
	}
	
	public String getMessagingInterfaceExternalAddress() {
		return getProperty(KEY_MESSAGING_INTERFACE_EXTERNAL_ADDRESS);
	}
	
	public String getMessagingInterfaceExternalPort() {
		return getProperty(KEY_MESSAGING_INTERFACE_EXTERNAL_PORT);
	}
	
	public String getUseTLS() {
		return getProperty(KEY_USE_TLS);
	}
}
