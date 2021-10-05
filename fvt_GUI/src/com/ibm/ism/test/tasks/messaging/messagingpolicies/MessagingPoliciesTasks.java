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
package com.ibm.ism.test.tasks.messaging.messagingpolicies;


import com.ibm.automation.core.Platform;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ism.test.appobjects.messaging.messagehubs.MessageHubsPage;
import com.ibm.ism.test.appobjects.messaging.messagingpolicy.MsgAddMessagingPolicyDialogObject;
import com.ibm.ism.test.common.IsmGuiTestException;
import com.ibm.ism.test.tasks.messaging.messagehubs.MessageHubsTasks;
import com.ibm.ism.test.testcases.messaging.messagingpolicies.ImsCreateMessagingPoliciesTestData;

public class MessagingPoliciesTasks {

	private static final String HEADER_MPOLICIES = "policy";
	private static final String HEADER_MSG_POLICIES = "messaging policy";
	
	private static final String HEADER_HUBS = "Hubs";
	private static final String HEADER_MESSAGE_HUBS = "MsgHub1";
//	private static final String MAXMSGTYPE = "maxmsgtype";
	
	public static void verifyPageLoaded() throws Exception {
		WebBrowser.waitForReady();
		if  (!WebBrowser.waitForText(HEADER_HUBS, 10)) {
			throw new IsmGuiTestException(HEADER_HUBS + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_MESSAGE_HUBS, 10)) {
			throw new IsmGuiTestException(HEADER_MESSAGE_HUBS + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_MPOLICIES, 10)) {
			throw new IsmGuiTestException(HEADER_MPOLICIES + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_MSG_POLICIES, 10)) {
			throw new IsmGuiTestException(HEADER_MSG_POLICIES + " not found!!!");
		}
	}
	
	public static void loadURL(ImsCreateMessagingPoliciesTestData testData) throws Exception {
		//BUG - Remove hard coded MsgHub1
		//WebBrowser.loadURL(testData.getURL() + "/messaging.jsp?nav=messageHubDetails&Name=MsgHub1");
		//Platform.sleep(3);
		//verifyPageLoaded();
		String msgHub = testData.getMessageHub();
		if (msgHub == null) {
			msgHub = new String("MsgHub1");
		}
		MessageHubsTasks.showMessageHub(testData, msgHub);
	}
	
	
	
