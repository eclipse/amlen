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
package com.ibm.ism.test.appobjects.help;

import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;

/**
 * This class provides access to all UI objects on related to Help menu.
 * 
 *
 */
public class HelpMenuObjects {
	public static SeleniumTestObject getMenu_Help() {
		return new SeleniumTestObject("//span[@class='idxHeaderHelpIcon']");
	}
	
	public static SeleniumTestObject getMenuItemAbout() {
		return new SeleniumTestObject("//td[@id='helpMenu_about_text']");
	}
}
