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
package com.ibm.ism.test.appobjects.appliance.users;

import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;

public class WebUIAddUserDialogObjects {

	public static final String TITLE_ADD_USER = "Add User";
	public static final String DIALOG_DESC = "SystemAdministrators can configure the appliance, and MessagingAdministrators can configure messaging.";
	public static final String GROUP_MEMBERSHIP_SYSTEM_ADMINISTRATORS = "SystemAdministrators";
	public static final String GROUP_MEMBERSHIP_MESSAGING_ADMINISTRATORS = "MessagingAdministrators";
	public static final String GROUP_MEMBERSHIP_USERS = "Users";
	public static final String TOOLTIP_DUPLICATE_ID = "A record with that name already exists.";
	public static final String TOOLTIP_PASSWORD_DO_NOT_MATCH = "The passwords do not match.";
	
	public static SeleniumTestObject getTitle_AddUser() {
		return new SeleniumTestObject("//span[@id='ism_widgets_Dialog_1_title']");
	}

	public static SeleniumTextTestObject getTextEntry_UserID() {
		return new SeleniumTextTestObject("//input[@id='addwebuiusersDialog_identifier']");
	}

	public static SeleniumTextTestObject getTextEntry_UserDesc() {
		return new SeleniumTextTestObject("//input[@id='addwebuiusersDialog_description']");
	}

	public static SeleniumTextTestObject getTextEntry_UserPassword() {
		return new SeleniumTextTestObject("//input[@id='addwebuiusersDialog_password']");
	}

	public static SeleniumTextTestObject getTextEntry_ConfirmPassword() {
		return new SeleniumTextTestObject("//input[@id='addwebuiusersDialog_confirmPassword']");
	}

	public static SeleniumTestObject getDownArrow_SelectGroupMembership() {
		//return new SeleniumTestObject("//table[@id='ism_widgets_CheckBoxSelect_1']/tbody/tr/td[2]/div");
		return new SeleniumTestObject("//table[@id='addwebuiusersDialog_memberGroups']/tbody/tr/td[2]/div");
	}

	public static SeleniumTestObject getCheckBox_GroupMembershipSystemAdministrators() throws Exception {
		return new SeleniumTestObject("//td[@id='idx_form__CheckBoxSelectMenuItem_0_text']");
		//return new SeleniumTestObject("//td[text()='SystemAdministrators']");
	}

	public static SeleniumTestObject getCheckBox_GroupMembershipMessagingAdministrators() {
		return new SeleniumTestObject("//td[@id='idx_form__CheckBoxSelectMenuItem_1_text']");
		//return new SeleniumTestObject("//td[text()='MessagingAdministrators']");
	}

	public static SeleniumTestObject getCheckBox_GroupMembershipUsers() {
		return new SeleniumTestObject("//td[@id='idx_form__CheckBoxSelectMenuItem_2_text']");
		//return new SeleniumTestObject("//td[text()='Users']");
	}

	public static SeleniumTestObject getButton_Save() {
		return new SeleniumTestObject("//span[@id='addwebuiusersDialog_saveButton']");
	}

	public static SeleniumTestObject getButton_Cancel() {
		return new SeleniumTestObject("//span[@id='dijit_form_Button_2']");
	}


}
