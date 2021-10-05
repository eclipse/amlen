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
 * Provides access to all test objects on the Server Locale dialog.
 * 
 *
 */
public class ServerLocaleDialogObjects {

	public static SeleniumTextTestObject getTextEntry_ServerLocale() {
		return new SeleniumTextTestObject("//input[@id='localeCombo']");
	}
	
	public static SeleniumTextTestObject getTextEntry_ServerTimeZone() {
		return new SeleniumTextTestObject("//input[@id='timeZoneCombo']");
	}
	
	public static SeleniumTextTestObject getTextEntry_Date() {
		return new SeleniumTextTestObject("//input[@id='dateCombo']");
	}
	
	public static SeleniumTextTestObject getTextEntry_Time() {
		return new SeleniumTextTestObject("//input[@id='timeCombo']");
	}
	public static SeleniumTestObject getButton_Close() {
		return new SeleniumTestObject("//span[text()='Close']");
	}
	
	public static SeleniumTestObject getButton_Save() {
		return new SeleniumTestObject("//span[text()='Save']");
	}
	
	
}
