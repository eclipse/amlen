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
package com.ibm.ism.test.testcases.messaging.messagehubs;


import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.common.CLIHelper;
import com.ibm.ism.test.common.IsmGuiTestException;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.messaging.messagehubs.MessageHubsTasks;

public class TC_CreateMsgHub {
	public static void main(String[] args) {
		TC_CreateMsgHub test = new TC_CreateMsgHub();
		System.exit(test.runTest(new ImsCreateMessageHubTestData(args)) ? 0 : 1);
	}
	
	public boolean runTest(ImsCreateMessageHubTestData testData) {
		boolean result = false;
		String testDesc = "Create Message Hub";

		Platform.setEngineToSeleniumWebDriver();
		WebBrowser.start(testData.getBrowser());
		WebBrowser.loadUrl(testData.getURL());
		WebBrowser.maximize();

		VisualReporter.startLogging(testData.getTestcaseAuthor(), testData.getTestcaseDescription(), testData.getTestArea());
		VisualReporter.logTestCaseStart(testData.getTestcaseDescription());


		try {
			LoginTasks.login(testData);
			MessageHubsTasks.loadURL(testData);
			MessageHubsTasks.addNewMessageHub(testData);
			MessageHubsTasks.showMessageHub(testData, testData.getMessageHubName());
			if (testData.isGB18030G2() == false) {
				CLIHelper.printMessageHub(testData, testData.getMessageHubName());
			}
			LoginTasks.logout(testData);
			result = true;
		} catch (IsmGuiTestException ismException) {
			String hubName = testData.getMessageHubName();
			if (testData.getMaxConfigItemNameLength() < hubName.codePointCount(0, hubName.length())) {
				if (ismException.getMessage().equals(testData.getErrorCWLNA5013())) {
					result = true;
				}
			}
			if (result == false) {
				VisualReporter.failTest(testDesc + " failed.", ismException);
			}
		} catch (Exception e){
			VisualReporter.failTest(testDesc + " failed.", e);
		}
		WebBrowser.shutdown();
		VisualReporter.stopLogging();
		return result;
	}
}

