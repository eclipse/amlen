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
package com.ibm.ism.test.tasks.messaging.endpoints;


import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.loggers.control.CoreLogger;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ism.test.appobjects.messaging.endpoint.AddConnectionPoliciesToEndpointDialogObjects;
import com.ibm.ism.test.appobjects.messaging.endpoint.AddMessagingPoliciesToEndpointDialogObjects;
import com.ibm.ism.test.appobjects.messaging.endpoint.AddProtocolsToEndpointDialogObjects;
import com.ibm.ism.test.appobjects.messaging.endpoint.MsgAddEndpointDialogObject;
import com.ibm.ism.test.appobjects.messaging.messagehubs.MessageHubsPage;
import com.ibm.ism.test.common.IsmGuiTestException;
import com.ibm.ism.test.tasks.messaging.messagehubs.MessageHubsTasks;
import com.ibm.ism.test.testcases.messaging.endpoints.ImsCreateEndpointsTestData;

public class EndpointsTasks {

	private static final String HEADER_ENDPOINTS = "Endpoint";
	private static final String HEADER_MSG_ENDPOINTS = "Endpoints";
	private static final String HEADER_HUBS = "Hubs";
	//private static final String HEADER_MESSAGE_HUBS = "MsgHub1";
	
	public static void verifyPageLoaded(String hub) throws Exception {
		WebBrowser.waitForReady();
		if  (!WebBrowser.waitForText(HEADER_HUBS, 10)) {
			throw new IsmGuiTestException(HEADER_HUBS + " not found!!!");
		}
		if  (!WebBrowser.waitForText(hub, 10)) {
			throw new IsmGuiTestException(hub + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_ENDPOINTS, 10)) {
			throw new IsmGuiTestException(HEADER_ENDPOINTS + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_MSG_ENDPOINTS, 10)) {
			throw new IsmGuiTestException(HEADER_MSG_ENDPOINTS + " not found!!!");
		}
	}
	
	public static void loadURL(ImsCreateEndpointsTestData testData) throws Exception {
		MessageHubsTasks.showMessageHub(testData, testData.getMessageHub());
	}
	
