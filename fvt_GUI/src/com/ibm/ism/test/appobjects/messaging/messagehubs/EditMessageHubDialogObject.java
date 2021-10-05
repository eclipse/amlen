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
package com.ibm.ism.test.appobjects.messaging.messagehubs;

import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;

public class EditMessageHubDialogObject {
	public static final String TITLE_EDIT_MESSAGE_HUB = "Edit Message Hub Properties";
	
	public static SeleniumTextTestObject getTextEntry_MessageHubName() {
		return new SeleniumTextTestObject("//input[@id='editismMessageHubDetailsDialog_name']");
	}

	public static SeleniumTextTestObject getTextEntry_MessageHubDesc() {
		return new SeleniumTextTestObject("//textarea[@id='editismMessageHubDetailsDialog_description']");
	}



	public static SeleniumTestObject getButton_Save() {
		return new SeleniumTestObject("//span[@id='editismMessageHubDetailsDialog_saveButton']");
	}

	public static SeleniumTestObject getButton_Cancel() {
		return new SeleniumTestObject("//span[@id='editismMessageHubDetailsDialog_dialog_closeButton']");
	}



}
