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
package com.ibm.ism.test.appobjects.cluster.membership;

import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;

import com.ibm.automation.core.web.SeleniumCore;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;

public class EditClusterMembershipObjects {

	public static SeleniumTestObject getTitle_editClusterMembership() {
		return new SeleniumTestObject("//span[@id='editClusterMembershipDialog_dialog_title']");
	}
	
	public static WebElement getWebElement_advancedSettings(){
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("editclusterMembershipDialog_advancedSection_titleBarNode"));
	}
	
	public static SeleniumTestObject getTrigger_advancedSettings() {
		return new SeleniumTestObject("//span[@id='editclusterMembershipDialog_advancedSection_titleNode']");
	}
	
	public static SeleniumTextTestObject getTextEntry_serverName() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_serverName']");
	}
	
	public static SeleniumTextTestObject getTextEntry_clusterName() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_clusterName']");
	}
	
	public static SeleniumTextTestObject getTextEntry_controlAddress() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_controlAddress']");
	}
	
	public static WebElement getWebElement_useMulticastDiscovery(){
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("editclusterMembershipDialog_useMulticastDiscovery"));
	}
	
	public static SeleniumTextTestObject getButton_useMulticastDiscovery() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_useMulticastDiscovery']");
	}
	
	public static SeleniumTextTestObject getTextEntry_multicastTimeToLive() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_multicastTimeToLive']");
	}
	
	public static SeleniumTextTestObject getTextEntry_joinClusterWithMembers() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_joinClusterWithMembers']");
	}
	
	public static SeleniumTextTestObject getTextEntry_controlPort() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_controlPort']");
	}
	
	public static SeleniumTextTestObject getTextEntry_controlExternalAddress() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_controlExternalAddress']");
	}
	
	public static SeleniumTextTestObject getTextEntry_controlExternalPort() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_controlExternalPort']");
	}
	
	public static SeleniumTextTestObject getTextEntry_messagingAddress() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_messagingAddress']");
	}
	
	public static SeleniumTextTestObject getTextEntry_messagingPort() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_messagingPort']");
	}
	
	public static SeleniumTextTestObject getTextEntry_messagingExternalAddress() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_messagingExternalAddress']");
	}
	
	public static SeleniumTextTestObject getTextEntry_messagingExternalPort() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_messagingExternalPort']");
	}
	
	public static SeleniumTextTestObject getButton_useTLS() {
		return new SeleniumTextTestObject("//input[@id='editclusterMembershipDialog_useTLS']");
	}
	
	public static WebElement getWebElement_useTLS(){
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("editclusterMembershipDialog_useTLS"));
	}

	public static SeleniumTestObject getButton_save() {
		return new SeleniumTestObject("//span[@id='editclusterMembershipDialog_saveButton']");
	}
	
	public static SeleniumTestObject getButton_cancel() {
		return new SeleniumTestObject("//span[@id='editclusterMembershipDialog_dialog_closeButton']");
	}
}
