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
package com.ibm.ism.test.testcases.appliance.systemcontrol;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.appliance.systemcontrol.SystemControlTasks;

public class TC_ChangeRunMode {
	
	public static void main(String[] args) {
		TC_ChangeRunMode test = new TC_ChangeRunMode();
		System.exit(test.runTest(new ImsSystemControlTestData(args)) ? 0 : 1);
	}

	public boolean runTest(ImsSystemControlTestData testData) {
		boolean result = false;
		String testDesc = "Run Mode";
		ImaConfig imaConfig = null;

		Platform.setEngineToSeleniumWebDriver();
		WebBrowser.start(testData.getBrowser());
		WebBrowser.loadUrl(testData.getURL());

		VisualReporter.startLogging(testData.getTestcaseAuthor(), testData.getTestcaseDescription(), testData.getTestArea());
		VisualReporter.logTestCaseStart(testData.getTestcaseDescription());

		try {
			LoginTasks.login(testData);
			SystemControlTasks.loadURL(testData);
			SystemControlTasks.setRunMode(testData);
			SystemControlTasks.stopMessagingServer(testData);
			try {
				imaConfig = new ImaConfig(testData.getA1Host(), testData.getUser(), testData.getPassword());
				imaConfig.connectToServer();
				System.out.println(imaConfig.executeOwnCommand("status imaserver"));
			} catch (Exception ex1) {
				ex1.printStackTrace();
			}
			SystemControlTasks.startMessagingServer(testData);
			if (imaConfig != null) {
				try {
					System.out.println(imaConfig.executeOwnCommand("status imaserver"));
					imaConfig.disconnectServer();
				} catch (Exception ex2) {
					ex2.printStackTrace();
				}
			}
			LoginTasks.logout(testData);
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
