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
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.messaging.messagehubs.MessageHubsTasks;

public class TC_EditMessageHub {
	public static void main(String[] args) {
		TC_EditMessageHub test = new TC_EditMessageHub();
		System.exit(test.runTest(new ImsEditMessageHubTestData(args)) ? 0 : 1);
	}
	
	public boolean runTest(ImsEditMessageHubTestData testData) {
		boolean result = false;
		String testDesc = "Edit Message Hub";

		Platform.setEngineToSeleniumWebDriver();
		WebBrowser.start(testData.getBrowser());
		WebBrowser.loadUrl(testData.getURL());
		WebBrowser.maximize();

		VisualReporter.startLogging(testData.getTestcaseAuthor(), testData.getTestcaseDescription(), testData.getTestArea());
		VisualReporter.logTestCaseStart(testData.getTestcaseDescription());

		try {
			LoginTasks.login(testData);
			MessageHubsTasks.editMessageHub(testData);
			LoginTasks.logout(testData);
			if (testData.isGB18030G2() == false) {
				CLIHelper.printMessageHub(testData, testData.getMessageHubName());
			}
			result = true;
		} catch (Exception e){
			VisualReporter.failTest(testDesc + " failed.", e);
			result = false;
		}
		WebBrowser.shutdown();
		VisualReporter.stopLogging();
		return result;
	}
}

