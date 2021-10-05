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
package com.ibm.ism.test.appobjects.appliance.systemcontrol;

import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;

import com.ibm.automation.core.web.SeleniumCore;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;

public class SystemControlPageObjects {
	
	public static SeleniumTestObject getLink_EditHostName() {
		return new SeleniumTestObject("//a[@id='applianceControl_editHostname']");
	}
	
	public static SeleniumTextTestObject getValue_HostName() {
		return new SeleniumTextTestObject("//span[@id='applianceControl_hostname']");
	}
	
	public static WebElement getWebElement_HostName() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("applianceControl_hostname"));
	}
	
	public static SeleniumTestObject getLink_LocateOnLED() {
		return new SeleniumTestObject("//a[@id='applianceControl_locateOn']");
	}

	public static SeleniumTestObject getLink_LocateOffLED() {
		return new SeleniumTestObject("//a[@id='applianceControl_locateOff']");
	}

	public static SeleniumTestObject getLink_RestartAppliance() {
		return new SeleniumTestObject("//a[@id='applianceControl_restart']");
	}

	public static SeleniumTestObject getLink_ShutdownAppliance() {
		return new SeleniumTestObject("//a[@id='applianceControl_shutdown']");
	}

	public static SeleniumTestObject getLink_EditSSHHost() {
		return new SeleniumTestObject("//a[@id='applianceControl_editSSHHost']");
	}

	public static SeleniumTestObject getValue_SSHHostSettings() {
		return new SeleniumTestObject("//span[@id='sshHostSettingsValue']");
	}

	public static SeleniumTestObject getValue_SSHHostStatus() {
		return new SeleniumTestObject("//span[@id='sshHostStatusValue']");
	}

	public static SeleniumTestObject getLink_StopMessagingServer() {
		return new SeleniumTestObject("//a[@id='messagingServerControl_stop']");
	}

	public static SeleniumTestObject getLink_ForceStopMessagingServer() {
		return new SeleniumTestObject("//a[@id='messagingServerControl_forceStop']");
	}

	public static SeleniumTestObject getLink_StartMessagingServer() {
		return new SeleniumTestObject("//a[@id='messagingServerControl_start']");
	}

	public static SeleniumTestObject getValue_MessagingServerStatus() {
		return new SeleniumTestObject("//span[@id='messagingServerControl_status']");
	}
	
	public static WebElement getWebElement_MessagingServerStatus() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("messagingServerControl_status"));
	}
	
	public static SeleniumTestObject getDownArrow_SelectRunmodeControl() {
		return new SeleniumTestObject("//td[@id='runModeControl_select_downArrow']");
	}
	
	public static SeleniumTestObject getMenuItem_RunmodeProduction() {
		return new SeleniumTestObject("//td[text()='production']");
	}

	public static SeleniumTestObject getMenuItem_RunmodeMaintenance() {
		return new SeleniumTestObject("//td[text()='maintenance']");
	}
	
	public static SeleniumTestObject getMenuItem_RunmodeCleanStore() {
		return new SeleniumTestObject("//td[text()='clean store']");
	}
	
}
