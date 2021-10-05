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
package com.ibm.ism.test.tasks.appliance.systemcontrol;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.appliance.systemcontrol.ConfirmRunmodeChangeDialogObjects;
import com.ibm.ism.test.appobjects.appliance.systemcontrol.EditNodeNameDialogObjects;
import com.ibm.ism.test.appobjects.appliance.systemcontrol.StopMessagingServerDialogObjects;
import com.ibm.ism.test.appobjects.appliance.systemcontrol.SystemControlPageObjects;
import com.ibm.ism.test.common.IsmGuiTestException;
import com.ibm.ism.test.common.IsmTestData;
import com.ibm.ism.test.testcases.appliance.systemcontrol.ImsSystemControlTestData;

public class SystemControlTasks {
	
	private static final String HEADER_SYSTEM_CONTROL = "System Control";
	private static final String HEADER_APPLIANCE = "Appliance";
	private static final String HEADER_MESSAGESIGHT_SERVER = "IBM MessageSight Server";
	private static final String RUN_MODE_PRODUCTION = "production";
	private static final String STATUS_AFTER_RESTART_RUN_MODE_PRODUCTION = "Running (production)";
	private static final String RUN_MODE_MAINTENANCE = "maintenance";
	private static final String STATUS_AFTER_RESTART_RUN_MODE_MAINTENANCE = "Running (maintenance)";
	private static final String RUN_MODE_CLEAN_STORE = "clean_store";
	private static final String STATUS_AFTER_RESTART_RUN_MODE_CLEAN_STORE = "Running (maintenance)";
	private static final String STATUS_MESSAGING_SERVER_STOPPED = "Stopped";

	public static void verifyPageLoaded() throws Exception {
		WebBrowser.waitForReady();
		if  (!WebBrowser.waitForText(HEADER_SYSTEM_CONTROL, 10)) {
			throw new IsmGuiTestException(HEADER_SYSTEM_CONTROL + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_APPLIANCE, 10)) {
			throw new IsmGuiTestException(HEADER_APPLIANCE + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_MESSAGESIGHT_SERVER, 10)) {
			throw new IsmGuiTestException(HEADER_MESSAGESIGHT_SERVER + " not found!!!");
		}
	}
	
	public static void loadURL(IsmTestData testData) throws Exception {
		WebBrowser.loadURL(testData.getURL() + "/appliance.html?nav=systemControl");
		verifyPageLoaded();
	}
	
	public static void setNodeName(ImsSystemControlTestData testData) throws Exception {
		SystemControlPageObjects.getLink_EditHostName().click();
		if (!WebBrowser.waitForText(EditNodeNameDialogObjects.DIALOG_DESC, 5)) {
			throw new IsmGuiTestException("'" + EditNodeNameDialogObjects.DIALOG_DESC + "' not found!!!");
		}
		EditNodeNameDialogObjects.getTextEntry_NodeName().setText(testData.getNewNodeName());
		EditNodeNameDialogObjects.getButton_Save().click();
		if (!WebBrowser.waitForText(testData.getNewNodeName(), 5)) {
			throw new IsmGuiTestException("'" + testData.getNewNodeName() + "' not found!!!");
		}
		String nodename = SystemControlPageObjects.getWebElement_HostName().getText();
		if (!nodename.equals(testData.getNewNodeName())) {
			throw new IsmGuiTestException("Node name is '" + nodename + "' while '" + testData.getNewNodeName() + "' is expected.");
		}
	}
	
	public static void setRunMode(ImsSystemControlTestData testData) throws Exception {
		SystemControlPageObjects.getDownArrow_SelectRunmodeControl().click();
		Platform.sleep(2);
		if (testData.getNewRunMode().equals(RUN_MODE_PRODUCTION)) {
			SystemControlPageObjects.getMenuItem_RunmodeProduction().click();
			if (testData.getAfterRestartStatus() == null) {
				testData.setAfterRestartStatus(STATUS_AFTER_RESTART_RUN_MODE_PRODUCTION);
			}
		} else if (testData.getNewRunMode().equals(RUN_MODE_MAINTENANCE)) {
			SystemControlPageObjects.getMenuItem_RunmodeMaintenance().click();
			if (testData.getAfterRestartStatus() == null) {
				testData.setAfterRestartStatus(STATUS_AFTER_RESTART_RUN_MODE_MAINTENANCE);
			}
		} else if (testData.getNewRunMode().equals(RUN_MODE_CLEAN_STORE)) {
			SystemControlPageObjects.getMenuItem_RunmodeCleanStore().click();
			if (testData.getAfterRestartStatus() == null) {
				testData.setAfterRestartStatus(STATUS_AFTER_RESTART_RUN_MODE_CLEAN_STORE);
			}
		} else {
			throw new IsmGuiTestException("Test case error, new run mode '" + testData.getNewRunMode() + "' is invalid");
		}
		if (!WebBrowser.waitForText(ConfirmRunmodeChangeDialogObjects.DIALOG_TITLE, 5)) {
			throw new IsmGuiTestException("'" + ConfirmRunmodeChangeDialogObjects.DIALOG_TITLE + "' not found!!!");
		}
		Platform.sleep(2);
		ConfirmRunmodeChangeDialogObjects.getButton_Confirm().click();
		Platform.sleep(2);
	}
	
	public static void stopMessagingServer(ImsSystemControlTestData testData) throws Exception {
		SystemControlPageObjects.getLink_StopMessagingServer().click();
		if (!WebBrowser.waitForText(StopMessagingServerDialogObjects.DIALOG_DESC, 5)) {
			throw new IsmGuiTestException("'" + StopMessagingServerDialogObjects.DIALOG_DESC + "' not found!!!");
		}
		StopMessagingServerDialogObjects.getButton_Stop().click();

		// Wait for Status text 
		int i = 0;
		boolean pass = false;
		String status = null;
		while (!pass && i < 60) {
			status = SystemControlPageObjects.getWebElement_MessagingServerStatus().getText();
			if (status.equals(STATUS_MESSAGING_SERVER_STOPPED)) {
				pass = true;
				break;
			} else {
				i++;
				Platform.sleep(1);
			}
		}
		if (!pass) {
			throw new IsmGuiTestException("After stop status is '" + status + "' while '" + STATUS_MESSAGING_SERVER_STOPPED + "' is expected.");
		}
		Platform.sleep(2);
	}
	
	public static void startMessagingServer(ImsSystemControlTestData testData) throws Exception {
		SystemControlPageObjects.getLink_StartMessagingServer().click();
		SystemControlPageObjects.getLink_StopMessagingServer().waitForExistence(15);

		// Wait for Status text
		int i = 0;
		boolean pass = false;
		String status = null;
		while (!pass && i < 60) {
			status = SystemControlPageObjects.getWebElement_MessagingServerStatus().getText();
			if (status.equals(testData.getAfterRestartStatus())) {
				pass = true;
				break;
			} else {
				i++;
				Platform.sleep(1);
			}
		}
		if (!pass) {
			throw new IsmGuiTestException("After restart status is '" + status + "' while '" + testData.getAfterRestartStatus() + "' is expected.");
		}
		Platform.sleep(2);
	}	
}
