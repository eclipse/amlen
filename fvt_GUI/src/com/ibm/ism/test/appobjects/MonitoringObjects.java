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

/**
 * Provides access to all test objects on the ISM Monitoring tab.
 * 
 *
 */
public class MonitoringObjects {

	public static SeleniumTestObject getTab_Monitoring() {
		return new SeleniumTestObject("//span[text()='Monitoring']");
	}
	
	public static SeleniumTestObject getActionLink_ServerDashboard() {
		return new SeleniumTestObject("//a[@id='actionNav_serverDashboard_trigger']");
	}

	public static SeleniumTestObject getActionLink_TopicStatistics() {
		return new SeleniumTestObject("//a[@id='actionNav_topicStatistics_trigger']");
	}

	public static SeleniumTestObject getActionLink_ConnectionStatistics() {
		return new SeleniumTestObject("//a[@id='actionNav_connectionStatistics_trigger']");
	}

	public static SeleniumTestObject getActionLink_LogViewer() {
		return new SeleniumTestObject("//a[@id='actionNav_logViewer_trigger']");
	}
	public static SeleniumTestObject getMenuItem_ServerDashboard() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/monitoring/serverDashboard_text']");
	}
	public static SeleniumTestObject getMenuItem_TopicStatistics() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/monitoring/topicStatistics_text']");
	}
	public static SeleniumTestObject getMenuItem_ConnectionStatistics() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/monitoring/connectionStatistics_text']");
	}
	public static SeleniumTestObject getMenuItem_LogViewer() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/monitoring/logViewer_text']");
	}
	public static SeleniumTestObject getMenu_Monitoring() {
		return new SeleniumTestObject("//span[text()='Monitoring']");
		
	}
	
	
	public static SeleniumTestObject getMenuItem_Connections() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/monitoring/connectionStatistics_text']");
	}
	public static SeleniumTestObject getMenuItem_Endpoints() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/monitoring/endpointStatistics_text']");
	}
	public static SeleniumTestObject getMenuItem_Queues() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/monitoring/queueMonitor_text']");
	}
	public static SeleniumTestObject getMenuItem_Topics() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/monitoring/topicMonitor_text']");
	}
	public static SeleniumTestObject getMenuItem_MQTTClients() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/monitoring/mqttClientMonitor_text']");
	}
	public static SeleniumTestObject getMenuItem_Subscriptions() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/monitoring/subscriptionMonitor_text']");
	}
	public static SeleniumTestObject getMenuItem_Appliance() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/monitoring/applianceMonitor_text']");
	}
	public static SeleniumTestObject getMenuItem_Download() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/monitoring/downloadLogs_text']");
	}
	
}
