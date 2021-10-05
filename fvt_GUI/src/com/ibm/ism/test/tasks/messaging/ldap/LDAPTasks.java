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
package com.ibm.ism.test.tasks.messaging.ldap;


import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.messaging.users.AddLDAPConnectionDialog;
import com.ibm.ism.test.appobjects.messaging.users.LDAPSettingsChangedDialog;
import com.ibm.ism.test.appobjects.messaging.users.MessagingUsersandGroupsPage;
import com.ibm.ism.test.common.IsmTestData;
import com.ibm.ism.test.testcases.ldap.ImsLDAPTestData;

public class LDAPTasks {

	private static final String HEADER = "External LDAP";
	//private static final String HEADER_MSG = "If external LDAP is configured, users and groups cannot be created, edited, or viewed on this page.";
	private static final String HEADER_MSG = "If external LDAP is enabled, it will be used for all messaging users and groups.";
	
	public static void verifyPageLoaded() throws Exception {
		WebBrowser.waitForReady();
		if  (!WebBrowser.waitForText(HEADER, 10)) {
			throw new Exception(HEADER + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_MSG, 10)) {
			throw new Exception(HEADER_MSG + " not found!!!");
		}
	}
	
	public static void loadURL(IsmTestData testData) throws Exception {
		WebBrowser.loadURL(testData.getURL() + "/messaging.jsp?nav=userAdministration");
		verifyPageLoaded();
	}
	
	public static void addLDAP(ImsLDAPTestData testData) throws Exception {
		
		MessagingUsersandGroupsPage.getGridIcon_LDAPAdd().click();
		
		String s;
		
		s = testData.getLDAPURL();
		if (s != null) {
			AddLDAPConnectionDialog.getTextEntry_URL().setText(s);
		}
		
		// Max Connections is no longer settable in the Web UI
		//s = testData.getLDAPMaxConnections();
		//if ( s != null) {
		//	AddLDAPConnectionDialog.getTextEntry_MaxConnections().setText(s);
		//}
		
		s = testData.getLDAPTimeout();
		if ( s != null) {
			AddLDAPConnectionDialog.getTextEntry_Timeout().setText(s);
		}
		
		s = testData.getLDAPCacheTimeout();
		if ( s != null) {
			AddLDAPConnectionDialog.getTextEntry_CacheTimeout().setText(s);
		}
		
		s = testData.getLDAPGroupCacheTimeout();
		if ( s != null) {
			AddLDAPConnectionDialog.getTextEntry_GroupCacheTimeout().setText(s);
		}
		
		s = testData.getLDAPBaseDN();
		if ( s != null) {
			AddLDAPConnectionDialog.getTextEntry_BaseDN().setText(s);
		}
		
		s = testData.getLDAPBindDN();
		if ( s != null) {
			AddLDAPConnectionDialog.getTextEntry_BindDN().setText(s);
		}
		
		s = testData.getLDAPBindPassword();
		if ( s != null) {
			AddLDAPConnectionDialog.getTextEntry_BindPassword().setText(s);
		}
		
		s = testData.getLDAPUserSuffix();
		if (s != null) {
			AddLDAPConnectionDialog.getTextEntry_UserSuffix().setText(s);
		}
		
		s = testData.getLDAPUserIdMap();
		if ( s != null) {
			AddLDAPConnectionDialog.getTextEntry_UserIDMap().setText(s);
		}
		
		s = testData.getLDAPGroupMemberIdMap();
		if ( s != null) {
			AddLDAPConnectionDialog.getTextEntry_GroupMemberIDMap().setText(s);
		}
		
		s = testData.getLDAPGroupIdMap();
		if ( s != null) {
			AddLDAPConnectionDialog.getTextEntry_GroupIDMap().setText(s);
		}
		
		s = testData.getLDAPGroupSuffix();
		if ( s != null) {
			AddLDAPConnectionDialog.getTextEntry_GroupSuffix().setText(s);
		}
		
		AddLDAPConnectionDialog.getButton_Save().click();
		
		verifyPageLoaded();
		
		s = testData.getLDAPEnabled();
		if (s != null && s.equalsIgnoreCase("true")) {
			MessagingUsersandGroupsPage.getButton_EnableLDAP().click();
			LDAPSettingsChangedDialog.getButton_Close().click();
		}

	}
}
