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
package com.ibm.ism.test.appobjects.firstserver;

import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;

public class WebUIConfigureServerConnectionPageObjects {

	public static final String TITLE_GET_STARTED = "Get started with IBM MessageSight Web UI";
	
	public static SeleniumTestObject getTitle_GetStarted() {
		return new SeleniumTestObject("//span[@id='ism_widgets_ExternalString_0']");
	}

	public static SeleniumTextTestObject getTextEntry_ServerName() {
		return new SeleniumTextTestObject("//input[@id='firstServer_serverName']");
	}

	public static SeleniumTextTestObject getTextEntry_AdminAddress() {
		return new SeleniumTextTestObject("//input[@id='firstServer_serverAdminAddress']");
	}

	public static SeleniumTextTestObject getTextEntry_AdminPort() {
		return new SeleniumTextTestObject("//input[@id='firstServer_serverAdminPort']");
	}

	public static SeleniumTextTestObject getTextEntry_Description() {
		return new SeleniumTextTestObject("//input[@id='firstServer_description']");
	}

	public static SeleniumTestObject getCheckBox_SendCredentials() throws Exception {
		//return new SeleniumTestObject("//td[@id='idx_form__CheckBoxSelectMenuItem_0_text']");
		//return new SeleniumTestObject("//td[text()='SystemAdministrators']");
		return new SeleniumTextTestObject("//input[@id='firstServer_sendWebUICredentials']");
	}

	public static SeleniumTestObject getButton_SaveandManage() {
		return new SeleniumTestObject("//span[@id='firstServer_saveButton_label']");
	}

	public static SeleniumTestObject getButton_TestConnection() {
		return new SeleniumTestObject("//span[@id='firstServer_testButton_label']");
	}
	public static SeleniumTestObject getButton_Close() {
		//return new SeleniumTestObject("//span[@id='closeFirstServerDialog_closeButton']");
		//return new SeleniumTestObject("//span[@id='digit_form_Button_0_label']");
		return new SeleniumTestObject("//span[text()='Close']");
	}
	public static SeleniumTestObject getButton_AddException() {
		return new SeleniumTestObject("//span[text()='Add Exception']");
	}
	public static SeleniumTestObject getButton_ConfirmSecurityException() {
		return new SeleniumTestObject("//span[text()='Confirm Security Exception']");
	}
	public static SeleniumTestObject getButton_LicenseAgreementAccept() {
		return new SeleniumTestObject("//span[@id='li_acceptButton_label']");
	}
	public static SeleniumTestObject getLink_ExpandClickHere() {
		return new SeleniumTestObject("//span[@id='idx_widget_SingleMessage_0_messageTitle']");
	}
//	public static SeleniumTestObject getLink_ExpandClickHere() {
//		return new SeleniumTestObject("//span[@id='idx_widget_SingleMessage_0_messageFakeTitle']");
//	}
	
	public static SeleniumTestObject getLink_ClickHere() {
		return new SeleniumTestObject("//span[text()='Click here']");
	}
	
	public static SeleniumTestObject getMenu_VerifyServer() {
		return new SeleniumTestObject("//span[@id='ismServerMenu_text']");
	}
	
	public static SeleniumTestObject getMenu_VerifyServer2() {
		return new SeleniumTestObject("//div[@id='ismServerMenu']");
	}
	
	public static SeleniumTestObject getMenu_VerifyServer3() {
		//return new SeleniumTestObject("//div[@id='ismServerMenu_recentServers']");
		//return new SeleniumTestObject("//span[text()='LOCALHOST(127.0.0.1:9089)']");
		return new SeleniumTestObject("//span[@id='ismServerMenu_recentServersLabel']");
	}
	
	public static SeleniumTestObject getMenu_VerifyServer4() {
		//return new SeleniumTestObject("//a[@id='ServerControl_viewAllLink']");
		return new SeleniumTestObject("//span[text()='Add']");
	}
	
	public static SeleniumTestObject getMenu_VerifyServer5() {
		return new SeleniumTestObject("//a[@id='ServerControl_recentServers_link']");
	}
	
	
	public static SeleniumTestObject getMenu_VerifyServer6() {
		return new SeleniumTestObject("//div[@id='ismServerMenu_leftContents']");
	}
	
	public static SeleniumTestObject getMenu_VerifyServer7() {
		return new SeleniumTestObject("//div[@id='widget_ismServerMenu_server']");
	}
	
	public static SeleniumTestObject getButton_LicenseAccepted() {
		//return new SeleniumTestObject("//span[@id='digit_form_Button_0_label']");
		return new SeleniumTestObject("//span[text()='Close']");
	}
	
	public static SeleniumTestObject getGridIcon_ServerConnectionAdd() {
		return new SeleniumTestObject("//span[@id='serverconnectionGrid_add']");
	}


}
