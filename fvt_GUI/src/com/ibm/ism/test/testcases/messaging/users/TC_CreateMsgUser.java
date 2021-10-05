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
package com.ibm.ism.test.testcases.messaging.users;


import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.messaging.users.MsgUserTasks;

public class TC_CreateMsgUser {
	public static void main(String[] args) {
		TC_CreateMsgUser test = new TC_CreateMsgUser();
		System.exit(test.runTest(new ImsCreateUserTestData(args)) ? 0 : 1);
	}
	
	public boolean runTest(ImsCreateUserTestData testData) {
		boolean result = false;
		String testDesc = "Create Messaging User";

		Platform.setEngineToSeleniumWebDriver();
		WebBrowser.start(testData.getBrowser());
		WebBrowser.loadUrl(testData.getURL());		

		VisualReporter.startLogging("Sheila McDonald-Rich", "IMS Create Messaging User", "IMS FVT");
		VisualReporter.logTestCaseStart(testDesc);

		try {
			LoginTasks.login(testData);
			MsgUserTasks.loadURL(testData);
			MsgUserTasks.addNewUser(testData);
			//LoginTasks.logout(testData);
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
