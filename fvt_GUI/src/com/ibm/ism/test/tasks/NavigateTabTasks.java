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
import com.ibm.ism.test.appobjects.MessagingObjects;
import com.ibm.ism.test.common.IsmTest;

/**
 * Performs navigation tasks on the tabs of the ISM Admin GUI console.
 *  
 *
 */
public class NavigateTabTasks {


	/**
	 * Navigates the Home page.
	 * 
	 * @param textToVerify Optional text to verify on the Home page.
	 * @return true if successful
	 */
	public static boolean navigateHome(String textToVerify) {
		// Home page
		boolean success = HomeTasks.clickHomeTab(textToVerify);
		if (!success) {
			return success;
		}
		HomeTasks.expandDashboard();
		IsmTest.delay();
		HomeTasks.toggleDashboard();

		HomeTasks.expandRecommendedTasks();
		IsmTest.delay();
		
		//Show that we can close and restore all tasks
		HomeTasks.closeUsersTask();
		IsmTest.delay();
		HomeTasks.closePortsTask();
		IsmTest.delay();
		HomeTasks.closeTopicsTask();
		IsmTest.delay();
		HomeTasks.restoreTasks();
		IsmTest.delay();
		
		//Show that we can go directly to a page
		HomeTasks.expandRecommendedTasks();
		IsmTest.delay();
		
		if (IsmTest.gsUserRole.equals(IsmTest.APPLIANCE_ADMIN_ROLE)) {
			HomeTasks.clickLink("Network Settings");
		} else {
			HomeTasks.clickLink("Topics");
		}
		IsmTest.delay();
		
		// Back to Home tab
		HomeTasks.clickHomeTab(textToVerify);
		return success;
	}

	/**
	 * Navigates the Messaging page.
	 * 
	 * @param textToVerify Optional text to verify on the Messaging
	 * @return true if successful
	 */
	public static boolean navigateMessaging(String textToVerify) {
		// Messaging page
		MessagingObjects.getTab_Messaging().hover();
		MessagingObjects.getTab_Messaging().click();
		
		WebBrowser.waitForReady(10);
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}

	/**
	 * Navigates the Monitoring page.
	 * 
	 * @param textToVerify Optional text to verify on the Monitoring page.
	 * @return true if successful
	 */
	public static boolean navigateMonitoring(String textToVerify) {
		boolean success = MonitoringTasks.clickMonitoringTab(null);
		if (!success) {
			return success;
		}

		MonitoringTasks.clickTopicStatisticsAction(null);
		MonitoringTasks.clickConnectionStatisticsAction(null);
		if (IsmTest.gsUserRole.equals(IsmTest.APPLIANCE_ADMIN_ROLE)) {
			MonitoringTasks.clickLogViewerAction(null);
		}
		MonitoringTasks.clickServerDashboardAction(null);
		
		return success;
	}

	/**
	 * Navigates the Appliance page.
	 * 
	 * @param textToVerify Optional text to verify on the Appliance page.
	 * @return true if successful
	 */
	public static boolean navigateAppliance(String textToVerify) {
		boolean success = ApplianceTasks.clickApplianceTab(textToVerify);
		if (!success) {
			return success;
		}
		
		ApplianceTasks.clickUsersAction(null);
		ApplianceTasks.clickPortsAction(null);
		ApplianceTasks.clickProfilesAction(null);
		ApplianceTasks.clickNetworkSettingsAction(null);
		IsmTest.delay();
		ApplianceTasks.expandEthernetInterfaces();
		IsmTest.delay();
		ApplianceTasks.toggleEthernetInterfaces();
		ApplianceTasks.expandDNS();
		IsmTest.delay();
		ApplianceTasks.toggleDNS();
		ApplianceTasks.expandServerTest();
		IsmTest.delay();
		ApplianceTasks.setServerConfiguration("TestProperty","1001");
		IsmTest.delay();
		ApplianceTasks.toggleServerTest();
		IsmTest.delay();
		ApplianceTasks.setDefaultGateway("mgt0", "9.3.1.1");
		IsmTest.delay();
		return success;
	}	

}
