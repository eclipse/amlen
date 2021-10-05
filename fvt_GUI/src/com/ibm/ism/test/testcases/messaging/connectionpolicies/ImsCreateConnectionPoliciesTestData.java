
package com.ibm.ism.test.testcases.messaging.connectionpolicies;


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
import com.ibm.ism.test.common.IsmTestData;

public class ImsCreateConnectionPoliciesTestData extends IsmTestData {

	public static final String KEY_MESSAGE_HUB = "MESSAGE_HUB";
	public static final String KEY_CONNECTION_POLICY_NAME = "CONNECTION_POLICY_NAME";
	public static final String KEY_CONNECTION_POLICY_DESC = "CONNECTION_POLICY_DESC";
	public static final String KEY_USER_ID = "USER_ID";
	public static final String KEY_CLIENT_ADDRESS = "CLIENT_ADDRESS";
	public static final String KEY_COMMON_NAME = "COMMON_NAME";
	public static final String KEY_CLIENT_ID = "CLIENT_ID";
	public static final String KEY_GROUP_ID = "GROUP_ID";
	public static final String KEY_ALLOW_DURABLE = "ALLOW_DURABLE";
	public static final String KEY_ALLOW_PERSISTENT_MESSAGES = "ALLOW_PERSISTENT_MESSAGES";
	
	
	public String getMessageHub() {
		return getProperty(KEY_MESSAGE_HUB);
	}
	
	public ImsCreateConnectionPoliciesTestData(String[] args) {
		super(args);
	}
	
	public String getConnectionPolicyName() {
		return getProperty(KEY_CONNECTION_POLICY_NAME);
	}
	
	public String getConnectionPolicyDescription() {
		return getProperty(KEY_CONNECTION_POLICY_DESC);
	}
	
	public String getUserID() {
		return getProperty(KEY_USER_ID);
	}
	
	public String getClientAddress() {
		return getProperty(KEY_CLIENT_ADDRESS);
	}
	
	public String getCommonName() {
		return getProperty(KEY_COMMON_NAME);
	}
	
	public String getClientID() {
		return getProperty(KEY_CLIENT_ID);
	}
	
	public String getGroupID() {
		return getProperty(KEY_GROUP_ID);
	}
	
	public boolean allowDurable() {
		boolean allow = true;
		if (getProperty(KEY_ALLOW_DURABLE) != null) {
			allow = getProperty(KEY_ALLOW_DURABLE).equalsIgnoreCase("true");
		}
		return allow;
	}
	
	public boolean allowPersistentMessages() {
		boolean allow = true;
		if (getProperty(KEY_ALLOW_PERSISTENT_MESSAGES) != null) {
			allow = getProperty(KEY_ALLOW_PERSISTENT_MESSAGES).equalsIgnoreCase("true");
		}
		return allow;
	}
}


