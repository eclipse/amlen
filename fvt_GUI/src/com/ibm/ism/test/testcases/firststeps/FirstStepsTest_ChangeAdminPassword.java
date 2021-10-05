/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
/**
 * 
 */
package com.ibm.ism.test.testcases.firststeps;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.common.IsmTestData;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.firststeps.FirstStepsTasks;

/**
 *
 */
public class FirstStepsTest_ChangeAdminPassword {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		FirstStepsTest_ChangeAdminPassword test = new FirstStepsTest_ChangeAdminPassword();
		System.exit(test.runTest(new IsmTestData(args)) ? 0 : 1);
	}
	
	public boolean runTest(IsmTestData testData) {
		Platform.setEngineToSeleniumWebDriver();
		boolean result = false;
		String testDesc = "ISM First Steps Change Admin Password";
		
		VisualReporter.startLogging("Mike Tran", "ISM First Steps Change Admin Password", "ISM FVT");
		VisualReporter.logTestCaseStart(testDesc);
		
		try {
			WebBrowser.start(testData.getBrowser());
			WebBrowser.loadUrl(testData.getURL());
			if (!LoginTasks.login(testData.getUser(), testData.getPassword())) {
				throw new Exception("Login failed for " + testData.getUser() + "/" + testData.getPassword());
			}
			FirstStepsTasks.verifyPageLoaded();
			result = FirstStepsTasks.changeAdminPassword(testData.getProperty("ADMIN_NEW_PASSWORD"), "Saving...Succeeded");
			if (!result) {
				throw new Exception(testDesc + " failed.");
			}
			
			if (!LoginTasks.logout()) {
				throw new Exception("Logout failed for " + testData.getUser());
			}
			
			WebBrowser.loadUrl(testData.getURL());
			if (!LoginTasks.login(testData.getUser(), testData.getProperty("ADMIN_NEW_PASSWORD"))) {
				throw new Exception("Login failed for " + testData.getUser() + "/" + testData.getProperty("ADMIN_NEW_PASSWORD"));
			}
			
			if (!LoginTasks.logout()) {
				throw new Exception("Logout failed for " + testData.getUser());
			}
			
		} catch (Exception e) {
			VisualReporter.failTest("Unexpected failure", e);
		}
		WebBrowser.shutdown();
		VisualReporter.stopLogging();
		return result;
	}	

}
