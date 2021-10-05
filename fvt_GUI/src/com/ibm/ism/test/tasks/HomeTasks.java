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
import com.ibm.ism.test.appobjects.HomeObjects;
//import com.ibm.ism.test.appobjects.firststeps.FirstStepsObjects;
import com.ibm.ism.test.common.IsmTest;

/**
 * Provides actions and tasks that can performed on the ISM Welcome tab.
 * 
 *
 */
public class HomeTasks {
	
	private static final String SECTION_COMMON_CONFIG = "Common configuration and customization tasks";
	private static final String VERIFY_CONFIG = "Verify your configuration with the MessageSight Messaging Tester sample application";
	private static final String CUSTOMIZE_APPLIANCE = "Customize appliance settings";
	private static final String SECURE_APPLIANCE = "Secure your appliance";
	private static final String CREATE_USERS_GROUPS = "Create users and groups";
	private static final String CONFIGURE_MESSAGESIGHT = "Configure IBM MessageSight to accept connections";
	private static final String APPLIANCE_DASHBOARD = "Appliance Dashboard";

	//public static boolean verifyPageLoaded() {
	//	WebBrowser.waitForReady();
	//	//return WebBrowser.isTextPresent(SECTION_COMMON_CONFIG);
	//	return WebBrowser.waitForText(SECTION_COMMON_CONFIG, 30);
	//}
	
	public static void verifyPageLoaded() throws Exception{
		WebBrowser.waitForReady();
		if (WebBrowser.waitForText(SECTION_COMMON_CONFIG, 10) == false) {
			throw new Exception(SECTION_COMMON_CONFIG + " not found!");
		}
		if (WebBrowser.waitForText(VERIFY_CONFIG, 10) == false) {
			throw new Exception(VERIFY_CONFIG + " not found!");
		}
		if (WebBrowser.waitForText(CUSTOMIZE_APPLIANCE, 10) == false) {
			throw new Exception(CUSTOMIZE_APPLIANCE + " not found!");
		}
		if (WebBrowser.waitForText(SECURE_APPLIANCE, 10) == false) {
			throw new Exception(SECURE_APPLIANCE + " not found!");
		}
		if (WebBrowser.waitForText(CREATE_USERS_GROUPS, 10) == false) {
			throw new Exception(CREATE_USERS_GROUPS + " not found!");
		}
		if (WebBrowser.waitForText(CONFIGURE_MESSAGESIGHT, 10) == false) {
			throw new Exception(CONFIGURE_MESSAGESIGHT + " not found!");
		}
		if (WebBrowser.waitForText(APPLIANCE_DASHBOARD, 10) == false) {
			throw new Exception(APPLIANCE_DASHBOARD + " not found!");
		}
	}
	

	public static boolean clickHomeTab(String textToVerify) {
		// Home page
		HomeObjects.getTab_Home().hover();
		HomeObjects.getTab_Home().click();
		
//		IsmTest.waitForBrowserReady();
//BB		if (WebBrowser.isTextPresent("Are you sure you want to leave First Steps?")){
//			FirstStepsObjects.getButton_LeaveFirstSteps().hover();
//			FirstStepsObjects.getButton_LeaveFirstSteps().click();
//	    }	
			
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;		
	}
	
	public static boolean toggleRecommendedTasks() {
		HomeObjects.getTwistie_RecommendedTasks().hover();
		HomeObjects.getTwistie_RecommendedTasks().click();
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean expandRecommendedTasks() {
		String style = HomeObjects.getTwistie_RecommendedTasks().getProperty("style");
		if (style.contains("closed")) {
			return toggleRecommendedTasks();
		}
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean toggleDashboard() {
		HomeObjects.getTwistie_Dashboard().hover();
		HomeObjects.getTwistie_Dashboard().click();
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean expandDashboard() {
		String style = HomeObjects.getTwistie_Dashboard().getProperty("style");
		if (style.contains("closed")) {
			return toggleDashboard();
		}
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean closeNetworkSettingsTask() {
		HomeObjects.getButton_CloseNetworkSettingsTask().hover();
		HomeObjects.getButton_CloseNetworkSettingsTask().click();
		return IsmTest.waitForBrowserReady();
	}

	public static boolean closePortsTask() {
		HomeObjects.getButton_ClosePortsTask().hover();
		HomeObjects.getButton_ClosePortsTask().click();
		return IsmTest.waitForBrowserReady();
	}

	public static boolean closeUsersTask() {
		HomeObjects.getButton_CloseUsersTask().hover();
		HomeObjects.getButton_CloseUsersTask().click();
		return IsmTest.waitForBrowserReady();
	}

	public static boolean closeTopicsTask() {
		HomeObjects.getButton_CloseTopicsTask().hover();
		HomeObjects.getButton_CloseTopicsTask().click();
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean restoreTasks() {
		HomeObjects.getLink_RestoreTasks().hover();
		HomeObjects.getLink_RestoreTasks().click();
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean clickLink(String link) {
		boolean success = clickHomeTab(null);
		if (!success) {
			return success;
		}
		HomeObjects.getLink(link).hover();
		HomeObjects.getLink(link).click();
		return IsmTest.waitForBrowserReady();
	}
	
}

