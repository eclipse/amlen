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
package com.ibm.ism.test.tasks.appliance.users;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.appliance.users.ApplianceUsersPageObjects;
import com.ibm.ism.test.appobjects.appliance.users.WebUIAddUserDialogObjects;
import com.ibm.ism.test.common.IsmGuiTestException;
import com.ibm.ism.test.common.IsmTestData;
import com.ibm.ism.test.testcases.appliance.users.ImsCreateUserTestData;

public class WebUIUserTasks {

	private static final String HEADER_USERS = "Users";
	private static final String HEADER_WEB_UI_USERS = "Web UI Users";
	
	public static void verifyPageLoaded() throws Exception {
		WebBrowser.waitForReady();
		if  (!WebBrowser.waitForText(HEADER_USERS, 10)) {
			throw new IsmGuiTestException(HEADER_USERS + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_WEB_UI_USERS, 10)) {
			throw new IsmGuiTestException(HEADER_WEB_UI_USERS + " not found!!!");
		}
	}
	
	public static void loadURL(IsmTestData testData) throws Exception {
		WebBrowser.loadURL(testData.getURL() + "/appliance.html?nav=users");
		Platform.sleep(3);
		verifyPageLoaded();
	}
	
	public static void addNewUser(ImsCreateUserTestData testData) throws Exception {
		ApplianceUsersPageObjects.getGridIcon_WebUiUserAdd().click();
		if (!WebBrowser.waitForText(WebUIAddUserDialogObjects.DIALOG_DESC, 5)) {
			throw new IsmGuiTestException("'" + WebUIAddUserDialogObjects.DIALOG_DESC + "' not found!!!");
		}
		Platform.sleep(3);
		WebUIAddUserDialogObjects.getTextEntry_UserID().setText(testData.getNewUserID());
		
		// Error test - duplicate ID
		if (testData.isDuplicateIDTest()) {
			Platform.sleep(3);
			if (!WebBrowser.waitForText(WebUIAddUserDialogObjects.TOOLTIP_DUPLICATE_ID, 5)) {
				throw new IsmGuiTestException("'" + WebUIAddUserDialogObjects.TOOLTIP_DUPLICATE_ID + "' not found!!!");
			}
			if (WebUIAddUserDialogObjects.getButton_Save().isEnabled()) {
				throw new IsmGuiTestException("Save button is enabled for duplicate ID test, User ID = " + testData.getNewUserID());
			}
			WebUIAddUserDialogObjects.getButton_Cancel().click();
			return;
		}
		
		WebUIAddUserDialogObjects.getTextEntry_UserPassword().setText(testData.getNewUserPwd());
		WebUIAddUserDialogObjects.getTextEntry_ConfirmPassword().setText(testData.getNewUserConfirmPwd());
		
		// Error test - passwords do not match
		if (testData.isPasswordsDoNotMatchTest()) {
			Platform.sleep(3);
			if (!WebBrowser.waitForText(WebUIAddUserDialogObjects.TOOLTIP_PASSWORD_DO_NOT_MATCH, 5)) {
				throw new IsmGuiTestException("'" + WebUIAddUserDialogObjects.TOOLTIP_PASSWORD_DO_NOT_MATCH + "' not found!!!");
			}
			if (WebUIAddUserDialogObjects.getButton_Save().isEnabled()) {
				throw new IsmGuiTestException("Save button is enabled for Passwords do not match test, User ID = " + testData.getNewUserID());
			}
			WebUIAddUserDialogObjects.getButton_Cancel().click();
			return;			
		}
		
		if (testData.getNewUserDescription() != null) {
			WebUIAddUserDialogObjects.getTextEntry_UserDesc().setText(testData.getNewUserDescription());
		}
		WebUIAddUserDialogObjects.getDownArrow_SelectGroupMembership().waitForExistence(5);
		WebUIAddUserDialogObjects.getDownArrow_SelectGroupMembership().click();
		for (String group : testData.getNewUserGroupMembership()) {
			if (group.equals(WebUIAddUserDialogObjects.GROUP_MEMBERSHIP_SYSTEM_ADMINISTRATORS)) {
				WebUIAddUserDialogObjects.getCheckBox_GroupMembershipSystemAdministrators().waitForExistence(5);
				WebUIAddUserDialogObjects.getCheckBox_GroupMembershipSystemAdministrators().click();
			} else if (group.equals(WebUIAddUserDialogObjects.GROUP_MEMBERSHIP_MESSAGING_ADMINISTRATORS)) {
				WebUIAddUserDialogObjects.getCheckBox_GroupMembershipMessagingAdministrators().waitForExistence(5);
				WebUIAddUserDialogObjects.getCheckBox_GroupMembershipMessagingAdministrators().click();
			} else if (group.equals(WebUIAddUserDialogObjects.GROUP_MEMBERSHIP_USERS)) {
				WebUIAddUserDialogObjects.getCheckBox_GroupMembershipUsers().waitForExistence(5);
				WebUIAddUserDialogObjects.getCheckBox_GroupMembershipUsers().click();
			} else {
				throw new IsmGuiTestException("Test Case error: " + group + " is not supported.");
			}
		}
		WebUIAddUserDialogObjects.getButton_Save().waitForExistence(5);
		WebUIAddUserDialogObjects.getButton_Save().click();
	}
}
