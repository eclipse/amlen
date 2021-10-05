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

//import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.firststeps.FirstStepsDialogTask;
import com.ibm.ism.test.appobjects.ApplianceObjects;
import com.ibm.ism.test.common.IsmTest;
import com.ibm.ism.test.tasks.firststeps.HomeTab;
import com.ibm.ism.test.tasks.firststeps.MessagingTab;
import com.ibm.ism.test.tasks.firststeps.MonitoringTab;

/**
 * Performs navigation tasks on the tabs of the ISM Admin GUI console.
 *  
 *
 */
public class NavigateTabTask {

	

	/**
	 * Navigates the Appliance page on the ISM GUI.
	 * 
	 * @param textToVerify Optional text to verify on the Appliance page.
	 * @return true if successful
	 */
	//public static boolean navigateAppliance(String textToVerify) {
	public static boolean navigateAppliance(String textToVerify) throws Exception {
		boolean success = ApplianceTasks.clickApplianceTab(textToVerify);
		if (!success) {
			return success;
		}
		if (FirstStepsDialogTask.verifyFirstStepsDialog() == true) {
			FirstStepsDialogTask.leaveFirstSteps();
	        IsmTest.delay ();
		} 
		IsmTest.delay ();
		
		
		ApplianceTasks.clickLocaleAction("English");
		IsmTest.delay(20);
		ApplianceTasks.setServerLocale_EN_US();
		IsmTest.delay(20);
		ApplianceTasks.setServerTimeZoneCentral();
		IsmTest.delay(50);
		ApplianceTasks.setServerDate("Nov 25, 2013");
		IsmTest.delay(20);
		ApplianceTasks.setServerTime("12:15:22 AM");
		IsmTest.delay(20);
		ApplianceTasks.saveServerTimeDate();
		IsmTest.delay(20);
		//ApplianceTasks.setServerLocale_Albanian();
		//IsmTest.delay(2000);
		ApplianceTasks.setServerTimeZoneAlaska();
		IsmTest.delay(60);
		ApplianceTasks.clickUsersAction(textToVerify);
		IsmTest.delay(20);
		ApplianceTasks.addNewUser();
		IsmTest.delay(2000);
		//ApplianceTasks.clickAddUserDialog();
		//IsmTest.delay(2000);
		if (!WebBrowser.waitForText(ApplianceObjects.ADDWEBUIUSERS_DESC, 5)) {
			throw new Exception("'" + ApplianceObjects.ADDWEBUIUSERS_DESC + "' not found!!!");
		}
		ApplianceTasks.setUserID("tester911");
		IsmTest.delay(50);
		ApplianceTasks.setDescription("User ID for tester1");
		IsmTest.delay(20);
		ApplianceTasks.setUserPassword("nyc4meto");
		IsmTest.delay(20);
		ApplianceTasks.setUserConfirmPassword("nyc4meto");
		IsmTest.delay(20);
//		ApplianceTasks.clickDropDownGroupMembership();
//		IsmTest.delay(2000);
		ApplianceTasks.saveUserID();
		IsmTest.delay(20);
		//ApplianceTasks.toggleServerLocale();
		//IsmTest.delay(2000);
		//ApplianceTasks.expandServerLocale();
		//IsmTest.delay(2000);
		//ApplianceTasks.toggleServerTest();
		//IsmTest.delay(2000);
		//ApplianceTasks.expandServerTest();
		//IsmTest.delay(2000);
		//ApplianceTasks.editServerLocale("English");
		//IsmTest.delay(2000);
		//ApplianceTasks.setServerLocaleUS();
		IsmTest.delay(20);

		return success;
	//} catch (Exception e) {
	//	VisualReporter.failTest("Unexpected failure", e);
	}
	
