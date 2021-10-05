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
package com.ibm.ism.test.appobjects.messaging.queues;

import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;

import com.ibm.automation.core.web.SeleniumCore;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;

public class AddQueueDialogObjects {
	public static final String HEADER_ADD_QUEUE = "Add Queue";
	public static final String DIALOG_DESC = "Define a queue for use with your messaging application.";
	

	public static SeleniumTextTestObject getTextEntry_QueueName() {
		return new SeleniumTextTestObject("//input[@id='addqueuesDialog_name']");
	}

	public static SeleniumTextTestObject getTextEntry_QueueDescription() {
		return new SeleniumTextTestObject("//textarea[@id='addqueuesDialog_descriptions']");
	}
	
	public static SeleniumTestObject getCheckBox_QueueAllowSend() {
		return new SeleniumTestObject("//input[@id='addqueuesDialog_allowSendCheckBox']");
	}
	
	public static SeleniumTextTestObject getTextEntry_MaxMessages() {
		return new SeleniumTextTestObject("//input[@id='addqueuesDialog_maxMessages']");
	}
	
	
	public static SeleniumTestObject getCheckBox_QueueConcurrentConsumers() {
		return new SeleniumTestObject("//input[@id='addqueuesDialog_concurrentConsumersCheckBox']");
	}
	

	public static SeleniumTestObject getButton_Save() {
		return new SeleniumTestObject("//span[@id='addqueuesDialog_saveButton']");
	}

	public static SeleniumTestObject getButton_Cancel() {
		return new SeleniumTestObject("//span[@id='addqueuesDialog_closeButton']");
	}

	public static WebElement getErrorMessageTitle() {
		//return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("idx_widget_SingleMessage_0_title"));
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.cssSelector("span[id^='idx_widget_SingleMessage_']"));
	}
	
	public static WebElement getErrorCode() {
		//*[@id="idx_widget_SingleMessage_0"]/div[1]/span[3]/span[2]
		//WebElement e = SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("idx_widget_SingleMessage_0"));
		WebElement e = SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.cssSelector("div[id^='idx_widget_SingleMessage_']"));
		return e.findElement(By.xpath(".//div[1]/span[2]/span[2]"));
	}
	


}
