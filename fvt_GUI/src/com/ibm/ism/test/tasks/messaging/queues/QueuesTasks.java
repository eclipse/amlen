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
package com.ibm.ism.test.tasks.messaging.queues;


import org.openqa.selenium.NoSuchElementException;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.messaging.queues.MessageQueuesPageObjects;
import com.ibm.ism.test.appobjects.messaging.queues.AddQueueDialogObjects;
import com.ibm.ism.test.common.IsmGuiTestException;
import com.ibm.ism.test.common.IsmTestData;
import com.ibm.ism.test.testcases.messaging.queues.ImaMessageQueuesTestData;

public class QueuesTasks {

	private static final String HEADER_MESSAGE_QUEUES = "Message Queues";
	private static final String HEADER_TEXT = "Message Queues are used in point-to-point messaging";
	//private static final String TITLE_MSG_HUB_NOT_FOUND = "The message hub cannot be found. It might have been deleted by another user.";
	
	public static void verifyPageLoaded() throws Exception {
		WebBrowser.waitForReady();
		if  (!WebBrowser.waitForText(HEADER_MESSAGE_QUEUES, 10)) {
			throw new IsmGuiTestException(HEADER_MESSAGE_QUEUES + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_TEXT, 10)) {
			throw new IsmGuiTestException(HEADER_TEXT + " not found!!!");
		}
	}
	
	public static void loadURL(IsmTestData testData) throws Exception {
		WebBrowser.loadURL(testData.getURL() + "/messaging.jsp?nav=messagequeues");
		Platform.sleep(3);
		verifyPageLoaded();
	}
	
	@SuppressWarnings("deprecation")
	public static void addNewMessageQueue(ImaMessageQueuesTestData testData) throws Exception {
		MessageQueuesPageObjects.getGridIcon_Add().click();
		if (!WebBrowser.waitForText(AddQueueDialogObjects.DIALOG_DESC, 5)) {
			throw new IsmGuiTestException("'" + AddQueueDialogObjects.DIALOG_DESC + "' not found!!!");
		}

		AddQueueDialogObjects.getTextEntry_QueueName().setText(testData.getMessageQueueName());

		if (testData.getNewMessageQueueDescription() != null) {
			AddQueueDialogObjects.getTextEntry_QueueDescription().setText(testData.getNewMessageQueueDescription());
		}
		
		if (testData.isNewMessageQueueAllowSend() == false) {
			AddQueueDialogObjects.getCheckBox_QueueAllowSend().click();
		}
		
		if (testData.getNewMessageQueueMaxMessages() != null) {
			AddQueueDialogObjects.getTextEntry_MaxMessages().setText(testData.getNewMessageQueueMaxMessages());
		}
		
		if (testData.isNewMessageQueueAllowConcurentConsumers() == false) {
			AddQueueDialogObjects.getCheckBox_QueueConcurrentConsumers().click();
		}
		
		WebBrowser.waitForReady();
		AddQueueDialogObjects.getButton_Save().click();
		WebBrowser.waitForReady();
		
		try {
			if (AddQueueDialogObjects.getErrorMessageTitle().isEnabled()) {
				String errorCode = AddQueueDialogObjects.getErrorCode().getText();
				if (errorCode != null) {
					AddQueueDialogObjects.getErrorCode().click();
					VisualReporter.logScreenCapture(errorCode , false);
					throw new IsmGuiTestException(errorCode);
				} else {
					VisualReporter.logScreenCapture("UNKNOWN" , false);
					throw new IsmGuiTestException("UNKNOWN");
				}
			}
		} catch (NoSuchElementException e) {
			// No error
		}
	}
	
	public static void filterMessageQueue(ImaMessageQueuesTestData testData) throws Exception {
		MessageQueuesPageObjects.getGridTextBox_Filter().setText(testData.getMessageQueueName());
		MessageQueuesPageObjects.getGridCell(0, 1).hover();
	}
}

