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
package com.ibm.ism.test.tasks.cluster.membership;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.cluster.membership.ClusterMembershipObjects;
import com.ibm.ism.test.appobjects.cluster.membership.EditClusterMembershipObjects;
import com.ibm.ism.test.common.IsmGuiTestException;
import com.ibm.ism.test.common.IsmTestData;
import com.ibm.ism.test.testcases.cluster.membership.IsmClusterMembershipData;

public class ClusterMembershipTasks {
	
	private static final String HEADER_CLUSTER_MEMBERSHIP = "Cluster Membership";
	private static final String HEADER_CLUSTER_STATUS = "Status";
	private static final String HEADER_CLUSTER_CONFIGURATION = "Configuration";
	
	public static final String DIALOG_TITLE_EDIT_CLUSTER_MEMBERSHIP = "Edit Cluster Configuration";
	// public static final String DIALOG_DESC_EDIT_CLUSTER_MEMBERSHIP = "Set a cluster name to determine which cluster this server joins. Afer saving a valid configuration, you can join the cluster.";
	
	public static final String DIALOG_TITLE_RESTART_MESSAGESIGHT = "Restart MessageSight";
	
	public static void verifyPageLoaded() throws Exception {
		if  (!WebBrowser.waitForText(HEADER_CLUSTER_MEMBERSHIP, 10)) {
			throw new IsmGuiTestException(HEADER_CLUSTER_MEMBERSHIP + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_CLUSTER_STATUS, 10)) {
			throw new IsmGuiTestException(HEADER_CLUSTER_STATUS + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_CLUSTER_CONFIGURATION, 10)) {
			throw new IsmGuiTestException(HEADER_CLUSTER_CONFIGURATION + " not found!!!");
		}
	}
	
	public static void verifyEditDialogLoaded() throws Exception {
		if (!WebBrowser.waitForText(DIALOG_TITLE_EDIT_CLUSTER_MEMBERSHIP, 5)) {
			throw new IsmGuiTestException(DIALOG_TITLE_EDIT_CLUSTER_MEMBERSHIP + "' not found!!!");
		}
	}
	
	public static void verifyRestartDialogLoaded() throws Exception {
		if (!WebBrowser.waitForText(DIALOG_TITLE_RESTART_MESSAGESIGHT, 5)) {
			throw new IsmGuiTestException(DIALOG_TITLE_RESTART_MESSAGESIGHT + "' not found!!!");
		}
	}
	
	public static void joinCluster() throws Exception {
		verifyPageLoaded();
		if (ClusterMembershipObjects.getWebElement_membershipStatus().getText() == "Not configured") {
			throw new IsmGuiTestException("Need to configure required values (Server Name, Cluster Name, Control Address) before joining cluster!");
		}
		if (ClusterMembershipObjects.getWebElement_membershipStatus().getText().contains("Member of cluster")) {
			throw new IsmGuiTestException("Cannot join cluster if server is already a member of a cluster!");
		}
		ClusterMembershipObjects.getLink_joinClusterMembership().click();
		verifyRestartDialogLoaded();
		ClusterMembershipObjects.getButton_restartMessageSight().click();
		Platform.sleep(35); // Wait for MessageSight to restart
		verifyPageLoaded();
		if (!ClusterMembershipObjects.getWebElement_membershipStatus().getText().contains("Member of cluster")) {
			throw new IsmGuiTestException("Server did not successfully join cluster! Cluster status: " + ClusterMembershipObjects.getWebElement_membershipStatus().getText());
		}
	}
	
	public static void leaveCluster() throws Exception {
		verifyPageLoaded();
		if (!ClusterMembershipObjects.getWebElement_membershipStatus().getText().contains("Member of cluster")) {
			throw new IsmGuiTestException("Cannot leave cluster if server is not a member of a cluster!");
		}
		ClusterMembershipObjects.getLink_leaveClusterMembership().click();
		Platform.sleep(1);
		verifyPageLoaded();
		if (!ClusterMembershipObjects.getWebElement_membershipStatus().getText().contains("Not a member")) {
			throw new IsmGuiTestException("Server did not sucessfully leave cluster! Cluster status: " + ClusterMembershipObjects.getWebElement_membershipStatus().getText());
		}
	}
	
