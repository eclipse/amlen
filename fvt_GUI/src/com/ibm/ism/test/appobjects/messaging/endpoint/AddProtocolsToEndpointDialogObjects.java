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
package com.ibm.ism.test.appobjects.messaging.endpoint;

import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;

public class AddProtocolsToEndpointDialogObjects {

	public static SeleniumTestObject getDialogHeader() {
		return new SeleniumTestObject("//*[@id='addProtocolsismEndpointListDialog_dialog_title']");
	}

	public static SeleniumTestObject getCheckBox_AllProtocols() {
		return new SeleniumTestObject("//*[@id='addProtocolsismEndpointListDialog_checkbox_AllProtocols']");
	}
	
	public static SeleniumTestObject getCheckBox_JMS() {
		return new SeleniumTextTestObject("//input[@id='idx_form_CheckBox_0']");
	}
	
	public static SeleniumTestObject getCheckBox_MQTT() {
		return new SeleniumTextTestObject("//input[@id='idx_form_CheckBox_1']");
	}
	
	public static SeleniumTestObject getButton_Save() {
		return new SeleniumTextTestObject("//*[@id='addProtocolsismEndpointListDialog_saveButton_label']");
	}
	

}
