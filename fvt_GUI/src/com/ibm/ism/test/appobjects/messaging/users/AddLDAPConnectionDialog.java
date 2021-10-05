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
package com.ibm.ism.test.appobjects.messaging.users;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;


public class AddLDAPConnectionDialog {

    public static SeleniumTextTestObject getTextEntry_URL() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_url']");
	}
    
    public static SeleniumTextTestObject getTextEntry_Certificate() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_certBox']");
	}
    
    public static SeleniumTextTestObject getTextEntry_MaxConnections() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_maxConns']");
	}
    
    public static SeleniumTextTestObject getTextEntry_Timeout() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_timeout']");
	}
    
    public static SeleniumTextTestObject getTextEntry_CacheTimeout() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_cacheTimeout']");
	}
    
    public static SeleniumTextTestObject getTextEntry_GroupCacheTimeout() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_groupCacheTimeout']");
	}
    
    public static SeleniumTextTestObject getTextEntry_BaseDN() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_baseDN']");
	}
    
    public static SeleniumTextTestObject getTextEntry_BindDN() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_bindDN']");
	}
    
    public static SeleniumTextTestObject getTextEntry_BindPassword() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_bindPassword']");
	}
    
    public static SeleniumTextTestObject getTextEntry_UserIDMap() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_userIDMap']");
	}
    
    public static SeleniumTextTestObject getTextEntry_UserSuffix() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_userSuffix']");
	}
    
    public static SeleniumTextTestObject getTextEntry_GroupMemberIDMap() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_groupMemberMap']");
	}
    
    public static SeleniumTextTestObject getTextEntry_GroupIDMap() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_groupMap']");
	}
    
    public static SeleniumTextTestObject getTextEntry_GroupSuffix() {
		return new SeleniumTextTestObject("//input[@id='addexldapDialog_groupSuffix']");
	}
    
    public static SeleniumTestObject getCheckBox_EnableCache() {
		return new SeleniumTestObject("//input[@id='addexldapDialog_enableCache_CheckBox']");
	}
    
    public static SeleniumTestObject getCheckBox_IncludeNestedGroups() {
		return new SeleniumTestObject("//input[@id='addexldapDialog_groupSearchCheckBox']");
	}
    
    public static SeleniumTestObject getCheckBox_IgnoreCase() {
		return new SeleniumTestObject("//input[@id='addexldapDialog_ignoreCaseCheckBox']");
	}
    
    public static SeleniumTestObject getButton_Save() {
		return new SeleniumTestObject("//span[@id='addexldapDialog_saveButton']");
	}
    
    public static SeleniumTestObject getButton_TestConnection() {
		return new SeleniumTestObject("//span[@id='addexldapDialog_testButton']");
	}
    
    public static SeleniumTestObject getButton_Cancel() {
		return new SeleniumTestObject("//span[@id='addexldapDialog_closeButton']");
	}
    
    
}