	@SuppressWarnings("deprecation")
	public static void addNewEndpoints(ImsCreateEndpointsTestData testData) throws Exception {
		
		//MsgAddEndpointDialogObject.getTab_Endpoint().hover();
		MsgAddEndpointDialogObject.getTab_Endpoint().click();
		MessageHubsPage.getGridIcon_EndPointAdd().click();
		if (!WebBrowser.waitForText(MsgAddEndpointDialogObject.TITLE_ADD_ENDPOINT, 5)) {
			throw new IsmGuiTestException("'" + MsgAddEndpointDialogObject.TITLE_ADD_ENDPOINT + "' not found!!!");
		}
		
		MsgAddEndpointDialogObject.getTextEntry_EndpointID().setText(testData.getEndpointName());
		
		if (testData.getEndpointDescription() != null) {
			MsgAddEndpointDialogObject.getTextEntry_EndpointDesc().setText(testData.getEndpointDescription());
		}
		
		if (testData.getEndpointPort() != null) {
			MsgAddEndpointDialogObject.getTextEntry_EndpointPort().setText(testData.getEndpointPort());
		}
		
		if (testData.getEndpointMaxMessageSize() != null) {
			MsgAddEndpointDialogObject.getTextEntry_EndpointMaxMessageSize().setText(testData.getEndpointMaxMessageSize());
		}
		
		if (testData.enableEndpoint() == true) {
			MsgAddEndpointDialogObject.getCheckbox_EndpointEnabled().click();
		}
		
		if ((testData.getEndpointProtocolJMS() != null) || (testData.getEndpointProtocolMQTT() != null)) {
			
			MsgAddEndpointDialogObject.getLink_AddProtocols().click();
			Platform.sleep(1);
			
			//Wait and verify that Add Protocols dialog is loaded
			AddProtocolsToEndpointDialogObjects.getDialogHeader().waitForExistence(5);
			
			//Clear "All protocols are valid for this endpoint." checkbox so that JMS and MQTT can be selected
			AddProtocolsToEndpointDialogObjects.getCheckBox_AllProtocols().click();
			Platform.sleep(1);
			
			if (testData.getEndpointProtocolJMS() != null) {
				AddProtocolsToEndpointDialogObjects.getCheckBox_JMS().click();
				Platform.sleep(1);
			}
			if (testData.getEndpointProtocolMQTT() != null) {
				AddProtocolsToEndpointDialogObjects.getCheckBox_MQTT().click();
				Platform.sleep(1);
			}
			AddProtocolsToEndpointDialogObjects.getButton_Save().click();
			Platform.sleep(1);
		}

		// FIXME: This will only select the second item from the drop-down menu
		if (testData.getEndpointIPAddress() != null) {
			MsgAddEndpointDialogObject.getDownArrow_IPAddress().waitForExistence(5);
			MsgAddEndpointDialogObject.getDownArrow_IPAddress().click();
			MsgAddEndpointDialogObject.getMenuItem2_IPAddress().click();
			
		}
		
		// FIXME: This will only select the first menu item from the drop-down menu
		if (testData.getEndpointSecurityProfile() != null) {
			Platform.sleep(5);
			Platform.sleep(5);
			MsgAddEndpointDialogObject.getDownArrow_SecurityProfile().waitForExistence(5);
			MsgAddEndpointDialogObject.getDownArrow_SecurityProfile().click();
			MsgAddEndpointDialogObject.getMenuItem1_SecurityProfile().click();
			
		}
		
		if (testData.getConnectionPolicyList() != null) {
			for (String connPol : testData.getConnectionPolicyList()) {
				MsgAddEndpointDialogObject.getGridIcon_ConnectionPoliciesAdd().click();
				if  (!WebBrowser.waitForText(AddConnectionPoliciesToEndpointDialogObjects.TITLE_ADD_CONNECTION_POLICIES_TO_ENDPOINT, 10)) {
					throw new IsmGuiTestException(AddConnectionPoliciesToEndpointDialogObjects.TITLE_ADD_CONNECTION_POLICIES_TO_ENDPOINT + " not found!!!");
				}
				AddConnectionPoliciesToEndpointDialogObjects.getTextBox_Filter().setText(connPol);
				Platform.sleep(1);
				VisualReporter.logScreenCapture(connPol , false);
				//AddConnectionPoliciesToEndpointDialogObjects.getCheckBox_FirstRow().hover();
				AddConnectionPoliciesToEndpointDialogObjects.getCheckBox_FirstRow().click();
				AddConnectionPoliciesToEndpointDialogObjects.getButton_Save().click();
			}
		} else {
			throw new IsmGuiTestException("Test case error: Connection policy list is not provided");
	    }
		
		if (testData.getMessagingPolicyList() != null) {
			for (String msgPol : testData.getMessagingPolicyList()) {
				MsgAddEndpointDialogObject.getGridIcon_MessagingPoliciesAdd().click();
				if  (!WebBrowser.waitForText(AddMessagingPoliciesToEndpointDialogObjects.TITLE_ADD_MESSAGING_POLICIES_TO_ENDPOINT, 10)) {
					throw new IsmGuiTestException(AddMessagingPoliciesToEndpointDialogObjects.TITLE_ADD_MESSAGING_POLICIES_TO_ENDPOINT + " not found!!!");
				}
				AddMessagingPoliciesToEndpointDialogObjects.getTextBox_Filter().setText(msgPol);
				Platform.sleep(1);
				VisualReporter.logScreenCapture(msgPol , false);
				//AddMessagingPoliciesToEndpointDialogObjects.getCheckBox_FirstRow().hover();
				AddMessagingPoliciesToEndpointDialogObjects.getCheckBox_FirstRow().click();
				AddMessagingPoliciesToEndpointDialogObjects.getButton_Save().click();
			}
		} else {			
			throw new IsmGuiTestException("Test case error: Messaging policy list is not provided");
		}
		
		VisualReporter.logScreenCapture(testData.getEndpointName() , false);
		
		MsgAddEndpointDialogObject.getButton_Save().click();
		
		if (testData.getA1Host() != null) {
			//Verify that the Endpoint was created.
			try {
				ImaConfig imaConfig = null;
				imaConfig = new ImaConfig(testData.getA1Host(), testData.getUser(), testData.getPassword());
				CoreLogger.logInfo("Endpoint was created. SSH to the appliance (" + testData.getA1Host() 
						+ ") with user (" + testData.getUser() + ") password (" + testData.getPassword() + ")");
				imaConfig.connectToServer();
				if (imaConfig.endpointExists(testData.getEndpointName()) == false) {
					imaConfig.disconnectServer();
					throw new IsmGuiTestException("CLI verification: Endpoint " + testData.getEndpointName() + " was not created.");
				}
				imaConfig.disconnectServer();
			} catch (Exception e) {
				e.printStackTrace();
				throw e;
			}			
		}
	}
}