	public static void loadURL(IsmTestData testData) throws Exception {
		WebBrowser.loadURL(testData.getURL() + "/cluster.jsp");
		verifyPageLoaded();
	}
	
	// Edits the cluster configuration 
	public static void editClusterMembershipConfiguration(IsmClusterMembershipData testData) throws Exception{
		// Open Edit Cluster Configuration Dialog
		ClusterMembershipObjects.getLink_editClusterMembership().click();
		verifyEditDialogLoaded();
		
		// Edit basic settings
		EditClusterMembershipObjects.getTextEntry_serverName().setText(testData.getServerName());
		EditClusterMembershipObjects.getTextEntry_clusterName().setText(testData.getClusterName());
		EditClusterMembershipObjects.getTextEntry_controlAddress().setText(testData.getControlInterfaceAddress());
				
		// Trigger advanced settings 
		if (EditClusterMembershipObjects.getWebElement_advancedSettings().getAttribute("class").contains("dijitClosed")) {
			EditClusterMembershipObjects.getTrigger_advancedSettings().hover();
			EditClusterMembershipObjects.getTrigger_advancedSettings().click();
			Platform.sleep(1);
		}
		

		// Edit advanced settings
		if (testData.getUseMulticastDiscovery().contains("Yes")) {
			if (EditClusterMembershipObjects.getWebElement_useMulticastDiscovery().getAttribute("aria-checked").contains("false")) {
				EditClusterMembershipObjects.getButton_useMulticastDiscovery().click();
			}
			EditClusterMembershipObjects.getTextEntry_multicastTimeToLive().setText(testData.getMulticastTimeToLive());
		}
		else {
			if (EditClusterMembershipObjects.getWebElement_useMulticastDiscovery().getAttribute("aria-checked").contains("true")) {
				EditClusterMembershipObjects.getButton_useMulticastDiscovery().click();
			}
		}
		EditClusterMembershipObjects.getTextEntry_joinClusterWithMembers().setText(testData.getJoinClusterWithMembers());
		EditClusterMembershipObjects.getTextEntry_controlPort().setText(testData.getControlInterfacePort());
		EditClusterMembershipObjects.getTextEntry_controlExternalAddress().setText(testData.getControlInterfaceExternalAddress());
		EditClusterMembershipObjects.getTextEntry_controlExternalPort().setText(testData.getControlInterfaceExternalPort());
		EditClusterMembershipObjects.getTextEntry_messagingAddress().setText(testData.getMessagingInterfaceAddress());
		EditClusterMembershipObjects.getTextEntry_messagingPort().setText(testData.getMessagingInterfacePort());
		EditClusterMembershipObjects.getTextEntry_messagingExternalAddress().setText(testData.getMessagingInterfaceExternalAddress());
		EditClusterMembershipObjects.getTextEntry_messagingExternalPort().setText(testData.getMessagingInterfaceExternalPort());
		if (testData.getUseTLS().contains("Yes")) {
			if (EditClusterMembershipObjects.getWebElement_useTLS().getAttribute("aria-checked") == "false") {
				EditClusterMembershipObjects.getButton_useTLS().click();
			}
		}
		else {
			if (EditClusterMembershipObjects.getWebElement_useTLS().getAttribute("aria-checked") == "true") {
				EditClusterMembershipObjects.getButton_useTLS().click();
			}
		}
		
		// Save changes
		EditClusterMembershipObjects.getButton_save().click();
	}
	
