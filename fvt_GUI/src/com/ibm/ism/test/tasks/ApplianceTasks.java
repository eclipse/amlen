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
/**
 * 
 */
package com.ibm.ism.test.tasks;


import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.ApplianceObjects;
import com.ibm.ism.test.common.IsmTest;

/**
 * Provides actions and tasks that can performed on the ISM Appliance tab.
 * 
 *
 */
public class ApplianceTasks {
	
	public static boolean clickApplianceTab(String textToVerify) {
		// Appliance page 
		ApplianceObjects.getTab_Appliance().hover();
		ApplianceObjects.getTab_Appliance().click();
		
//BB1	IsmTest.waitForBrowserReady();
//		if (WebBrowser.isTextPresent("Leave First Steps")){
//			FirstStepsObjects.getButton_LeaveFirstSteps().hover();
//			FirstStepsObjects.getButton_LeaveFirstSteps().click();
//	    }	
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	/**
	 * Manipulate User Actions
	 * Add User
	 * Delete User
	 * Edit User
	 */
	
	public static boolean clickUsersAction(String textToVerify) {
		ApplianceObjects.getMenu_Appliance().hover();
		ApplianceObjects.getMenu_Appliance().click();
		IsmTest.waitForBrowserReady();
		ApplianceObjects.getMenuItem_ApplianceUsers().hover();
		ApplianceObjects.getMenuItem_ApplianceUsers().click();
		IsmTest.waitForBrowserReady();
//		if (FirstStepsDialogTask.verifyFirstStepsDialog() == true) {
//			FirstStepsDialogTask.leaveFirstSteps();
//	        IsmTest.delay ();
//		} 
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		} else {
			return WebBrowser.isTextPresent("User Role Configuration");
		}
	}

	public static boolean addNewUser() {
		
		ApplianceObjects.getUsersToolbar().hover();
		ApplianceObjects.getUsersToolbar().click();
		IsmTest.delay(5000);
		ApplianceObjects.add_Users().hover();
		ApplianceObjects.add_Users().click();
		IsmTest.delay(2000);
		return IsmTest.waitForBrowserReady();			
	}
	
