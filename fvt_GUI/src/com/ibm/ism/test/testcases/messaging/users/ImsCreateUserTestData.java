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

public class ImsCreateUserTestData extends IsmTestData {

	public static final String KEY_NEW_USER_ID = "NEW_USER_ID";
	public static final String KEY_NEW_USER_DESC = "NEW_USER_DESC";
	public static final String KEY_NEW_USER_PWD = "NEW_USER_PWD";
	public static final String KEY_NEW_USER_CONFIRM_PWD = "NEW_USER_CONFIRM_PWD";
	public static final String KEY_NEW_USER_GROUPS = "NEW_USER_GROUPS";
	
	public ImsCreateUserTestData(String[] args) {
		super(args);
	}
	
	public String getNewUserID() {
		return getProperty(KEY_NEW_USER_ID);
	}
	
	public String getNewUserDescription() {
		return getProperty(KEY_NEW_USER_DESC);
	}
	
	public String getNewUserPwd() {
		return getProperty(KEY_NEW_USER_PWD);
	}
	
	public String getNewUserConfirmPwd() {
		return getProperty(KEY_NEW_USER_CONFIRM_PWD);
	}
		
	public ArrayList<String> getNewUserGroupMembership() {
		String str = getProperty(KEY_NEW_USER_GROUPS);
		return new ArrayList<String>(Arrays.asList(str.split("\\s*,\\s*")));
	}
}
