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
package com.ibm.ism.test.appobjects;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;

/**
 * Provides access to all test objects on the ISM Appliance tab.
 * 
 *
 */
public class ApplianceObjects {
	
	public static SeleniumTestObject getTab_Appliance() {
		return new SeleniumTestObject("//span[text()='Appliance']");
	}
	
//	public static SeleniumTestObject getActionLink_Users() {
//		return new SeleniumTestObject("//a[@id='actionNav_users_trigger']");
//	}
	
	public static SeleniumTestObject getActionLink_Users() {
		return new SeleniumTestObject("//td[@id='dijit_MenuItem_14_text']");
	//	return new SeleniumTestObject("//span[text()='Users']");
	}

	public static SeleniumTestObject getMenu_Appliance() {
//		return new SeleniumTestObject("//span[@class='dijitInline.dijitTabArrowButton']");
//		return new SeleniumTestObject("//span[@class='dijitTabArrowButton']");
//		return new SeleniumTestObject("//span[@class='tabLabel']");
		return new SeleniumTestObject("//span[text()='Appliance']");
		
	}
	
	public static SeleniumTestObject getMenuItem_ApplianceUsers() {
	//	return new SeleniumTestObject("//td[text()='Users']");
	//	return new SeleniumTestObject("//td[@id='dijit_MenuItem_14_text']");
		return new SeleniumTestObject("//td[@id='navMenuItem_action/appliance/users_text']");
	}
	
	public static SeleniumTestObject getMenuItem_ApplianceNetworkSettings() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/appliance/networkSettings_text']");
		}
	
	public static SeleniumTestObject getMenuItem_ApplianceDateandTime() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/appliance/locale_text']");
		}
	
	public static SeleniumTestObject getMenuItem_ApplianceSecuritySettings() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/appliance/securitySettings_text']");
		}
	
	public static SeleniumTestObject getMenuItem_ApplianceSystemControl() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/appliance/systemControl_text']");
		}
	
	public static SeleniumTestObject getMenuItem_ApplianceHighAvailability() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/appliance/highAvailability_text']");
		}
	
	public static SeleniumTestObject getMenuItem_ApplianceWebUISettings() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/appliance/webuiSecuritySettings_text']");
		}
	
	public static SeleniumTestObject getUsersToolbar() {
		//return new SeleniumTestObject("//span[@id='webuiusersToolbar_Add_label']");
		//return new SeleniumTextTestObject("//div[text()='Add']");
		//return new SeleniumTestObject("//div[@id='webuiusersToolbar']");
		return new SeleniumTestObject("//span[@id='webuiusersGrid_add']");
	}
	
	public static SeleniumTestObject add_Users() {
		//return new SeleniumTestObject("//span[@id='webuiusersToolbar_Add_label']");
		return new SeleniumTestObject("//span[@id='ism_widgets_Dialog_1_title']");
		//return new SeleniumTextTestObject("//div[text()='Add']");
		
	}
	
	public static SeleniumTextTestObject getDialogBox_AddUser() {
		return new SeleniumTextTestObject("//div[@id='idx_oneui_Dialog_1']");
	}
	
	public static SeleniumTextTestObject getTextEntry_UserID() {
		return new SeleniumTextTestObject("//input[@id='ism_widgets_TextBox_2']");
	}
	
	public static SeleniumTextTestObject getTextEntry_Description() {
		return new SeleniumTextTestObject("//input[@id='ism_widgets_TextBox_3']");	
	}
	
	public static SeleniumTextTestObject getTextEntry_Password() {
		return new SeleniumTextTestObject("//input[@id='addwebuiusersDialog_passwd1']");
	}
	
	public static SeleniumTextTestObject getTextEntry_ConfirmPassword() {
		return new SeleniumTextTestObject("//input[@id='addwebuiusersDialog_passwd2']");
	}
	public static SeleniumTestObject getButton_AddUsers_Save() {
		return new SeleniumTestObject("//span[@id='addwebuiusersDialog_saveButton_label']");
	}

	public static SeleniumTestObject getDropDownMenuIcon_GroupMembership() {
		return new SeleniumTextTestObject("//div[@id='widget_idx_form_DropDownMultiSelect_1']//input[@class='dijitReset dijitInputField dijitArrowButtonInner']");
		//return new SeleniumTextTestObject("//div[@id='widget_idx_form_DropDownMultiSelect_1']");
		
	}
	
	
	
	public static SeleniumTestObject selectCheckBox_SystemAdministrators() {
		return new SeleniumTextTestObject("//label[@id='idx_form_DropDownMultiSelect_1_popupSystemAdministrators_label']");
		
	}
	
	public static SeleniumTestObject selectCheckBox_MessagingAdministrators() {
		return new SeleniumTextTestObject("//label[@id='idx_form_DropDownMultiSelect_1_popupMessagingAdministrators_label']");
		
	}
	
	public static SeleniumTestObject selectCheckBox_Users() {
		return new SeleniumTextTestObject("//label[@id='idx_form_DropDownMultiSelect_1_popupUsers_label']");
		
	}
	
	public static SeleniumTestObject delete_Users() {
		//return new SeleniumTestObject("//span[@id='webuiusersToolbar_Delete_label']");
		return new SeleniumTestObject("//span[@id='webuiusersToolbar_Delete']");
		//return new SeleniumTextTestObject("//div[text()='Delete']");
		
	}
	
	
	public static SeleniumTextTestObject getDialogBox_DeleteUser() {
		return new SeleniumTextTestObject("//div[@id='removewebuiusersDialog']");
	}
	
	public static SeleniumTestObject getButton_Delete() {
		return new SeleniumTestObject("//span[@id='removewebuiusersDialog_button_ok_label']");
	}
	
	public static SeleniumTestObject getButton_Cancel() {
		return new SeleniumTestObject("//span[@id='digit_form_Button_3_label']");
	}
	//	public static SeleniumTestObject getActionLink_Ports() {
