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
 * Provides access to all test objects on the Ethernet Interfaces dialog.
 * 
 *
 */
public class EthernetInterfacesDialogObjects {

	public static SeleniumTextTestObject getTextEntry_DefaultGateway() {
		return new SeleniumTextTestObject("//input[@id='ethIntDialog_defaultGateway']");
	}
	
	public static SeleniumTestObject getButton_Close() {
		return new SeleniumTestObject("//span[text()='Close']");
	}
	
	public static SeleniumTestObject getButton_Save() {
		return new SeleniumTestObject("//span[text()='Save']");
	}
	
	
}
