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
 * Provides access to all test objects on the ISM Welcome tab.
 * 
 *
 */
public class HomeObjects {
	
	public static SeleniumTestObject getTab_Home() {
		return new SeleniumTestObject("//span[text()='Home']");
	}
	
	public static SeleniumTestObject getTwistie_RecommendedTasks() {
		return new SeleniumTestObject("//span[@id='toggleTrigger_recommendedTasks']");
	}

	public static SeleniumTestObject getTwistie_Dashboard() {
		return new SeleniumTestObject("//span[@id='toggleTrigger_dashboard']");
	}
	
	public static SeleniumTestObject getLink_RestoreTasks() {
		return new SeleniumTestObject("//a[@id='restoreTasksLink']");
	}
	
	public static SeleniumTestObject getButton_CloseNetworkSettingsTask() {
		return new SeleniumTestObject("//button[@id='applianceSettings_closeButton']");
	}

	public static SeleniumTestObject getButton_CloseUsersTask() {
		return new SeleniumTestObject("//button[@id='users_closeButton']");
	}

	public static SeleniumTestObject getButton_ClosePortsTask() {
		return new SeleniumTestObject("//button[@id='ports_closeButton']");
	}
	
	public static SeleniumTestObject getButton_CloseTopicsTask() {
		return new SeleniumTestObject("//button[@id='topics_closeButton']");
	}
	
	
	public static SeleniumTestObject getLink(String value) {
		return new SeleniumTestObject("//a[text()='" + value + "']");
	}
	public static SeleniumTestObject getMenu_Home() {
		return new SeleniumTestObject("//span[text()='Home']");
		
	}
}

