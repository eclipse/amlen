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
package com.ibm.ism.test.testcases.messaging.users;



import java.util.ArrayList;
import java.util.Arrays;

import com.ibm.ism.test.common.IsmTestData;

public class ImsCreateGroupTestData extends IsmTestData {

	public static final String KEY_NEW_GROUP_ID = "NEW_GROUP_ID";
	public static final String KEY_NEW_GROUP_DESC = "NEW_GROUP_DESC";
	public static final String KEY_NEW_GROUP_GROUPS = "NEW_GROUP_GROUPS";
	
	public ImsCreateGroupTestData(String[] args) {
		super(args);
	}
	
	public String getNewGroupID() {
		return getProperty(KEY_NEW_GROUP_ID);
	}
	
	public String getNewGroupDescription() {
		return getProperty(KEY_NEW_GROUP_DESC);
	}
	
		
	public ArrayList<String> getNewGroupGroupMembership() {
		String str = getProperty(KEY_NEW_GROUP_GROUPS);
		return new ArrayList<String>(Arrays.asList(str.split("\\s*,\\s*")));
	}
}
