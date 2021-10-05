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

public class AddConnectionPoliciesToEndpointDialogObjects {

	public static final String TITLE_ADD_CONNECTION_POLICIES_TO_ENDPOINT = "Add Connection Policies to the Endpoint";
	
	public static SeleniumTextTestObject getTextBox_Filter() {
		return new SeleniumTextTestObject("//*[@id='ism_widgets_TextBox_3']");
	}
	
	public static SeleniumTestObject getCheckBox_FirstRow() {
		return new SeleniumTestObject("//*[@id='addConnectionPoliciesismEndpointListDialog_policiesReferencedObjectGrid']/div[3]/div[2]/div/table/tbody/tr/td[1]/div/div/div/input");
	}
	
	public static SeleniumTestObject getButton_Save() {
		return new SeleniumTestObject("//span[@id='addConnectionPoliciesismEndpointListDialog_saveButton']");
	}
	
	
}
