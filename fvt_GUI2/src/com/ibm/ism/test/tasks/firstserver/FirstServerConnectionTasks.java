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
package com.ibm.ism.test.tasks.firstserver;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.control.CoreLogger;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.appliance.users.ApplianceUsersPageObjects;
import com.ibm.ism.test.appobjects.appliance.users.WebUIAddUserDialogObjects;
//import com.ibm.ism.test.appobjects.appliance.users.WebUIConfigureServerConnectionPageObjects;
import com.ibm.ism.test.appobjects.firstserver.WebUIConfigureServerConnectionPageObjects;
import com.ibm.ism.test.common.IsmGuiTestException;
import com.ibm.ism.test.common.IsmTestData;
import com.ibm.ism.test.testcases.appliance.users.ImsCreateUserTestData;


public class FirstServerConnectionTasks {

	private static final String HEADER_USERS = "Users";
	private static final String HEADER_WEB_UI_USERS = "Web UI Users";
	
	private static final String HEADER_GETSTARTED = "Get started with IBM MessageSight Web UI";
	private static final String HEADER_CONNECTION = "Configure a server connection";
	private static final String HEADER_CONFIRM_EXCEPTION = "You are about to override how Firefox identifies this site";
	
	public static void verifyWebUIUsersPageLoaded() throws Exception {
		WebBrowser.waitForReady();
		if  (!WebBrowser.waitForText(HEADER_USERS, 10)) {
			throw new IsmGuiTestException(HEADER_USERS + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_WEB_UI_USERS, 10)) {
			throw new IsmGuiTestException(HEADER_WEB_UI_USERS + " not found!!!");
		}
	}
	
	public static void verifyServerConnectionsPageLoaded() throws Exception {
		WebBrowser.waitForReady();
		if  (!WebBrowser.waitForText(HEADER_GETSTARTED, 10)) {
			throw new IsmGuiTestException(HEADER_GETSTARTED + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_CONNECTION, 10)) {
			throw new IsmGuiTestException(HEADER_CONNECTION + " not found!!!");
		}
	}
	
	public static void verifyConfirmSecurityExceptionPageLoaded() throws Exception {
		WebBrowser.waitForReady();
		if  (!WebBrowser.waitForText(HEADER_CONFIRM_EXCEPTION, 10)) {
			throw new IsmGuiTestException(HEADER_CONFIRM_EXCEPTION + " not found!!!");
		}
	}
	public static void loadURL(IsmTestData testData) throws Exception {
		CoreLogger.testStart("Go to Web UI Users page");
		WebBrowser.loadURL(testData.getURL() + "/appliance.html?nav=users");
		Platform.sleep(5);
		System.out.println("LoadURL firstserver page ");
		//verifyWebUIUsersPageLoaded();
	}
	
	public static void loadAdminURL(IsmTestData testData) throws Exception {
		CoreLogger.testStart("Go to Web UI Users page");
		WebBrowser.loadURL(testData.getAdminURL() + "/index.html");
	//	WebBrowser.loadURL(testData.getURL() + "/firstserver.jsp");
	//  Platform.sleep(3);
		Platform.sleep(5);
		System.out.println("LoadURL firstserver page ");
		//verifyWebUIUsersPageLoaded();
	}
	
	public static void loadLicenseAgreement(IsmTestData testData) throws Exception {
		CoreLogger.testStart("Go to License Agreement page");
		WebUIConfigureServerConnectionPageObjects.getButton_LicenseAgreementAccept().focus();
		WebUIConfigureServerConnectionPageObjects.getButton_LicenseAgreementAccept().click();
	    System.out.println("Click Accept License Agreement Button ");
	    Platform.sleep(5);
		System.out.println("LoadURL firstserver page ");
		//verifyLicenseAgreementPageLoaded();
	}
	
	public static void loadLicenseAccepted(IsmTestData testData) throws Exception {
		CoreLogger.testStart("Go to License Agreement Accepted dialog");
		WebUIConfigureServerConnectionPageObjects.getButton_LicenseAccepted().focus();
		WebUIConfigureServerConnectionPageObjects.getButton_LicenseAccepted().click();
	    System.out.println("Click Accept License Agreement Button ");
	    Platform.sleep(5);
		System.out.println("LoadURL firstserver page ");
		//verifyLicenseAcceptedPageLoaded();
	}
	
