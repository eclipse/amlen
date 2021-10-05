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

import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;

import com.ibm.automation.core.web.SeleniumCore;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;

public class MsgAddMessageHubDialogObject {
	public static final String TITLE_ADD_MESSAGE_HUB = "Add Message Hub";
	public static final String DIALOG_DESC = "Define a message hub to manage appliance connections.";
	

	public static SeleniumTestObject getTitle_AddGroup() {
		return new SeleniumTestObject("//span[@id='ism_widgets_Dialog_6_title']");
	}

	public static SeleniumTextTestObject getTextEntry_MessageHubID() {
		//return new SeleniumTextTestObject("//input[@id='ism_widgets_TextBox_0']");
		return new SeleniumTextTestObject("//input[@id='addmessageHubGridDialog_name']");
	}

	public static SeleniumTextTestObject getTextEntry_MessageHubDesc() {
		//return new SeleniumTextTestObject("//input[@id='ism_widgets_TextArea_0']");
		//return new SeleniumTextTestObject("//textarea[@id='ism_widgets_TextArea_0']");
		return new SeleniumTextTestObject("//textarea[@id='addmessageHubGridDialog_description']");
	}

	public static SeleniumTestObject getButton_Save() {
		return new SeleniumTestObject("//span[@id='addmessageHubGridDialog_saveButton']");
	}

	public static SeleniumTestObject getButton_Cancel() {
		return new SeleniumTestObject("//span[@id='dijit_form_Button_4']");
	}
	
	public static WebElement getErrorMessageTitle() {
		//return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("idx_widget_SingleMessage_0_title"));
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.cssSelector("span[id^='idx_widget_SingleMessage_']"));
	}
	
	public static WebElement getErrorCode() {
		//WebElement e = SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("idx_widget_SingleMessage_0"));
		WebElement e = SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.cssSelector("div[id^='idx_widget_SingleMessage_']"));
		return e.findElement(By.xpath(".//div[1]/span[2]/span[2]"));
	}
	
}