	public static void addNewMessagingPolicy(ImsCreateMessagingPoliciesTestData testData) throws Exception {
		//MsgAddMessagingPolicyDialogObject.getTab_MessagingPolicy().hover();
		MsgAddMessagingPolicyDialogObject.getTab_MessagingPolicy().click();
		MessageHubsPage.getGridIcon_MsgPolAdd().click();
		
		if (!WebBrowser.waitForText(MsgAddMessagingPolicyDialogObject.TITLE_ADD_MESSAGING_POLICY, 5)) {
				throw new IsmGuiTestException("'" + MsgAddMessagingPolicyDialogObject.TITLE_ADD_MESSAGING_POLICY + "' not found!!!");
		}
		
		MsgAddMessagingPolicyDialogObject.getTextEntry_MessagingPolicyID().setText(testData.getNewMessagingPoliciesID());
		
		if (testData.getNewMessagingPoliciesDescription() != null) {
			MsgAddMessagingPolicyDialogObject.getTextEntry_MessagingPolicyDesc().setText(testData.getNewMessagingPoliciesDescription());
		}		
		
		MsgAddMessagingPolicyDialogObject.getTextEntry_Destination().setText(testData.getNewDestination());
		
		MsgAddMessagingPolicyDialogObject.getDownArrow_DestinationType().waitForExistence(5);
		MsgAddMessagingPolicyDialogObject.getDownArrow_DestinationType().click();
		Platform.sleep(1);
		
		if (testData.isTopic()) {
			//MsgAddMessagingPolicyDialogObject.getCheckBox_Topic().waitForExistence(5);
			//MsgAddMessagingPolicyDialogObject.getCheckBox_Topic().click();
			MsgAddMessagingPolicyDialogObject.getMenuItem_DestimationTypeTopic().waitForExistence(5);
			MsgAddMessagingPolicyDialogObject.getMenuItem_DestimationTypeTopic().click();
			Platform.sleep(1);
			
			//FIXME: Click only action under test
			MsgAddMessagingPolicyDialogObject.getCheckBox_Publish().click();
			Platform.sleep(1);
			MsgAddMessagingPolicyDialogObject.getCheckBox_Subscribe().click();
			Platform.sleep(1);
			
			if (testData.getNewMaxMessages() != null) {
				MsgAddMessagingPolicyDialogObject.getTextEntry_MaxMessages().setText(testData.getNewMaxMessages());
				Platform.sleep(1);
			}
				
			if (testData.getNewMaxMessagesBehavior() != null) {
				MsgAddMessagingPolicyDialogObject.getDownArrow_MaxMessagesBehavior().click();	
				Platform.sleep(1);
				if (testData.isMaxMsgBehaviorDiscard()) {
					MsgAddMessagingPolicyDialogObject.getMenuItem_DiscardOldestMessages().click();
				} else if (testData.isMaxMsgBehaviorReject()) {
					MsgAddMessagingPolicyDialogObject.getMenuItem_RejectNewMessages().click();
				} else {
					throw new IsmGuiTestException("Test case error: " + testData.getNewMaxMessagesBehavior() + " is not a valid option");
				}
			}
				
			if (testData.isDisconnectedClientNotification()) {
				MsgAddMessagingPolicyDialogObject.getCheckBox_DisconnectedClientNotification().click();
				Platform.sleep(1);
			}

			if (testData.getMaxMessageTTL() != null) {
				MsgAddMessagingPolicyDialogObject.getTextEntry_MaxMessageTTL().setText(testData.getMaxMessageTTL());
				Platform.sleep(1);
			}
		} else if (testData.isQueue()) {
			//MsgAddMessagingPolicyDialogObject.getCheckBox_Queue().waitForExistence(5);
			//MsgAddMessagingPolicyDialogObject.getCheckBox_Queue().click();
			MsgAddMessagingPolicyDialogObject.getMenuItem_DestimationTypeQueue().waitForExistence(5);
			MsgAddMessagingPolicyDialogObject.getMenuItem_DestimationTypeQueue().click();
			Platform.sleep(1);						
			MsgAddMessagingPolicyDialogObject.getCheckBox_Send().click();
			MsgAddMessagingPolicyDialogObject.getCheckBox_Browse().click();
			MsgAddMessagingPolicyDialogObject.getCheckBox_Queue_Receive().click();
			
			if (testData.getMaxMessageTTL() != null) {
				MsgAddMessagingPolicyDialogObject.getTextEntry_MaxMessageTTL().setText(testData.getMaxMessageTTL());
				Platform.sleep(1);
			}
		} else if (testData.isSharedSubscription()){
			//MsgAddMessagingPolicyDialogObject.getCheckBox_Subscription().waitForExistence(5);
			//MsgAddMessagingPolicyDialogObject.getCheckBox_Subscription().click();
			MsgAddMessagingPolicyDialogObject.getMenuItem_DestimationTypeSubscription().waitForExistence(5);
			MsgAddMessagingPolicyDialogObject.getMenuItem_DestimationTypeSubscription().click();
			Platform.sleep(1);
			MsgAddMessagingPolicyDialogObject.getCheckBox_Receive().click();
			MsgAddMessagingPolicyDialogObject.getCheckBox_Control().click();
			Platform.sleep(1);

			if (testData.getNewMaxMessages() != null) {
				MsgAddMessagingPolicyDialogObject.getTextEntry_MaxMessages().setText(testData.getNewMaxMessages());
				Platform.sleep(1);
			}
			
			if (testData.getNewMaxMessagesBehavior() != null) {
				MsgAddMessagingPolicyDialogObject.getDownArrow_MaxMessagesBehavior().click();	
				Platform.sleep(1);
				if (testData.isMaxMsgBehaviorDiscard()) {
					MsgAddMessagingPolicyDialogObject.getMenuItem_DiscardOldestMessages().click();
				} else if (testData.isMaxMsgBehaviorReject()) {
					MsgAddMessagingPolicyDialogObject.getMenuItem_RejectNewMessages().click();
				} else {
					throw new IsmGuiTestException("Test case error: " + testData.getNewMaxMessagesBehavior() + " is not a valid option");
				}
			}			
		} else {
			throw new IsmGuiTestException("Test case error: " + testData.getNewDestinationType() + " is not a valid option");
		}
		
		if (testData.getNewUserID() != null) {
			MsgAddMessagingPolicyDialogObject.getCheckBox_UserID().click();
			Platform.sleep(1);
			MsgAddMessagingPolicyDialogObject.getTextEntry_UserID().setText(testData.getNewUserID());
			Platform.sleep(1);
		}
			
		if (testData.getNewClientAddress() != null) {
			MsgAddMessagingPolicyDialogObject.getCheckBox_clientAddress().click();
			Platform.sleep(1);
			MsgAddMessagingPolicyDialogObject.getTextEntry_ClientIPAddress().setText(testData.getNewClientAddress());
			Platform.sleep(1);
		}
					
		if (testData.getNewCommonName() != null) {
			MsgAddMessagingPolicyDialogObject.getCheckBox_CertificateCommonName().click();
			Platform.sleep(1);
			MsgAddMessagingPolicyDialogObject.getTextEntry_CertificateCommonName().setText(testData.getNewCommonName());
			Platform.sleep(1);
		}
			
		if (testData.getNewClientID() != null) {
			MsgAddMessagingPolicyDialogObject.getCheckBox_ClientID().click();
			Platform.sleep(1);
			MsgAddMessagingPolicyDialogObject.getTextEntry_ClientID().setText(testData.getNewClientID());
			Platform.sleep(1);
		}
			
		if (testData.getNewGroupID() != null) {
			MsgAddMessagingPolicyDialogObject.getCheckBox_GroupID().click();
			Platform.sleep(1);
			MsgAddMessagingPolicyDialogObject.getTextEntry_GroupID().setText(testData.getNewGroupID());
			Platform.sleep(1);
		}

		MsgAddMessagingPolicyDialogObject.getCheckBox_Protocol().click();
		Platform.sleep(1);
		
		MsgAddMessagingPolicyDialogObject.getDownArrow_SelectProtocol().click();
		Platform.sleep(1);				
		
		//FIXME Only click the protocol under test
		// For examples,
		//    Topic MQTT
		//    Topic JMS
		//    Shared Subscription - MQTT
		MsgAddMessagingPolicyDialogObject.getCheckBox_JMS().click();
		Platform.sleep(1);
		if (testData.isQueue() == false) {
			MsgAddMessagingPolicyDialogObject.getCheckBox_MQTT().click();
			Platform.sleep(1);
		}
		
		//MsgAddMessagingPolicyDialogObject.getButton_Save().focus();
		MsgAddMessagingPolicyDialogObject.getButton_Save().click();
		
		if (testData.getA1Host() != null) {
			//Verify that the Messaging Policy was created.
			try {
				ImaConfig imaConfig = null;
				imaConfig = new ImaConfig(testData.getA1Host(), testData.getUser(), testData.getPassword());
				imaConfig.connectToServer();
				if (imaConfig.messagingPolicyExists(testData.getNewMessagingPoliciesID()) == false) {
					imaConfig.disconnectServer();
					throw new IsmGuiTestException("CLI verification: Messaging Policy " + testData.getNewMessagingPoliciesID() + " was not created.");
				}
				imaConfig.disconnectServer();
			} catch (Exception e) {
				e.printStackTrace();
				throw e;
			}			
		}
		
	}
	
}
