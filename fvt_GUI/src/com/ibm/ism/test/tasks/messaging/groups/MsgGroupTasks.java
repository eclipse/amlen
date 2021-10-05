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
package com.ibm.ism.test.tasks.messaging.groups;


import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.messaging.users.MessagingUsersandGroupsPage;
import com.ibm.ism.test.appobjects.messaging.users.MsgAddGroupDialogObject;
import com.ibm.ism.test.common.IsmTestData;
import com.ibm.ism.test.testcases.messaging.users.ImsCreateGroupTestData;

public class MsgGroupTasks {

	private static final String HEADER_GROUPS = "Groups";
	private static final String HEADER_MSG_GROUPS = "Messaging Groups";
	
	public static void verifyPageLoaded() throws Exception {
		WebBrowser.waitForReady();
		if  (!WebBrowser.waitForText(HEADER_GROUPS, 10)) {
			throw new Exception(HEADER_GROUPS + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_MSG_GROUPS, 10)) {
			throw new Exception(HEADER_MSG_GROUPS + " not found!!!");
		}
	}
	
	public static void loadURL(IsmTestData testData) throws Exception {
		WebBrowser.loadURL(testData.getURL() + "/messaging.html?nav=userAdministration");
		verifyPageLoaded();
	}
	
	public static void addNewGroup(ImsCreateGroupTestData testData) throws Exception {
		MessagingUsersandGroupsPage.getGridIcon_MsgGroupAdd().click();
		if (!WebBrowser.waitForText(MsgAddGroupDialogObject.DIALOG_DESC, 5)) {
			throw new Exception("'" + MsgAddGroupDialogObject.DIALOG_DESC + "' not found!!!");
		}
		MsgAddGroupDialogObject.getTextEntry_GroupID().setText(testData.getNewGroupID());
		
		if (testData.getNewGroupDescription() != null) {
			MsgAddGroupDialogObject.getTextEntry_GroupDesc().setText(testData.getNewGroupDescription());
		}
		//if (testData.getNewGroupGroupMembership() != null) {
		//MsgAddGroupDialogObject.getButton_GroupMembershipArrow().click();
		//for (String group : testData.getNewGroupGroupMembership()) {
			//if (group.equals(MsgAddGroupDialogObject.GROUP_MEMBERSHIP_testgrp1)) {
				//MsgAddGroupDialogObject.getCheckBox_GroupMembershiptestgrp1().click();
			//} else if (group.equals(MsgAddGroupDialogObject.GROUP_MEMBERSHIP_testgrp2)) {
				//MsgAddGroupDialogObject.getCheckBox_GroupMembershiptestgrp2().click();
			//} else if (group.equals(MsgAddGroupDialogObject.GROUP_MEMBERSHIP_testgrp3)) {
				//MsgAddGroupDialogObject.getCheckBox_GroupMembershiptestgrp3().click();
			//} else {
				//throw new Exception("Test Case error: " + group + " is not supported.");
			//}
		//}
		MsgAddGroupDialogObject.getButton_Save().click();
	}
}
