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
/**
 * 
 */
package com.ibm.ism.test.appobjects;

import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;

/**
 * Provides the test objects on the MessageSight login page and the logout menu.
 * 
 *
 */
public class LoginObjects {
	
	public static final String ID_LOGIN_BUTTON = "login_submitButton";
	public static final String ID_USERID_TEXTBOX = "login_userIdInput";
	public static final String ID_PASSWORD_TEXTBOX = "login_passwordInput";
	public static final String CLASS_USER_MENU = "idxHeaderUserText";
	public static final String ID_USER_LOGOUT_MENUITEM = "userActions_logout_text";
	
	public static final String LOCATOR_LOGIN_BUTTON = "//span[@id='login_submitButton']";
	public static final String LOCATOR_USERID_TEXTBOX = "//input[@id='login_userIdInput']";
	public static final String LOCATOR_PASSWORD_TEXTBOX = "//input[@id='login_passwordInput']";
	public static final String LOCATOR_USER_MENU = "//span[@class='idxHeaderUserText']";
	public static final String LOCATOR_USER_LOGOUT_MENUITEM = "//td[@id='userActions_logout_text']";

	public static SeleniumTextTestObject getTextEntry_UserId() {
		return new SeleniumTextTestObject(LOCATOR_USERID_TEXTBOX);
	}
	
	public static SeleniumTextTestObject getTextEntry_Password() {
		return new SeleniumTextTestObject(LOCATOR_PASSWORD_TEXTBOX);
	}
	public static SeleniumTestObject getButton_Login() {
		return new SeleniumTestObject(LOCATOR_LOGIN_BUTTON);
	}
	
	public static SeleniumTestObject getMenu_User() {
		return new SeleniumTestObject(LOCATOR_USER_MENU);
	}
	
	public static SeleniumTestObject getMenuItem_Logout() {
		return new SeleniumTestObject(LOCATOR_USER_LOGOUT_MENUITEM);
	}
}
