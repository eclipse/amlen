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
import com.ibm.ism.test.appobjects.MonitoringObjects;
//import com.ibm.ism.test.appobjects.firststeps.FirstStepsObjects;
import com.ibm.ism.test.common.IsmTest;

/**
 * Provides actions and tasks that can performed on the ISM Monitoring tab.
 * 
 *
 */
public class MonitoringTasks {

	public static boolean clickMonitoringTab(String textToVerify) {
		IsmTest.delay ();
		MonitoringObjects.getTab_Monitoring().hover();
		MonitoringObjects.getTab_Monitoring().click();
		
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
	
	public static boolean clickMonitoringConnectionsTab(String textToVerify) {
		MonitoringObjects.getTab_Monitoring().hover();
		MonitoringObjects.getTab_Monitoring().click();
		MonitoringObjects.getMenuItem_Connections().hover();
		MonitoringObjects.getMenuItem_Connections().click();
		
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
		
	public static boolean clickMonitoringEndpointsTab(String textToVerify) {
		MonitoringObjects.getTab_Monitoring().hover();
		MonitoringObjects.getTab_Monitoring().click();
		MonitoringObjects.getMenuItem_Endpoints().hover();
		MonitoringObjects.getMenuItem_Endpoints().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	public static boolean clickMonitoringQueuesTab(String textToVerify) {
		MonitoringObjects.getTab_Monitoring().hover();
		MonitoringObjects.getTab_Monitoring().click();
		MonitoringObjects.getMenuItem_Queues().hover();
		MonitoringObjects.getMenuItem_Queues().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	public static boolean clickMonitoringTopicsTab(String textToVerify) {
		MonitoringObjects.getTab_Monitoring().hover();
		MonitoringObjects.getTab_Monitoring().click();
		MonitoringObjects.getMenuItem_Topics().hover();
		MonitoringObjects.getMenuItem_Topics().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	public static boolean clickMonitoringMQTTClientsTab(String textToVerify) {
		MonitoringObjects.getTab_Monitoring().hover();
		MonitoringObjects.getTab_Monitoring().click();
		MonitoringObjects.getMenuItem_MQTTClients().hover();
		MonitoringObjects.getMenuItem_MQTTClients().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	public static boolean clickMonitoringSubscriptionsTab(String textToVerify) {
		MonitoringObjects.getTab_Monitoring().hover();
		MonitoringObjects.getTab_Monitoring().click();
		MonitoringObjects.getMenuItem_Subscriptions().hover();
		MonitoringObjects.getMenuItem_Subscriptions().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	public static boolean clickMonitoringApplianceTab(String textToVerify) {
		MonitoringObjects.getTab_Monitoring().hover();
		MonitoringObjects.getTab_Monitoring().click();
		MonitoringObjects.getMenuItem_Appliance().hover();
		MonitoringObjects.getMenuItem_Appliance().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	public static boolean clickMonitoringDownloadLogsTab(String textToVerify) {
		MonitoringObjects.getTab_Monitoring().hover();
		MonitoringObjects.getTab_Monitoring().click();
		MonitoringObjects.getMenuItem_Download().hover();
		MonitoringObjects.getMenuItem_Download().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
		
		
		
		
		
		
		
	public static boolean clickServerDashboardAction(String textToVerify) {
	//	MonitoringObjects.getActionLink_ServerDashboard().hover();
	//	MonitoringObjects.getActionLink_ServerDashboard().click();
		IsmTest.delay ();
		MonitoringObjects.getMenu_Monitoring().hover();
		MonitoringObjects.getMenu_Monitoring().click();
		MonitoringObjects.getMenuItem_ServerDashboard().hover();
		MonitoringObjects.getMenuItem_ServerDashboard().click();
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		} else {
			return WebBrowser.isTextPresent("Server Dashboard");
		}
	}

	public static boolean clickTopicStatisticsAction(String textToVerify) {
		//MonitoringObjects.getActionLink_TopicStatistics().hover();
		//MonitoringObjects.getActionLink_TopicStatistics().click();
		IsmTest.delay ();
		MonitoringObjects.getMenu_Monitoring().hover();
		MonitoringObjects.getMenu_Monitoring().click();
		MonitoringObjects.getMenuItem_TopicStatistics().hover();
		MonitoringObjects.getMenuItem_TopicStatistics().click();
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		} else {
			return WebBrowser.isTextPresent("Topic Statistics");
		}
	}

	public static boolean clickConnectionStatisticsAction(String textToVerify) {
		//MonitoringObjects.getActionLink_ConnectionStatistics().hover();
		//MonitoringObjects.getActionLink_ConnectionStatistics().click();
		IsmTest.delay ();
		MonitoringObjects.getMenu_Monitoring().hover();
		MonitoringObjects.getMenu_Monitoring().click();
		MonitoringObjects.getMenuItem_ConnectionStatistics().hover();
		MonitoringObjects.getMenuItem_ConnectionStatistics().click();
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		} else {
			return WebBrowser.isTextPresent("Connection Statistics");
		}
	}

	public static boolean clickLogViewerAction(String textToVerify) {
		//MonitoringObjects.getActionLink_LogViewer().hover();
		//MonitoringObjects.getActionLink_LogViewer().click();
		IsmTest.delay ();
		MonitoringObjects.getMenu_Monitoring().hover();
		MonitoringObjects.getMenu_Monitoring().click();
		MonitoringObjects.getMenuItem_LogViewer().hover();
		MonitoringObjects.getMenuItem_LogViewer().click();
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		} else {
			return WebBrowser.isTextPresent("Log Viewer");
		}
	}
	
	
}
