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
package com.ibm.ism.test.appobjects.messaging.users;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;


public class MsgAddGroupDialogObject {



	public static final String TITLE_ADD_USER = "Add Group";
	public static final String DIALOG_DESC = "Messaging groups can be added to messaging policies to control the messaging resources that a group of users can access.";
	public static final String GROUP_MEMBERSHIP_testgrp1 = "testgrp1";
	public static final String GROUP_MEMBERSHIP_testgrp2 = "testgrp2";
	public static final String GROUP_MEMBERSHIP_testgrp3 = "testgrp3";

	public static SeleniumTestObject getTitle_AddGroup() {
		return new SeleniumTestObject("//span[@id='ism_widgets_Dialog_6_title']");
	}

	public static SeleniumTextTestObject getTextEntry_GroupID() {
		return new SeleniumTextTestObject("//input[@id='ism_widgets_TextBox_33']");
	}

	public static SeleniumTextTestObject getTextEntry_GroupDesc() {
		return new SeleniumTextTestObject("//input[@id='ism_widgets_TextBox_34']");
	}

	
	public static SeleniumTestObject getButton_GroupMembershipArrow() {
		return new SeleniumTestObject("//table[@id='ism_widgets_CheckBoxSelect_3']/tbody/tr/td[2]/div");
	}

	public static SeleniumTestObject getCheckBox_GroupMembershiptestgrp1() {
		return new SeleniumTestObject("//td[@id='idx_form__CheckBoxSelectMenuItem_0_text']");
	}
	
	

	public static SeleniumTestObject getCheckBox_GroupMembershiptestgrp2() {
		return new SeleniumTestObject("//td[@id='idx_form__CheckBoxSelectMenuItem_1_text']");
	}

	public static SeleniumTestObject getCheckBox_GroupMembershiptestgrp3() {
		return new SeleniumTestObject("//td[@id='idx_form__CheckBoxSelectMenuItem_2_text']");
	}

	public static SeleniumTestObject getButton_Save() {
		return new SeleniumTestObject("//span[@id='addmsggroupsDialog_saveButton']");
	}

	public static SeleniumTestObject getButton_Cancel() {
		return new SeleniumTestObject("//span[@id='dijit_form_Button_11']");
	}


}
