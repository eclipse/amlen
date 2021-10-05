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

public class ClusterMembershipObjects {
	
	public static SeleniumTestObject getTab_cluster() {
		return new SeleniumTestObject("//span[text()='Cluster']");
	}
	
	public static SeleniumTestObject getLink_joinClusterMembership() {
		return new SeleniumTestObject("//a[@id='clusterMembership_joinLink']");
	}
	
	public static SeleniumTestObject getLink_leaveClusterMembership() {
		return new SeleniumTestObject("//a[@id='clusterMembership_leaveLink']");
	}
	
	public static SeleniumTestObject getLink_editClusterMembership() {
		return new SeleniumTestObject("//a[@id='clusterMembership_editLink']");
	}
	
	public static SeleniumTestObject getButton_restartMessageSight() {
		return new SeleniumTestObject("//span[@id='restartInfoclusterMembershipDialog_button_restart']");
	}
	
	public static WebElement getWebElement_membershipStatus() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_statusValue"));
	}
	
	public static WebElement getWebElement_serverName() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_serverName"));
	}
	
	public static WebElement getWebElement_clusterName() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_clusterName"));
	}
	
	public static WebElement getWebElement_serverUri() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_serverUri"));
	}
	
	public static WebElement getWebElement_useMulticastDiscovery() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_useMulticastDiscovery"));
	}
	
	public static WebElement getWebElement_withMembers() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_withMembers"));
	}
	
	public static WebElement getWebElement_controlInterfaceAddress() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_controlAddress"));
	}
	
	public static WebElement getWebElement_controlInterfacePort() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_controlPort"));
	}
	
	public static WebElement getWebElement_controlInterfaceExternalAddress() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_controlExternalAddress"));
	}
	
	public static WebElement getWebElement_controlInterfaceExternalPort() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_controlExternalPort"));
	}
	
	public static WebElement getWebElement_messagingInterfaceAddress() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_messagingAddress"));
	}
	
	public static WebElement getWebElement_messagingInterfacePort() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_messagingPort"));
	}
	
	public static WebElement getWebElement_messagingInterfaceExternalAddress() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_messagingExternalAddress"));
	}
	
	public static WebElement getWebElement_messagingInterfaceExternalPort() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_messagingExternalPort"));
	}
	
	public static WebElement getWebElement_useTLS(){
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("clusterMembership_useTLS"));
	}		
}
