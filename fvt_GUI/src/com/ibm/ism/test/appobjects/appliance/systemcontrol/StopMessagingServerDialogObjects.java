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
package com.ibm.ism.test.appobjects.appliance.systemcontrol;

import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;

public class StopMessagingServerDialogObjects {
	
	public static final String DIALOG_TITLE = "Stop the IBM MessageSight Sever";
	public static final String DIALOG_DESC = "Are you sure that you want to stop the IBM MessageSight server?  Stopping the server prevents all messaging.";
	
	public static SeleniumTestObject getTitle_StopMessagingServer() {
		return new SeleniumTestObject("//span[@id='messagingServerControl_stopDialog_title']");
	}
	
	public static SeleniumTestObject getButton_Stop() {
		return new SeleniumTestObject("//span[@id='messagingServerControl_stopDialog_button_ok']");
	}
	
	public static SeleniumTestObject getButton_Cancel() {
		return new SeleniumTestObject("//span[@id='dijit_form_Button_4']");
	}
	
}
