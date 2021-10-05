
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
package com.ibm.ism.test.tasks.messaging.connectionpolicies;


import com.ibm.automation.core.Platform;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ism.test.appobjects.messaging.connectionpolicy.MsgAddConnectionPolicyDialogObject;
import com.ibm.ism.test.appobjects.messaging.messagehubs.MessageHubsPage;
import com.ibm.ism.test.common.IsmGuiTestException;
import com.ibm.ism.test.tasks.messaging.messagehubs.MessageHubsTasks;
import com.ibm.ism.test.testcases.messaging.connectionpolicies.ImsCreateConnectionPoliciesTestData;

public class ConnectionPoliciesTasks {

	private static final String HEADER_CPOLICIES = "policy";
	private static final String HEADER_CONN_POLICIES = "connection policy";
	
	private static final String HEADER_HUBS = "Hubs";
	private static final String HEADER_MESSAGE_HUBS = "MsgHub1";
	
	public static void verifyPageLoaded() throws Exception {
		if  (!WebBrowser.waitForText(HEADER_HUBS, 10)) {
			throw new IsmGuiTestException(HEADER_HUBS + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_MESSAGE_HUBS, 10)) {
			throw new IsmGuiTestException(HEADER_MESSAGE_HUBS + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_CPOLICIES, 10)) {
			throw new IsmGuiTestException(HEADER_CPOLICIES + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_CONN_POLICIES, 10)) {
			throw new IsmGuiTestException(HEADER_CONN_POLICIES + " not found!!!");
		}
	}
	
	public static void loadURL(ImsCreateConnectionPoliciesTestData testData) throws Exception {
		String msgHub = testData.getMessageHub();
		if (msgHub == null) {
			msgHub = new String("MsgHub1");
		}
		MessageHubsTasks.showMessageHub(testData, msgHub);
	}
	
	public static void addNewConnectionPolicy(ImsCreateConnectionPoliciesTestData testData) throws Exception {
		//MsgAddConnectionPolicyDialogObject.getTab_ConnectionPolicy().hover();
		MsgAddConnectionPolicyDialogObject.getTab_ConnectionPolicy().click();
		MessageHubsPage.getGridIcon_ConnPolAdd().click();
		if (!WebBrowser.waitForText(MsgAddConnectionPolicyDialogObject.TITLE_ADD_CONNECTION_POLICY, 5)) {
			throw new IsmGuiTestException("'" + MsgAddConnectionPolicyDialogObject.TITLE_ADD_CONNECTION_POLICY + "' not found!!!");
		}
		
		Platform.sleep(1);
		MsgAddConnectionPolicyDialogObject.getTextEntry_ConnectionPolicyID().waitForExistence(5);
		MsgAddConnectionPolicyDialogObject.getTextEntry_ConnectionPolicyID().setText(testData.getConnectionPolicyName());

		if (testData.getConnectionPolicyDescription() != null) {
			MsgAddConnectionPolicyDialogObject.getTextEntry_ConnectionPolicyDesc().setText(testData.getConnectionPolicyDescription());
		}
		
		if (testData.getUserID() != null) {
			Platform.sleep(1);
			MsgAddConnectionPolicyDialogObject.getCheckBox_UserID().click();
			Platform.sleep(1);
			MsgAddConnectionPolicyDialogObject.getTextEntry_UserID().setText(testData.getUserID());
		}
		
		if (testData.getClientAddress() != null) {
			Platform.sleep(1);
			MsgAddConnectionPolicyDialogObject.getCheckBox_clientAddress().click();
			Platform.sleep(1);
			MsgAddConnectionPolicyDialogObject.getTextEntry_ClientIPAddress().setText(testData.getClientAddress());
		}
				
		if (testData.getCommonName() != null) {
			Platform.sleep(1);
			MsgAddConnectionPolicyDialogObject.getCheckBox_CertificateCommonName().click();
			Platform.sleep(1);
			MsgAddConnectionPolicyDialogObject.getTextEntry_CertificateCommonName().setText(testData.getCommonName());
		}
		
		if (testData.getClientID() != null) {
			Platform.sleep(1);
			MsgAddConnectionPolicyDialogObject.getCheckBox_ClientID().click();
			Platform.sleep(1);
			MsgAddConnectionPolicyDialogObject.getTextEntry_ClientID().setText(testData.getClientID());
		}
		
		if (testData.getGroupID() != null) {
			Platform.sleep(1);
			MsgAddConnectionPolicyDialogObject.getCheckBox_GroupID().click();
			Platform.sleep(1);
			MsgAddConnectionPolicyDialogObject.getTextEntry_GroupID().setText(testData.getGroupID());
		}
		
		Platform.sleep(1);
		MsgAddConnectionPolicyDialogObject.getCheckBox_Protocol().click();
		Platform.sleep(1);
		MsgAddConnectionPolicyDialogObject.getDownArrow_SelectProtocol().click();
		Platform.sleep(1);
		
		//FIXME Only click the protocol under test
		MsgAddConnectionPolicyDialogObject.getCheckBox_JMS().click();
		Platform.sleep(1);
		MsgAddConnectionPolicyDialogObject.getCheckBox_MQTT().click();
		Platform.sleep(1);

		if (testData.allowDurable() == false) {
			MsgAddConnectionPolicyDialogObject.getCheckBox_AllowDurable().click();
			Platform.sleep(1);
		}
		
		if (testData.allowPersistentMessages() == false) {
			MsgAddConnectionPolicyDialogObject.getCheckBox_AllowPersistentMessages().click();
			Platform.sleep(1);
		}
		
		MsgAddConnectionPolicyDialogObject.getButton_Save().click();
		
		if (testData.getA1Host() != null) {
			//Use the CLI to verify that the Connection Policy was created.
			try {
				ImaConfig imaConfig = null;
				imaConfig = new ImaConfig(testData.getA1Host(), testData.getUser(), testData.getPassword());
				imaConfig.connectToServer();
				if (imaConfig.connectionPolicyExists(testData.getConnectionPolicyName()) == false) {
					imaConfig.disconnectServer();
					throw new IsmGuiTestException("CLI verification: Connection Policy " + testData.getConnectionPolicyName() + " was not created.");
				}
				imaConfig.disconnectServer();
			} catch (Exception e) {
				e.printStackTrace();
				throw e;
			}
		}
	}
}


