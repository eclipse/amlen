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
package com.ibm.ism.test.tasks.messaging.messagehubs;


import org.openqa.selenium.NoSuchElementException;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ism.test.appobjects.messaging.messagehubs.EditMessageHubDialogObject;
import com.ibm.ism.test.appobjects.messaging.messagehubs.MessageHubsPage;
import com.ibm.ism.test.appobjects.messaging.messagehubs.MsgAddMessageHubDialogObject;
import com.ibm.ism.test.common.IsmGuiTestException;
import com.ibm.ism.test.common.IsmTestData;
import com.ibm.ism.test.testcases.messaging.messagehubs.ImsCreateMessageHubTestData;
import com.ibm.ism.test.testcases.messaging.messagehubs.ImsEditMessageHubTestData;

public class MessageHubsTasks {

	private static final String HEADER_HUBS = "Hubs";
	private static final String HEADER_MESSAGE_HUBS = "Message Hubs";
	//private static final String TITLE_MSG_HUB_NOT_FOUND = "The message hub cannot be found. It might have been deleted by another user.";
	private static final String TITLE_MSG_HUB_NOT_FOUND = "The message hub cannot be found.";
	
	public static void verifyPageLoaded() throws Exception {
		WebBrowser.waitForReady();
		if  (!WebBrowser.waitForText(HEADER_HUBS, 10)) {
			throw new IsmGuiTestException(HEADER_HUBS + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_MESSAGE_HUBS, 10)) {
			throw new IsmGuiTestException(HEADER_MESSAGE_HUBS + " not found!!!");
		}
	}
	
	public static void loadURL(IsmTestData testData) throws Exception {
		WebBrowser.loadURL(testData.getURL() + "/messaging.html?nav=messageHubs");
		Platform.sleep(3);
		verifyPageLoaded();
	}
	
	@SuppressWarnings("deprecation")
	public static void showMessageHub(IsmTestData testData, String hub) throws Exception {
		WebBrowser.loadURL(testData.getURL() + "/messaging.jsp?nav=messageHubDetails&Name=" + hub);
		WebBrowser.waitForReady();
		
		try {
			if (MessageHubsPage.getErrorMessage().isEnabled()) {
				VisualReporter.logScreenCapture(MessageHubsPage.getErrorMessage().getText() , false);
				throw new IsmGuiTestException(MessageHubsPage.getErrorMessage().getText());
			}
		} catch (NoSuchElementException e) {
			// No error message
		}

		if  (!WebBrowser.waitForText(HEADER_HUBS, 10)) {
			throw new IsmGuiTestException(HEADER_HUBS + " not found!!!");
		}
		if  (WebBrowser.isTextPresent(TITLE_MSG_HUB_NOT_FOUND)) {
			throw new IsmGuiTestException(hub + " not found!!!");
		}
	}
	
	@SuppressWarnings("deprecation")
	public static void addNewMessageHub(ImsCreateMessageHubTestData testData) throws Exception {
		MessageHubsPage.getGridIcon_MsgHubAdd().click();
		if (!WebBrowser.waitForText(MsgAddMessageHubDialogObject.DIALOG_DESC, 5)) {
			throw new IsmGuiTestException("'" + MsgAddMessageHubDialogObject.DIALOG_DESC + "' not found!!!");
		}
		MsgAddMessageHubDialogObject.getTextEntry_MessageHubID().setText(testData.getMessageHubName());
		
		
		if (testData.getMessageHubDescription() != null) {
			MsgAddMessageHubDialogObject.getTextEntry_MessageHubDesc().setText(testData.getMessageHubDescription());
		}
		WebBrowser.waitForReady();
		MsgAddMessageHubDialogObject.getButton_Save().click();		
		WebBrowser.waitForReady();
		
		
		try {	
			if (MsgAddMessageHubDialogObject.getErrorMessageTitle().isEnabled()) {
				String errorCode = MsgAddMessageHubDialogObject.getErrorCode().getText();
				if (errorCode != null) {
					MsgAddMessageHubDialogObject.getErrorCode().click();
					VisualReporter.logScreenCapture(errorCode , false);
					throw new IsmGuiTestException(errorCode);
				} else {
					VisualReporter.logScreenCapture("UNKNOWN" , false);
					throw new IsmGuiTestException("UNKNOWN");
				}
			}
		} catch (NoSuchElementException e) {
			// Save was successful
			if (testData.getA1Host() != null) {
				//Use CLI to verify that the Messaging Hub was created.
				try {
					ImaConfig imaConfig = null;
					imaConfig = new ImaConfig(testData.getA1Host(), testData.getUser(), testData.getPassword());
					imaConfig.connectToServer();
					if (imaConfig.messageHubExists(testData.getMessageHubName()) == false) {
						imaConfig.disconnectServer();
						throw new IsmGuiTestException("CLI verification: Messaging Policy " + testData.getMessageHubName() + " was not created.");
					}
					imaConfig.disconnectServer();
				} catch (Exception ex) {
					ex.printStackTrace();
					throw ex;
				}
			}
		}
	}
	
	public static void filterMessageHub(ImsCreateMessageHubTestData testData) throws Exception {
		MessageHubsPage.getGridTextBox_Filter().setText(testData.getMessageHubName());
		MessageHubsPage.getGridCell(0, 1).hover();
	}
	
	public static void editMessageHub(ImsEditMessageHubTestData testData) throws Exception {
		MessageHubsTasks.showMessageHub(testData, testData.getMessageHubName());
		MessageHubsPage.getLink_Edit().click();
		if  (!WebBrowser.waitForText(EditMessageHubDialogObject.TITLE_EDIT_MESSAGE_HUB, 10)) {
			throw new IsmGuiTestException(EditMessageHubDialogObject.TITLE_EDIT_MESSAGE_HUB + " not found!!!");
		}
		//Verify that the message hub name
		if (!EditMessageHubDialogObject.getTextEntry_MessageHubName().getText().equals(testData.getMessageHubName())) {
			throw new IsmGuiTestException("Message hub name " +  EditMessageHubDialogObject.getTextEntry_MessageHubName()
					+ " is not expected.  The expected message hub name is " + testData.getMessageHubName());
		}
		
		EditMessageHubDialogObject.getTextEntry_MessageHubDesc().setText(testData.getMessageHubDescription());
		EditMessageHubDialogObject.getButton_Save().click();
		
		//Verify the description has been changed
		MessageHubsPage.getLink_Edit().click();
		if (!EditMessageHubDialogObject.getTextEntry_MessageHubDesc().getText().equals(testData.getMessageHubDescription())) {
			throw new IsmGuiTestException("Failed verification of new message hub description.");
		}
		
	}
}

