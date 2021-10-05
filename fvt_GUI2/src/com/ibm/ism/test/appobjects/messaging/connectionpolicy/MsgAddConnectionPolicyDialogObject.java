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

package com.ibm.ism.test.appobjects.messaging.connectionpolicy;
import com.ibm.automation.core.web.SeleniumCore;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;
import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;

public class MsgAddConnectionPolicyDialogObject {
	public static final String TITLE_ADD_CONNECTION_POLICY = "Add Connection Policy";
	public static final String DIALOG_DESC = "A connection policy authorizes clients to connect to IBM MessageSight endpoints.";
	

	public static SeleniumTestObject getTab_ConnectionPolicy() {
		return new SeleniumTestObject("//span[@id='ismMHTabContainer_tablist_ismMHConnectionPolicies']");
	}
	
	public static SeleniumTextTestObject getTextEntry_ConnectionPolicyID() {
		return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_name']");
	}

	public static SeleniumTextTestObject getTextEntry_ConnectionPolicyDesc() {
		//return new SeleniumTextTestObject("//input[@id='ism_widgets_TextArea_0']");
		return new SeleniumTextTestObject("//textarea[@id='addismConnectionPolicyListDialog_description']");
	}

	public static SeleniumTestObject getCheckBox_clientAddress() {
		return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_checkbox_clientAddress']");
	}

	public static SeleniumTextTestObject getTextEntry_ClientIPAddress() {
		return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_clientAddress']");
	}
	
	//public static SeleniumTestObject getCheckBox_UserID() {
	//	return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_checkbox_UserID']");
	//}
	
	public static WebElement getCheckBox_UserID() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("addismConnectionPolicyListDialog_checkbox_UserID"));
	}
	
	public static SeleniumTextTestObject getTextEntry_UserID() {
		return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_UserID']");
	}
	
	public static SeleniumTestObject getCheckBox_CertificateCommonName() {
		return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_checkbox_commonNames']");
	}
	
	public static SeleniumTextTestObject getTextEntry_CertificateCommonName() {
		return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_commonNames']");
	}
	
	public static SeleniumTestObject getCheckBox_ClientID() {
		return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_checkbox_clientID']");
	}
	
	public static SeleniumTextTestObject getTextEntry_ClientID() {
		return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_clientID']");
	}
	
	public static SeleniumTestObject getCheckBox_GroupID() {
		return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_checkbox_GroupID']");
	}
	
	public static SeleniumTextTestObject getTextEntry_GroupID() {
		return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_GroupID']");
	}
	
	public static SeleniumTestObject getCheckBox_Protocol() {
		return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_checkbox_protocol']");
	}
	
	public static SeleniumTestObject getDownArrow_SelectProtocol() {
		return new SeleniumTestObject("//table[@id='addismConnectionPolicyListDialog_protocol']/tbody/tr/td[2]/div");
	}
	
	public static SeleniumTestObject getCheckBox_JMS() {
		return new SeleniumTextTestObject("//div[@id='addismConnectionPolicyListDialog_protocol_menu']/table/tbody/tr/td/table/tbody/tr");
	}
	
	public static SeleniumTestObject getCheckBox_MQTT() {
		return new SeleniumTextTestObject("//div[@id='addismConnectionPolicyListDialog_protocol_menu']/table/tbody/tr/td/table/tbody/tr[2]");
	}
	
	public static SeleniumTestObject getCheckBox_AllowDurable() {
		return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_AllowDurable']");
	}
	
	public static SeleniumTestObject getCheckBox_AllowPersistentMessages() {
		return new SeleniumTextTestObject("//input[@id='addismConnectionPolicyListDialog_AllowPersistentMessages']");
	}
	
	public static SeleniumTestObject getButton_Save() {
		return new SeleniumTestObject("//span[@id='addismConnectionPolicyListDialog_saveButton']");
	}

	public static SeleniumTestObject getButton_Cancel() {
		return new SeleniumTestObject("//span[@id='dijit_form_Button_4']");
	}



}