	// Verify the cluster configuration changes have been made
	public static void verifyClusterMembershipConfiguration(IsmClusterMembershipData testData) throws Exception{
		verifyPageLoaded();
		String serverName = ClusterMembershipObjects.getWebElement_serverName().getText();
		if (!serverName.equals(testData.getServerName())) {
			throw new IsmGuiTestException("Server Name is '" + serverName + "' while '" + testData.getServerName() + "' is expected.");
		}
		String clusterName = ClusterMembershipObjects.getWebElement_clusterName().getText();
		if (!clusterName.equals(testData.getClusterName())) {
			throw new IsmGuiTestException("Cluster Name is '" + clusterName + "' while '" + testData.getClusterName() + "' is expected.");
		}
		String useMulticastDiscovery = ClusterMembershipObjects.getWebElement_useMulticastDiscovery().getText();
		if (!useMulticastDiscovery.equals(testData.getUseMulticastDiscovery())) {
			throw new IsmGuiTestException("Use Multicast Discovery is '" + useMulticastDiscovery + "' while '" + testData.getUseMulticastDiscovery() + "' is expected.");
		}
		String joinClusterWithMembers = ClusterMembershipObjects.getWebElement_withMembers().getText();
		if (!joinClusterWithMembers.equals(testData.getJoinClusterWithMembers())) {
			throw new IsmGuiTestException("Join Cluster With Members is '" + joinClusterWithMembers + "' while '" + testData.getJoinClusterWithMembers() + "' is expected.");
		}
		String controlInterfaceAddress = ClusterMembershipObjects.getWebElement_controlInterfaceAddress().getText();
		if (!controlInterfaceAddress.equals(testData.getControlInterfaceAddress())) {
			throw new IsmGuiTestException("Control Interface Address is '" + controlInterfaceAddress + "' while '" + testData.getControlInterfaceAddress() + "' is expected.");
		}
		String controlInterfacePort = ClusterMembershipObjects.getWebElement_controlInterfacePort().getText();
		if (!controlInterfacePort.equals(testData.getControlInterfacePort())) {
			throw new IsmGuiTestException("Control Interface Port is '" + controlInterfacePort + "' while '" + testData.getControlInterfacePort() + "' is expected.");
		}
		String controlInterfaceExternalAddress = ClusterMembershipObjects.getWebElement_controlInterfaceExternalAddress().getText();
		if (!controlInterfaceExternalAddress.equals(testData.getControlInterfaceExternalAddress())) {
			throw new IsmGuiTestException("Control Interface External Address is '" + controlInterfaceExternalAddress + "' while '" + testData.getControlInterfaceExternalAddress() + "' is expected.");
		}
		String controlInterfaceExternalPort = ClusterMembershipObjects.getWebElement_controlInterfaceExternalPort().getText();
		if (!controlInterfaceExternalPort.equals(testData.getControlInterfaceExternalPort())) {
			throw new IsmGuiTestException("Control Interface External Port is '" + controlInterfaceExternalPort + "' while '" + testData.getControlInterfaceExternalPort() + "' is expected.");
		}
		String messagingInterfaceAddress = ClusterMembershipObjects.getWebElement_messagingInterfaceAddress().getText();
		if (!messagingInterfaceAddress.equals(testData.getMessagingInterfaceAddress())) {
			throw new IsmGuiTestException("Messaging Interface Address is '" + messagingInterfaceAddress + "' while '" + testData.getMessagingInterfaceAddress() + "' is expected.");
		}
		String messagingInterfacePort = ClusterMembershipObjects.getWebElement_messagingInterfacePort().getText();
		if (!messagingInterfacePort.equals(testData.getMessagingInterfacePort())) {
			throw new IsmGuiTestException("Messaging Interface Port is '" + messagingInterfacePort + "' while '" + testData.getMessagingInterfacePort() + "' is expected.");
		}
		String messagingInterfaceExternalAddress = ClusterMembershipObjects.getWebElement_messagingInterfaceExternalAddress().getText();
		if (!messagingInterfaceExternalAddress.equals(testData.getMessagingInterfaceExternalAddress())) {
			throw new IsmGuiTestException("Messaging Interface External Address is '" + messagingInterfaceExternalAddress + "' while '" + testData.getMessagingInterfaceExternalAddress() + "' is expected.");
		}
		String messagingInterfaceExternalPort = ClusterMembershipObjects.getWebElement_messagingInterfaceExternalPort().getText();
		if (!messagingInterfaceExternalPort.equals(testData.getMessagingInterfaceExternalPort())) {
			throw new IsmGuiTestException("Messaging Interface External Port is '" + messagingInterfaceExternalPort + "' while '" + testData.getMessagingInterfaceExternalPort() + "' is expected.");
		}
	}		
}