	public static void getVerifyServerConnection(IsmTestData testData) throws Exception {
		CoreLogger.testStart("Go to Verify Server Connection dialog");
		WebUIConfigureServerConnectionPageObjects.getMenu_VerifyServer().click();
		CoreLogger.info("Back from VerifyServer");
	    System.out.println("Click Server: 127.0.0.1 dropdown arrow ");
	    Platform.sleep(5);
	    WebUIConfigureServerConnectionPageObjects.getMenu_VerifyServer2().click();
		CoreLogger.info("Back from VerifyServer2");
	    Platform.sleep(1);
	    //WebUIConfigureServerConnectionPageObjects.getMenu_VerifyServer3().click();
		//CoreLogger.info("Back from VerifyServer3");
	    //Platform.sleep(1);
	   // WebUIConfigureServerConnectionPageObjects.getMenu_VerifyServer4().click();
		//CoreLogger.info("Back from VerifyServer4");
	    //Platform.sleep(1);
	    //WebUIConfigureServerConnectionPageObjects.getMenu_VerifyServer5().click();
		//CoreLogger.info("Back from VerifyServer5");
	    //Platform.sleep(1);
	    //WebUIConfigureServerConnectionPageObjects.getMenu_VerifyServer6().click();
		//CoreLogger.info("Back from VerifyServer6");
	    //Platform.sleep(1);
	    //WebUIConfigureServerConnectionPageObjects.getMenu_VerifyServer7().click();
		//CoreLogger.info("Back from VerifyServer7");
	    //Platform.sleep(1);
		//verifyServerConnectionsPageLoaded();
	}
	
	public static void addServerConnection(IsmTestData testData) throws Exception {
		CoreLogger.testStart("Go to Add Server Connection page");
		WebUIConfigureServerConnectionPageObjects.getGridIcon_ServerConnectionAdd().click();
	    System.out.println("Click Add Server Connection ");
	    Platform.sleep(5);
		System.out.println("LoadURL firstserver page ");
		//verifyServerConnectionsPageLoaded();
	}
	
	public static void getAddCertificate(IsmTestData testData) throws Exception {
		CoreLogger.testStart("Go to Add Certificate page");
		WebUIConfigureServerConnectionPageObjects.getButton_AddException().click();
	    System.out.println("Click Add Exception Button ");
	    Platform.sleep(5);
		System.out.println("Click Add Exception Button for Certificate ");
		//verifyAddCertificatePageLoaded();
	}
	
	public static void getConfirmCertificate(IsmTestData testData) throws Exception {
		CoreLogger.testStart("Go to Confirm Certificate page");
		WebUIConfigureServerConnectionPageObjects.getButton_ConfirmSecurityException().click();
	    System.out.println("Click Confirm Security Exception Button ");
	    Platform.sleep(5);
		System.out.println("Click Confirm Security Exception Button for Certificate ");
		verifyConfirmSecurityExceptionPageLoaded();
	}
	
