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
package com.ibm.ism.test.testcases.appliance.users;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ism.test.common.CLIHelper;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.appliance.users.WebUIUserTasks;
import com.ibm.ism.test.tasks.monitoring.connections.MonitoringConnectionsTasks;

public class TC_CreateUser {
	public static void main(String[] args) {
		TC_CreateUser test = new TC_CreateUser();
		System.exit(test.runTest(new ImsCreateUserTestData(args)) ? 0 : 1);
	}
	
	public boolean runTest(ImsCreateUserTestData testData) {
		boolean result = false;
		String testDesc = "Create User";
		ImaConfig imaConfig = null;
		
		
		if (testData.isCLICreateUserTest()) {
			try {
				CLIHelper.printWebUIHost(testData);
				CLIHelper.printWebUIPort(testData);
				imaConfig = new ImaConfig(testData.getA1Host(), testData.getUser(), testData.getPassword());
				imaConfig.connectToServer();
				imaConfig.createUser(testData.getNewUserID(), testData.getNewUserPwd(), ImaConfig.USER_GROUP_TYPE.WebUI, testData.getNewUserGroups(), testData.getNewUserDescription());
				imaConfig.disconnectServer();
			} catch (Exception e) {
				e.printStackTrace();
				return false;
			}
		}

		Platform.setEngineToSeleniumWebDriver();
		WebBrowser.start(testData.getBrowser());
		WebBrowser.loadUrl(testData.getURL());

		VisualReporter.startLogging(testData.getTestcaseAuthor(), testData.getTestcaseDescription(), testData.getTestArea());
		VisualReporter.logTestCaseStart(testData.getTestcaseDescription());

		try {
			if (testData.isCLICreateUserTest()) {
				LoginTasks.login(testData);
				MonitoringConnectionsTasks.loadURL(testData);
				LoginTasks.logout();
			} else {
				LoginTasks.login(testData);
				WebUIUserTasks.loadURL(testData);
				WebUIUserTasks.addNewUser(testData);
				LoginTasks.logout(testData);
				if (testData.getNewUserGroupMembership().contains(ImsCreateUserTestData.GROUP_SYSTEM_ADMINISTRATORS)) {
					try {
						imaConfig = new ImaConfig(testData.getA1Host(), testData.getNewUserID(), testData.getNewUserPwd());
						imaConfig.connectToServer();
						String response = imaConfig.executeOwnCommand("status imaserver");
						System.out.println("status imaserver: " + response);
						VisualReporter.logScriptInfo("\"status imaserver\" output:" + response, 6);
						imaConfig.disconnectServer();
					} catch (Exception e) {
						e.printStackTrace();
						if (imaConfig != null && imaConfig.isConnected()) {
							imaConfig.disconnectServer();
						}
					}
				}
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