//		return new SeleniumTestObject("//a[@id='actionNav_ports_trigger']");
//	}
	
	public static SeleniumTestObject getMenuItem_Ports() {
		return new SeleniumTestObject("//td[@id='dijit_MenuItem_15_text']");
	}
	
	public static SeleniumTestObject getMenuItem_NetworkSettings() {
//		return new SeleniumTestObject("//td[@id='dijit_MenuItem_16_text']");
		return new SeleniumTestObject("//td[@id='navMenuItem_action/appliance/networkSettings_text']");
	}
	
	public static SeleniumTestObject getMenuItem_Profiles() {
		return new SeleniumTestObject("//td[@id='dijit_MenuItem_17_text']");
	}
	
	public static SeleniumTestObject getMenuItem_Locale() {
//		return new SeleniumTestObject("//td[@id='dijit_MenuItem_18_text']");
		return new SeleniumTestObject("//td[@id='navMenuItem_action/appliance/locale_text']");
	}
	public static SeleniumTestObject getTwistie_EthernetInterfaces() {
		return new SeleniumTestObject("//span[@id='toggleTrigger_ethernetInterfaces']");
	}

	public static SeleniumTestObject getTwistie_DNS() {
		return new SeleniumTestObject("//span[@id='toggleTrigger_dns']");
	}

	public static SeleniumTestObject getTwistie_ServerTest() {
		return new SeleniumTestObject("//span[@id='toggleTrigger_serverTest']");
	}
	
	public static SeleniumTextTestObject getTextEntry_ServerTestProperty() {
		return new SeleniumTextTestObject("//input[@id='networkTest_key']");
	}

	public static SeleniumTextTestObject getTextEntry_ServerTestValue() {
		return new SeleniumTextTestObject("//input[@id='networkTest_value']");
	}
	
	public static SeleniumTestObject getButton_Send() {
		return new SeleniumTestObject("//span[@id='networkTest_button']");
	}
	
	public static SeleniumTestObject getButton_Save() {
		return new SeleniumTestObject("//span[@id='saveTimeDateButton']");
	}
	public static SeleniumTestObject getImage_Edit(String name) {
		return new SeleniumTestObject("//img[@id='" + name + "_edit']");
	}
	
	public static SeleniumTestObject getMenuItem_en_US() {
		//return new SeleniumTextTestObject("//div[@id='localeCombo_popup137']");
		return new SeleniumTextTestObject("//div[text()='English (United States)']");
	}
	
	public static SeleniumTestObject getMenuItem_AlbanianLocale() {
		//return new SeleniumTextTestObject("//div[@id='localeCombo_popup7']");
		return new SeleniumTextTestObject("//div[text()='Albanian']");
	}
	
	public static SeleniumTestObject getDropDownMenuIcon_Locale() {
		return new SeleniumTextTestObject("//div[@id='widget_localeCombo']//input[@class='dijitReset dijitInputField dijitArrowButtonInner']");
		
	}
	
	public static SeleniumTestObject getMenuItem_CentralTimeZone() {
		//return new SeleniumTextTestObject("//div[@id='timeZoneCombo_popup164']");
		//return new SeleniumTextTestObject("//div[text()='Central Time (America/Chicago)']");
		return new SeleniumTextTestObject("//div[text()='Central Time (America/Chicago)']");
		
	}
	public static SeleniumTestObject getMenuItem_AlaskaTimeZone() {
		return new SeleniumTextTestObject("//div[@id='timeZoneCombo_popup6']");
		
	}
	public static SeleniumTestObject getDropDownMenuIcon_TimeZone() {
		return new SeleniumTextTestObject("//div[@id='widget_timeZoneCombo']//input[@class='dijitReset dijitInputField dijitArrowButtonInner']");
		
	}
	
	
	
	public static SeleniumTextTestObject getTextEntry_Date() {
		return new SeleniumTextTestObject("//input[@id='dateCombo']");
	}
	
	public static SeleniumTextTestObject getTextEntry_Time() {
		return new SeleniumTextTestObject("//input[@id='timeCombo']");
	}
	
	public static SeleniumTextTestObject getTextEntry_ServerDateProperty() {
		return new SeleniumTextTestObject("//input[@id='dateCombo']");
	}
	
	public static SeleniumTextTestObject getTextEntry_UserPasswordProperty() {
		return new SeleniumTextTestObject("//input[@id='addwebuiusersDialog_passwd1']");
	}
	
	public static final String ADDWEBUIUSERS_DESC = "SystemAdministrators can configure the appliance, and MessagingAdministrators can configure messaging.";
	
	public static SeleniumTextTestObject getTextEntry_UserIDProperty() {
		//return new SeleniumTextTestObject("//div[@id='widget_ism_widgets_TextBox_2']");
		return new SeleniumTextTestObject("//input[@id='ism_widgets_TextBox_2']");
	}
	
	public static SeleniumTextTestObject getTextEntry_UserDescProperty() {
		//return new SeleniumTextTestObject("//div[@id='widget_ism_widgets_TextBox_3']");
		return new SeleniumTextTestObject("//input[@id='ism_widgets_TextBox_3']");
	}
	
	public static SeleniumTextTestObject getTextEntry_UserConfirmPasswordProperty() {
		return new SeleniumTextTestObject("//input[@id='addwebuiusersDialog_passwd2']");
	}
	public static SeleniumTextTestObject getTextEntry_ServerTimeProperty() {
		return new SeleniumTextTestObject("//input[@id='timeCombo']");
	}
	
	
	public static SeleniumTextTestObject getTable_Webuiusers() {
		return new SeleniumTextTestObject("//div[@id='webuiusersGrid']");
	}
	
	public static SeleniumTextTestObject getTable_WebuiusersUserIDCellText()
	//public static SeleniumTextTestObject getTable_WebuiusersUserIDCellText(int rowNumber)
	{
		return new SeleniumTextTestObject("//td[text()='tester1']");
		//return new SeleniumTextTestObject("//div[@id='webuiusers']/table/tbody/tr[" + rowNumber + "]/td[2]");
	}
	
	
	//public static WebTable getSugarTable()
	//{
	//	return new WebTable("//form[@id='MassUpdate']/table/tbody/tr");
	//}
	
	
	//public static WebLink getSugarTableDescriptionCellText(int rowNumber)
	//{
	//	return new WebLink("//form[@id='MassUpdate']/table/tbody/tr[" + rowNumber + "]/td[6]");
	//}
	
	
	//public static WebLink getSugarTableOpportunityNumberCellText(int rowNumber)
	//{
	//	return new WebLink("//form[@id='MassUpdate']/table/tbody/tr[" + rowNumber + "]/td[5]");
	//}
	
}
