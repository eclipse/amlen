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
package com.ibm.ism.test.tasks.messaging.users;


import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.messaging.users.MessagingUsersandGroupsPage;
import com.ibm.ism.test.appobjects.messaging.users.MsgAddUserDialogObject;
import com.ibm.ism.test.common.IsmTestData;
import com.ibm.ism.test.testcases.messaging.users.ImsCreateUserTestData;

public class MsgUserTasks {

	private static final String HEADER_USERS = "Users";
	private static final String HEADER_MSG_USERS = "Messaging Users";
	
	public static void verifyPageLoaded() throws Exception {
		WebBrowser.waitForReady();
		if  (!WebBrowser.waitForText(HEADER_USERS, 10)) {
			throw new Exception(HEADER_USERS + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_MSG_USERS, 10)) {
			throw new Exception(HEADER_MSG_USERS + " not found!!!");
		}
	}
	
	public static void loadURL(IsmTestData testData) throws Exception {
		WebBrowser.loadURL(testData.getURL() + "/messaging.html?nav=userAdministration");
		verifyPageLoaded();
	}
	
	public static void addNewUser(ImsCreateUserTestData testData) throws Exception {
		MessagingUsersandGroupsPage.getGridIcon_MsgUserAdd().click();
		if (!WebBrowser.waitForText(MsgAddUserDialogObject.DIALOG_DESC, 5)) {
			throw new Exception("'" + MsgAddUserDialogObject.DIALOG_DESC + "' not found!!!");
		}
		MsgAddUserDialogObject.getTextEntry_UserID().setText(testData.getNewUserID());
		MsgAddUserDialogObject.getTextEntry_UserPassword().setText(testData.getNewUserPwd());
		MsgAddUserDialogObject.getTextEntry_ConfirmPassword().setText(testData.getNewUserConfirmPwd());
		if (testData.getNewUserDescription() != null) {
			MsgAddUserDialogObject.getTextEntry_UserDesc().setText(testData.getNewUserDescription());
		}
		MsgAddUserDialogObject.getButton_GroupMembershipArrow().click();
		for (String group : testData.getNewUserGroupMembership()) {
			if (group.equals(MsgAddUserDialogObject.GROUP_MEMBERSHIP_testgrp1)) {
				MsgAddUserDialogObject.getCheckBox_GroupMembershiptestgrp1().click();
			} else if (group.equals(MsgAddUserDialogObject.GROUP_MEMBERSHIP_testgrp2)) {
				MsgAddUserDialogObject.getCheckBox_GroupMembershiptestgrp2().click();
			} else if (group.equals(MsgAddUserDialogObject.GROUP_MEMBERSHIP_testgrp3)) {
				MsgAddUserDialogObject.getCheckBox_GroupMembershiptestgrp3().click();
			} else {
				throw new Exception("Test Case error: " + group + " is not supported.");
			}
		}
		MsgAddUserDialogObject.getButton_Save().click();
	}
}