	public static boolean navigateApplianceUsers(String textToVerify) {
		boolean success = ApplianceTasks.clickApplianceUsersTab(textToVerify);
		if (!success) {
			return success;
		}
		//ApplianceTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateApplianceNetworkSettings(String textToVerify) {
		boolean success = ApplianceTasks.clickApplianceNetworkSettingsTab(textToVerify);
		if (!success) {
			return success;
		}
		//ApplianceTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateApplianceDateandTime(String textToVerify) {
		boolean success = ApplianceTasks.clickApplianceDateandTimeTab(textToVerify);
		if (!success) {
			return success;
		}
		//ApplianceTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateApplianceSecuritySettings(String textToVerify) {
		boolean success = ApplianceTasks.clickApplianceSecuritySettingsTab(textToVerify);
		if (!success) {
			return success;
		}
		//ApplianceTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateApplianceSystemControl(String textToVerify) {
		boolean success = ApplianceTasks.clickApplianceSystemControlTab(textToVerify);
		if (!success) {
			return success;
		}
		//ApplianceTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateApplianceHighAvailability(String textToVerify) {
		boolean success = ApplianceTasks.clickApplianceHighAvailabilityTab(textToVerify);
		if (!success) {
			return success;
		}
		//ApplianceTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateApplianceWebUISettings(String textToVerify) {
		boolean success = ApplianceTasks.clickApplianceWebUISettingsTab(textToVerify);
		if (!success) {
			return success;
		}
		//ApplianceTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateHome(String textToVerify) {
		boolean success = HomeTasks.clickHomeTab(textToVerify);
		if (!success) {
			return success;
		}
		HomeTab.verifyPageLoaded();
		IsmTest.delay (20);
			
		return success;
	}	
	
	public static boolean navigateMessaging(String textToVerify) {
		boolean success = MessagingTasks.clickMessagingTab(textToVerify);
		if (!success) {
			return success;
		}
		MessagingTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateMessageHub(String textToVerify) {
		boolean success = MessagingTasks.clickMessageHubTab(textToVerify);
		if (!success) {
			return success;
		}
		MessagingTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateMessageQueue(String textToVerify) {
		boolean success = MessagingTasks.clickMessageQueueTab(textToVerify);
		if (!success) {
			return success;
		}
		MessagingTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateMessageMQConnectivity(String textToVerify) {
		boolean success = MessagingTasks.clickMessageMQConnectivityTab(textToVerify);
		if (!success) {
			return success;
		}
		MessagingTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateMessageUser(String textToVerify) {
		boolean success = MessagingTasks.clickMessageUserTab(textToVerify);
		if (!success) {
			return success;
		}
		MessagingTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	public static boolean navigateMonitoring(String textToVerify) {
		boolean success = MonitoringTasks.clickMonitoringTab(textToVerify);
		if (!success) {
			return success;
		}
		MonitoringTab.verifyPageLoaded();
		IsmTest.delay (20);
		
		
		
		
		return success;
	}	
	
	public static boolean navigateMonitoringConnections(String textToVerify) {
		boolean success = MonitoringTasks.clickMonitoringConnectionsTab(textToVerify);
		if (!success) {
			return success;
		}
		MonitoringTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateMonitoringEndpoints(String textToVerify) {
		boolean success = MonitoringTasks.clickMonitoringEndpointsTab(textToVerify);
		if (!success) {
			return success;
		}
		MonitoringTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateMonitoringQueues(String textToVerify) {
		boolean success = MonitoringTasks.clickMonitoringQueuesTab(textToVerify);
		if (!success) {
			return success;
		}
		MonitoringTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateMonitoringTopics(String textToVerify) {
		boolean success = MonitoringTasks.clickMonitoringTopicsTab(textToVerify);
		if (!success) {
			return success;
		}
		MonitoringTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateMonitoringMQTTClients(String textToVerify) {
		boolean success = MonitoringTasks.clickMonitoringMQTTClientsTab(textToVerify);
		if (!success) {
			return success;
		}
		MonitoringTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateMonitoringSubscriptions(String textToVerify) {
		boolean success = MonitoringTasks.clickMonitoringSubscriptionsTab(textToVerify);
		if (!success) {
			return success;
		}
		MonitoringTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateMonitoringAppliance(String textToVerify) {
		boolean success = MonitoringTasks.clickMonitoringApplianceTab(textToVerify);
		if (!success) {
			return success;
		}
		MonitoringTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
	
	public static boolean navigateMonitoringDownload(String textToVerify) {
		boolean success = MonitoringTasks.clickMonitoringDownloadLogsTab(textToVerify);
		if (!success) {
			return success;
		}
		MonitoringTab.verifyPageLoaded();
		IsmTest.delay (20);
		
				
		return success;
	}	
}
