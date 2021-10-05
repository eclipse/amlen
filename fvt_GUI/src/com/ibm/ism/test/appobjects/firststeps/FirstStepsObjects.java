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
package com.ibm.ism.test.appobjects.firststeps;

import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;
/**
 * Provides access to all test objects on the ISM Messaging tab.
 * 
 *
 */
public class FirstStepsObjects {
	
	public static SeleniumTestObject getTab_FirstSteps() {
		return new SeleniumTestObject("//span[text()='First Steps']");
	}
	
	public static SeleniumTextTestObject getTextEntry_NewPassword() {
		return new SeleniumTextTestObject("//input[@id='newPasswordField']");
	}
	
	public static SeleniumTextTestObject getTextEntry_ConfirmPassword() {
		return new SeleniumTextTestObject("//input[@id='confirmPasswordField']");
	}
	
	public static SeleniumTestObject getButton_SavePassword() {
		return new SeleniumTestObject("//span[@id='fs_changePasswordButton']");
	}
	
	public static SeleniumTextTestObject getText_ChangePasswordOutput() {
		return new SeleniumTextTestObject("//div[@id='fs_changePasswordOutputText']");
	}
	
	public static SeleniumTestObject getButton_TestConnection() {
		return new SeleniumTestObject("//span[text()='Test connection']");
	}
	
	//public static SeleniumTestObject getButton_SaveandRun() {
	//	return new SeleniumTestObject("//span[text()='Save and Run']");
	//}
	public static SeleniumTestObject getButton_SaveandRun() {
		return new SeleniumTestObject("//span[@id='fs_demoButton']");
	}
	
	//public static SeleniumTestObject getButton_SaveandClose() {
	//	return new SeleniumTestObject("//span[text()='Save and Close']");
	//}
	public static SeleniumTestObject getButton_SaveandClose() {
		return new SeleniumTestObject("//span[@id='fs_closeButton']");
	}
	
	//public static SeleniumTestObject getButton_Cancel() {
	//	return new SeleniumTestObject("//span[text()='Cancel']");
	//}
	public static SeleniumTestObject getButton_Cancel() {
		return new SeleniumTestObject("//span[@id='fs_cancelButton']");
	}
	
	public static SeleniumTestObject getButton_LeaveFirstSteps() {
		return new SeleniumTestObject("//span[@widgetid='dijit_form_Button_0']");
	}

	public static SeleniumTestObject getButton_CompleteFirstSteps() {
		return new SeleniumTestObject("//span[@widgetid='dijit_form_Button_1']");
	}
	
	
}