	public static boolean clickAddUserDialog() {
		ApplianceObjects.getDialogBox_AddUser().hover();
		ApplianceObjects.getDialogBox_AddUser().click();
		IsmTest.delay(1000);
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean clickUsersTable() {
		ApplianceObjects.getTable_Webuiusers().hover();
		ApplianceObjects.getTable_Webuiusers().click();
		IsmTest.delay(2000);
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean selectUserFromTable() {
		//ApplianceObjects.getTable_WebuiusersUserIDCellText(2).hover();
		//ApplianceObjects.getTable_WebuiusersUserIDCellText(2).click();
		ApplianceObjects.getTable_WebuiusersUserIDCellText().hover();
		ApplianceObjects.getTable_WebuiusersUserIDCellText().click();
		ApplianceObjects.getTable_WebuiusersUserIDCellText().doubleClick();
		IsmTest.delay(2000);
		return IsmTest.waitForBrowserReady();
	}
    public static boolean deleteNewUser() {
		
		ApplianceObjects.getUsersToolbar().hover();
		ApplianceObjects.getUsersToolbar().click();
		IsmTest.delay(5000);
    	
		ApplianceObjects.delete_Users().hover();
		ApplianceObjects.delete_Users().click();
		ApplianceObjects.delete_Users().doubleClick();
		IsmTest.delay(5000);
		return IsmTest.waitForBrowserReady();			
	}
	public static boolean clickDeleteUserDialog() {
		ApplianceObjects.getDialogBox_DeleteUser().hover();
		ApplianceObjects.getDialogBox_DeleteUser().click();
		IsmTest.delay(2000);
		return IsmTest.waitForBrowserReady();
	}
	
   public static boolean deleteButtonDeleteUser() {
		
		//ApplianceObjects.getDialogBox_AddUser().hover();
		//ApplianceObjects.getDialogBox_AddUser().click();
		//IsmTest.delay(5000);
		ApplianceObjects.getButton_Delete().hover();
		ApplianceObjects.getButton_Delete().click();
		IsmTest.delay(5000);
		return IsmTest.waitForBrowserReady();	
	}
   
   public static boolean cancelButtonDeleteUser() {
		
		//ApplianceObjects.getDialogBox_AddUser().hover();
		//ApplianceObjects.getDialogBox_AddUser().click();
		//IsmTest.delay(5000);
		ApplianceObjects.getButton_Cancel().hover();
		ApplianceObjects.getButton_Cancel().click();
		IsmTest.delay(5000);
		return IsmTest.waitForBrowserReady();	
	}
	
   
	public static boolean setUserID(String property) {
		//ApplianceObjects.getDialogBox_AddUser().hover();
		//ApplianceObjects.getDialogBox_AddUser().click();
		//IsmTest.delay(1000);
		ApplianceObjects.getTextEntry_UserID().hover();
		ApplianceObjects.getTextEntry_UserID().click();
		setUserIDProp(property);
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean setDescription(String property) {
		//ApplianceObjects.getDialogBox_AddUser().hover();
		//ApplianceObjects.getDialogBox_AddUser().click();
		//IsmTest.delay(5000);
		ApplianceObjects.getTextEntry_Description().hover();
		ApplianceObjects.getTextEntry_Description().click();
		setUserDescProp(property);
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean setUserPassword(String property) {
		//ApplianceObjects.getDialogBox_AddUser().hover();
		//ApplianceObjects.getDialogBox_AddUser().click();
		//IsmTest.delay(5000);
		ApplianceObjects.getTextEntry_Password().hover();
		ApplianceObjects.getTextEntry_Password().click();
		setUserPasswordProp(property);
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean setUserConfirmPassword(String property) {
		//ApplianceObjects.getDialogBox_AddUser().hover();
		//ApplianceObjects.getDialogBox_AddUser().click();
		//IsmTest.delay(5000);
		ApplianceObjects.getTextEntry_ConfirmPassword().hover();
		ApplianceObjects.getTextEntry_ConfirmPassword().click();
		setUserConfirmPasswordProp(property);
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean clickDropDownGroupMembership() {
		//ApplianceObjects.getDialogBox_AddUser().hover();
		//ApplianceObjects.getDialogBox_AddUser().click();
		//IsmTest.delay(5000);
		ApplianceObjects.getDropDownMenuIcon_GroupMembership().hover();
		ApplianceObjects.getDropDownMenuIcon_GroupMembership().click();
		//setUserConfirmPasswordProp(property);
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean selectDropDownGroup_SysAdmins() {
		//ApplianceObjects.getDialogBox_AddUser().hover();
		//ApplianceObjects.getDialogBox_AddUser().click();
		//IsmTest.delay(5000);
		ApplianceObjects.selectCheckBox_SystemAdministrators().hover();
		ApplianceObjects.selectCheckBox_SystemAdministrators().click();
		//setUserConfirmPasswordProp(property);
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean selectDropDownGroup_MsgAdmins() {
		//ApplianceObjects.getDialogBox_AddUser().hover();
		//ApplianceObjects.getDialogBox_AddUser().click();
		//IsmTest.delay(5000);
		ApplianceObjects.selectCheckBox_MessagingAdministrators().hover();
		ApplianceObjects.selectCheckBox_MessagingAdministrators().click();
		//setUserConfirmPasswordProp(property);
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean selectDropDownGroup_Users() {
		//ApplianceObjects.getDialogBox_AddUser().hover();
		//ApplianceObjects.getDialogBox_AddUser().click();
		//IsmTest.delay(5000);
		ApplianceObjects.selectCheckBox_Users().hover();
		ApplianceObjects.selectCheckBox_Users().click();
		//setUserConfirmPasswordProp(property);
		return IsmTest.waitForBrowserReady();
	}
public static boolean saveUserID() {
		
		//ApplianceObjects.getDialogBox_AddUser().hover();
		//ApplianceObjects.getDialogBox_AddUser().click();
		//IsmTest.delay(5000);
		ApplianceObjects.getButton_AddUsers_Save().hover();
		ApplianceObjects.getButton_AddUsers_Save().click();
		IsmTest.delay(5000);
		return IsmTest.waitForBrowserReady();	
	}
	

	public static boolean clickPortsAction(String textToVerify) {
		ApplianceObjects.getMenuItem_Ports().hover();
		ApplianceObjects.getMenuItem_Ports().click();
		IsmTest.waitForBrowserReady();
//		if (FirstStepsDialogTask.verifyFirstStepsDialog() == true) {
//			FirstStepsDialogTask.leaveFirstSteps();
//	        IsmTest.delay ();
//		} 
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		} else {
			return WebBrowser.isTextPresent("Port Configuration");
		}		
	}

	public static boolean clickNetworkSettingsAction(String textToVerify) {
		ApplianceObjects.getMenu_Appliance().hover();
		ApplianceObjects.getMenu_Appliance().click();
		IsmTest.waitForBrowserReady();
		ApplianceObjects.getMenuItem_NetworkSettings().hover();
		ApplianceObjects.getMenuItem_NetworkSettings().click();
		IsmTest.waitForBrowserReady();
//		if (FirstStepsDialogTask.verifyFirstStepsDialog() == true) {
//			FirstStepsDialogTask.leaveFirstSteps();
//	        IsmTest.delay ();
//		} 
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;		
	}

	public static boolean clickProfilesAction(String textToVerify) {
		ApplianceObjects.getMenuItem_Profiles().hover();
		ApplianceObjects.getMenuItem_Profiles().click();
		IsmTest.waitForBrowserReady();
//		if (FirstStepsDialogTask.verifyFirstStepsDialog() == true) {
//			FirstStepsDialogTask.leaveFirstSteps();
//	        IsmTest.delay ();
//		} 
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;		
	}
	
	public static boolean clickLocaleAction(String textToVerify) {
		ApplianceObjects.getMenu_Appliance().hover();
		ApplianceObjects.getMenu_Appliance().click();
		IsmTest.waitForBrowserReady();
		ApplianceObjects.getMenuItem_Locale().hover();
		ApplianceObjects.getMenuItem_Locale().click();
		IsmTest.waitForBrowserReady();
//		if (FirstStepsDialogTask.verifyFirstStepsDialog() == true) {
//			FirstStepsDialogTask.leaveFirstSteps();
//	        IsmTest.delay ();
//		} 
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;		
	}
	public static boolean toggleEthernetInterfaces() {
		ApplianceObjects.getTwistie_EthernetInterfaces().hover();
		ApplianceObjects.getTwistie_EthernetInterfaces().click();
		return IsmTest.waitForBrowserReady();
	}

	public static boolean toggleServerLocale() {
		ApplianceObjects.getTwistie_ServerTest().hover();
		ApplianceObjects.getTwistie_ServerTest().click();
		return IsmTest.waitForBrowserReady();
	}

	public static boolean expandEthernetInterfaces() {
		String style = ApplianceObjects.getTwistie_EthernetInterfaces().getProperty("style");
		if (style.contains("closed")) {
			return toggleEthernetInterfaces();
		}
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean expandServerLocale() {	
		String style = ApplianceObjects.getTwistie_ServerTest().getProperty("style");
		if (style.contains("closed")) {
			return toggleServerLocale();
		}
		return IsmTest.waitForBrowserReady();
	}

	public static boolean toggleDNS() {
		ApplianceObjects.getTwistie_DNS().hover();
		ApplianceObjects.getTwistie_DNS().click();
		return IsmTest.waitForBrowserReady();
	}

	public static boolean expandDNS() {
		String style = ApplianceObjects.getTwistie_DNS().getProperty("style");
		if (style.contains("closed")) {
			return toggleDNS();
		}
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean toggleServerTest() {
		ApplianceObjects.getTwistie_ServerTest().hover();
		ApplianceObjects.getTwistie_ServerTest().click();
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean expandServerTest() {
		String style = ApplianceObjects.getTwistie_ServerTest().getProperty("style");
		if (style.contains("closed")) {
			return toggleServerTest();
		}
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean setServerTestProperty(String property) {
		ApplianceObjects.getTextEntry_ServerTestProperty().focus();
		ApplianceObjects.getTextEntry_ServerTestProperty().setText(property);
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean setServerTestPropertyValue(String value) {
		ApplianceObjects.getTextEntry_ServerTestValue().focus();
		ApplianceObjects.getTextEntry_ServerTestValue().setText(value);
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean setServerConfiguration(String property, String value) {
		setServerTestProperty(property);
		setServerTestPropertyValue(value);
		ApplianceObjects.getButton_Send().hover();
		ApplianceObjects.getButton_Send().click();
		return IsmTest.waitForBrowserReady();
	}
	
	/**
	 * Opens the Edit Ethernet Interface dialog for an interface
	 * @param name Name of network interface, e.g. eth0
	 * @return true if successful
	 */
	public static boolean editEthernetInterface(String name) {
		expandEthernetInterfaces();
		
		// Click on Edit image to display the Network Interface dialog
		ApplianceObjects.getImage_Edit(name).click();
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean editServerLocale(String name) {
		expandServerLocale();
		
		// Click on Edit image to display the Network Interface dialog
		ApplianceObjects.getImage_Edit(name).click();
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean closeEthernetInterfaceDialog() {
		EthernetInterfacesDialogTasks.close();
		return IsmTest.waitForBrowserReady();
	}
	
	/**
	 * Set Default Gateway for an interface
	 * @param name Name of the network interface, e.g. eth0
	 * @param value Value to set
	 * @return true if successful
	 */
	public static boolean setDefaultGateway(String name, String value) {
		editEthernetInterface(name);
		EthernetInterfacesDialogTasks.setDefaultGateway(value);
		EthernetInterfacesDialogTasks.close();
		return IsmTest.waitForBrowserReady();
	}

	/**
	 * Set Server Locale
	 * @param name Name of the locale, ex: English (United States)
	 * @param value Value to set
	 * @return true if successful
	 */

	
	//public static boolean setServerLocale_EN_US() {
	///public static boolean setServerLocale_EN_US (String textToVerify){
		//ApplianceObjects.getDropDownMenuIcon_Locale().hover();
		//ApplianceObjects.getDropDownMenuIcon_Locale().click();
		//IsmTest.delay(2000);
		//ApplianceObjects.getMenuItem_en_US().hover();
		//ApplianceObjects.getMenuItem_en_US().click();
		//IsmTest.delay(2000);
		//return IsmTest.waitForBrowserReady();	
///		if (textToVerify != null) {
///			return WebBrowser.isTextPresent(textToVerify);
///		} else {
///			return WebBrowser.isTextPresent("The server locale");
///		}			
		
	//}
	
	public static boolean setServerLocale_EN_US() {
		ApplianceObjects.getDropDownMenuIcon_Locale().hover();
		ApplianceObjects.getDropDownMenuIcon_Locale().click();
		IsmTest.delay(2000);
		ApplianceObjects.getMenuItem_en_US().hover();
		ApplianceObjects.getMenuItem_en_US().click();
		IsmTest.delay(2000);
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean setServerLocale_Albanian() {
		ApplianceObjects.getDropDownMenuIcon_Locale().hover();
		ApplianceObjects.getDropDownMenuIcon_Locale().click();
		IsmTest.delay(2000);
		ApplianceObjects.getMenuItem_AlbanianLocale().hover();
		ApplianceObjects.getMenuItem_AlbanianLocale().focus();
		ApplianceObjects.getMenuItem_AlbanianLocale().click();
		IsmTest.delay(2000);
		return IsmTest.waitForBrowserReady();
		
		
	}
	
	
	/**
	 * Set Server Time Zone
	 * @param name Name of the Time zone, ex: Central Time (America/Chicago)
	 * @param value Value to set
	 * @return true if successful
	 */
	
	public static boolean setServerTimeZoneCentral() {
		ApplianceObjects.getDropDownMenuIcon_TimeZone().hover();
		ApplianceObjects.getDropDownMenuIcon_TimeZone().click();
		IsmTest.delay(5000);
		ApplianceObjects.getMenuItem_CentralTimeZone().hover();
		ApplianceObjects.getMenuItem_CentralTimeZone().click();
		IsmTest.delay(2000);
		return IsmTest.waitForBrowserReady();
		
		
	}
	
	public static boolean setServerTimeZoneAlaska() {
	//	public static boolean setServerTimeZoneAlaska(String textToVerify) {
		ApplianceObjects.getDropDownMenuIcon_TimeZone().hover();
		ApplianceObjects.getDropDownMenuIcon_TimeZone().click();
		IsmTest.delay(5000);
		ApplianceObjects.getMenuItem_AlaskaTimeZone().hover();
		ApplianceObjects.getMenuItem_AlaskaTimeZone().doubleClick();
		IsmTest.delay(2000);
		return IsmTest.waitForBrowserReady();
		
		
	}
	
	//Test starts here for Text
//		IsmTest.waitForBrowserReady();
//		if (textToVerify != null) {
//			return WebBrowser.isTextPresent(textToVerify);
//		} else {
//			return WebBrowser.isTextPresent("The server locale");
//		}
	
//	}	
		
			
	/**
	 * Set Server Date and Time
	 * Date ex: Dec 20, 2012
	 * Time ex: 10:12:31 PM
	 * @return true if successful
	 */
	
    public static boolean setServerDateProp(String property) {
		ApplianceObjects.getTextEntry_ServerDateProperty().focus();
		ApplianceObjects.getTextEntry_ServerDateProperty().setText(property);
		return IsmTest.waitForBrowserReady();
	}
	
    public static boolean setUserIDProp(String property) {
		ApplianceObjects.getTextEntry_UserIDProperty().focus();
		ApplianceObjects.getTextEntry_UserIDProperty().setText(property);
		return IsmTest.waitForBrowserReady();
	}
    
    public static boolean setUserDescProp(String property) {
		ApplianceObjects.getTextEntry_UserDescProperty().focus();
		ApplianceObjects.getTextEntry_UserDescProperty().setText(property);
		return IsmTest.waitForBrowserReady();
	}
    
    public static boolean setUserPasswordProp(String property) {
		ApplianceObjects.getTextEntry_UserPasswordProperty().focus();
		ApplianceObjects.getTextEntry_UserPasswordProperty().setText(property);
		return IsmTest.waitForBrowserReady();
	}
    
    public static boolean setUserConfirmPasswordProp(String property) {
		ApplianceObjects.getTextEntry_UserConfirmPasswordProperty().focus();
		ApplianceObjects.getTextEntry_UserConfirmPasswordProperty().setText(property);
		return IsmTest.waitForBrowserReady();
	}
//	public static boolean setServerDatePropValue(String value) {
//		ApplianceObjects.getTextEntry_ServerDateValue().focus();
//		ApplianceObjects.getTextEntry_ServerDateValue().setText(value);
//		return IsmTest.waitForBrowserReady();
//	}
	
	

	 public static boolean setServerTimeProp(String property) {
			ApplianceObjects.getTextEntry_ServerTimeProperty().focus();
			ApplianceObjects.getTextEntry_ServerTimeProperty().setText(property);
			return IsmTest.waitForBrowserReady();
	}
	
	
	public static boolean setServerDate(String property) {
		ApplianceObjects.getTextEntry_Date().hover();
		ApplianceObjects.getTextEntry_Date().click();
		setServerDateProp(property);
		return IsmTest.waitForBrowserReady();
	}
	
	
	public static boolean setServerTime(String property) {
		ApplianceObjects.getTextEntry_Time().hover();
		ApplianceObjects.getTextEntry_Time().click();
		setServerTimeProp(property);
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean saveServerTimeDate() {
		
		ApplianceObjects.getButton_Save().hover();
		ApplianceObjects.getButton_Save().click();
		IsmTest.delay(5000);
		return IsmTest.waitForBrowserReady();
		
		
	}
	
	public static boolean clickApplianceUsersTab(String textToVerify) {
		ApplianceObjects.getTab_Appliance().hover();
		ApplianceObjects.getTab_Appliance().click();
		ApplianceObjects.getMenuItem_ApplianceUsers().hover();
		ApplianceObjects.getMenuItem_ApplianceUsers().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	public static boolean clickApplianceNetworkSettingsTab(String textToVerify) {
		ApplianceObjects.getTab_Appliance().hover();
		ApplianceObjects.getTab_Appliance().click();
		ApplianceObjects.getMenuItem_ApplianceNetworkSettings().hover();
		ApplianceObjects.getMenuItem_ApplianceNetworkSettings().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	public static boolean clickApplianceDateandTimeTab(String textToVerify) {
		ApplianceObjects.getTab_Appliance().hover();
		ApplianceObjects.getTab_Appliance().click();
		ApplianceObjects.getMenuItem_ApplianceDateandTime().hover();
		ApplianceObjects.getMenuItem_ApplianceDateandTime().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	public static boolean clickApplianceSecuritySettingsTab(String textToVerify) {
		ApplianceObjects.getTab_Appliance().hover();
		ApplianceObjects.getTab_Appliance().click();
		ApplianceObjects.getMenuItem_ApplianceSecuritySettings().hover();
		ApplianceObjects.getMenuItem_ApplianceSecuritySettings().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	public static boolean clickApplianceSystemControlTab(String textToVerify) {
		ApplianceObjects.getTab_Appliance().hover();
		ApplianceObjects.getTab_Appliance().click();
		ApplianceObjects.getMenuItem_ApplianceSystemControl().hover();
		ApplianceObjects.getMenuItem_ApplianceSystemControl().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	public static boolean clickApplianceHighAvailabilityTab(String textToVerify) {
		ApplianceObjects.getTab_Appliance().hover();
		ApplianceObjects.getTab_Appliance().click();
		ApplianceObjects.getMenuItem_ApplianceHighAvailability().hover();
		ApplianceObjects.getMenuItem_ApplianceHighAvailability().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	public static boolean clickApplianceWebUISettingsTab(String textToVerify) {
		ApplianceObjects.getTab_Appliance().hover();
		ApplianceObjects.getTab_Appliance().click();
		ApplianceObjects.getMenuItem_ApplianceWebUISettings().hover();
		ApplianceObjects.getMenuItem_ApplianceWebUISettings().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
}