	public static void getClickHere(IsmTestData testData) throws Exception {
		CoreLogger.testStart("Go to Click Here to get Certificate page");
		WebUIConfigureServerConnectionPageObjects.getLink_ExpandClickHere().focus();
		WebUIConfigureServerConnectionPageObjects.getLink_ExpandClickHere().click();
		System.out.println("Click Here to Confirm Security Exception Button DETAILED ");
		Platform.sleep(5);
		//WebUIConfigureServerConnectionPageObjects.getLink_ClickHere().focus();
		//WebUIConfigureServerConnectionPageObjects.getLink_ClickHere().click();
	    //System.out.println("Click Here to Confirm Security Exception Button ");
	    //WebUIConfigureServerConnectionPageObjects.getLink_ClickHere2().focus();
		//WebUIConfigureServerConnectionPageObjects.getLink_ClickHere2().click();
	    //System.out.println("Click Here to Confirm Security Exception Button 2 ");
	    Platform.sleep(5);
		System.out.println("Click Here to Confirm Security Exception Button for Certificate ");
		//verifyConfirmSecurityExceptionPageLoaded();
	}
	
	
	public static void getServerConnection(IsmTestData testData) throws Exception {
	    
	    Platform.sleep(10);
	    verifyServerConnectionsPageLoaded();
	    System.out.println("Start getServerConnection ");
	    WebUIConfigureServerConnectionPageObjects.getTextEntry_ServerName().click();
	    WebUIConfigureServerConnectionPageObjects.getTextEntry_ServerName().setText(testData.getNewServerName());
	    System.out.println("Set ServerName ");
	    WebUIConfigureServerConnectionPageObjects.getTextEntry_AdminAddress().click();
	    WebUIConfigureServerConnectionPageObjects.getTextEntry_AdminAddress().setText(testData.getNewAdminAddress());
	    System.out.println("Set Admin Address ");
	    WebUIConfigureServerConnectionPageObjects.getTextEntry_AdminPort().click();
	    WebUIConfigureServerConnectionPageObjects.getTextEntry_AdminPort().setText(testData.getNewAdminPort());
	    System.out.println("Set Admin Port ");
	    if (testData.isNewSendWebUICredentials()){
	       WebUIConfigureServerConnectionPageObjects.getCheckBox_SendCredentials().click();
	       System.out.println("Click Send Credentials ");
	       Platform.sleep(1);
	    }
	    WebUIConfigureServerConnectionPageObjects.getTextEntry_Description().click();
	    WebUIConfigureServerConnectionPageObjects.getTextEntry_Description().setText(testData.getNewServerDescription());
	    System.out.println("Set Description ");
	    WebUIConfigureServerConnectionPageObjects.getButton_SaveandManage().click();
	    System.out.println("Click Save and Manage Button ");
	    Platform.sleep(3);
	    WebUIConfigureServerConnectionPageObjects.getButton_Close().click();
	    System.out.println("Click Close Button ");
	   // return;
	}
	
	
	public static void addNewUser(ImsCreateUserTestData testData) throws Exception {
		CoreLogger.testStart("Add New Users page");
		WebBrowser.loadURL(testData.getURL() + "/appliance.html?nav=users");
		Platform.sleep(10);
	    verifyWebUIUsersPageLoaded();
		ApplianceUsersPageObjects.getGridIcon_WebUiUserAdd().click();
		if (!WebBrowser.waitForText(WebUIAddUserDialogObjects.DIALOG_DESC, 5)) {
			throw new IsmGuiTestException("'" + WebUIAddUserDialogObjects.DIALOG_DESC + "' not found!!!");
		}
		Platform.sleep(3);
		WebUIAddUserDialogObjects.getTextEntry_UserID().setText(testData.getNewUserID());
		
		// Error test - duplicate ID
		if (testData.isDuplicateIDTest()) {
			Platform.sleep(3);
			if (!WebBrowser.waitForText(WebUIAddUserDialogObjects.TOOLTIP_DUPLICATE_ID, 5)) {
				CoreLogger.info("Duplicate ID");
				throw new IsmGuiTestException("'" + WebUIAddUserDialogObjects.TOOLTIP_DUPLICATE_ID + "' not found!!!");
			}
			if (WebUIAddUserDialogObjects.getButton_Save().isEnabled()) {
				CoreLogger.info(testData.getNewUserID());
				throw new IsmGuiTestException("Save button is enabled for duplicate ID test, User ID = " + testData.getNewUserID());
			}
			WebUIAddUserDialogObjects.getButton_Cancel().click();
			return;
		}
		
		WebUIAddUserDialogObjects.getTextEntry_UserPassword().setText(testData.getNewUserPwd());
		WebUIAddUserDialogObjects.getTextEntry_ConfirmPassword().setText(testData.getNewUserConfirmPwd());
		
		// Error test - passwords do not match
		if (testData.isPasswordsDoNotMatchTest()) {
			Platform.sleep(3);
			if (!WebBrowser.waitForText(WebUIAddUserDialogObjects.TOOLTIP_PASSWORD_DO_NOT_MATCH, 5)) {
				throw new IsmGuiTestException("'" + WebUIAddUserDialogObjects.TOOLTIP_PASSWORD_DO_NOT_MATCH + "' not found!!!");
			}
			if (WebUIAddUserDialogObjects.getButton_Save().isEnabled()) {
				CoreLogger.error("Save button is enabled for Passwords do not match test");
				CoreLogger.info(testData.getNewUserID());
				throw new IsmGuiTestException("Save button is enabled for Passwords do not match test, User ID = " + testData.getNewUserID());
			}
			WebUIAddUserDialogObjects.getButton_Cancel().click();
			return;			
		}
		
		if (testData.getNewUserDescription() != null) {
			WebUIAddUserDialogObjects.getTextEntry_UserDesc().setText(testData.getNewUserDescription());
		}
		WebUIAddUserDialogObjects.getDownArrow_SelectGroupMembership().waitForExistence(5);
		WebUIAddUserDialogObjects.getDownArrow_SelectGroupMembership().click();
		for (String group : testData.getNewUserGroupMembership()) {
			if (group.equals(WebUIAddUserDialogObjects.GROUP_MEMBERSHIP_SYSTEM_ADMINISTRATORS)) {
				WebUIAddUserDialogObjects.getCheckBox_GroupMembershipSystemAdministrators().waitForExistence(5);
				WebUIAddUserDialogObjects.getCheckBox_GroupMembershipSystemAdministrators().click();
			} else if (group.equals(WebUIAddUserDialogObjects.GROUP_MEMBERSHIP_MESSAGING_ADMINISTRATORS)) {
				WebUIAddUserDialogObjects.getCheckBox_GroupMembershipMessagingAdministrators().waitForExistence(5);
				WebUIAddUserDialogObjects.getCheckBox_GroupMembershipMessagingAdministrators().click();
			} else if (group.equals(WebUIAddUserDialogObjects.GROUP_MEMBERSHIP_USERS)) {
				WebUIAddUserDialogObjects.getCheckBox_GroupMembershipUsers().waitForExistence(5);
				WebUIAddUserDialogObjects.getCheckBox_GroupMembershipUsers().click();
			} else {
				throw new IsmGuiTestException("Test Case error: " + group + " is not supported.");
			}
		}
		WebUIAddUserDialogObjects.getButton_Save().waitForExistence(5);
		WebUIAddUserDialogObjects.getButton_Save().click();
	}
}

