
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
package com.ibm.ism.test.testcases.messaging.connectionpolicies;


import com.ibm.automation.core.Platform;

//import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.common.CLIHelper;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.messaging.connectionpolicies.*;
public class TC_CreateConnectionPolicy {
	public static void main(String[] args) {
		TC_CreateConnectionPolicy test = new TC_CreateConnectionPolicy();
		System.exit(test.runTest(new ImsCreateConnectionPoliciesTestData(args)) ? 0 : 1);
	}
	
	public boolean runTest(ImsCreateConnectionPoliciesTestData testData) {
		boolean result = false;
		String testDesc = "Create Connection Policy";

		Platform.setEngineToSeleniumWebDriver();
		WebBrowser.start(testData.getBrowser());
		WebBrowser.loadUrl(testData.getURL());		

//		VisualReporter.startLogging("Sheila McDonald-Rich", "IMS Create Connection Policy", "IMS FVT");
//		VisualReporter.logTestCaseStart(testDesc);

		try {
			CLIHelper.printConnectionPolicies(testData);
			LoginTasks.login(testData);
			ConnectionPoliciesTasks.loadURL(testData);
			ConnectionPoliciesTasks.addNewConnectionPolicy(testData);
			LoginTasks.logout(testData);
			CLIHelper.printConnectionPolicy(testData, testData.getConnectionPolicyName());
			result = true;
		} catch (Exception e){
//			VisualReporter.failTest(testDesc + " failed.", e);
			result = false;
		}
		WebBrowser.shutdown();
//		VisualReporter.stopLogging();
		return result;
	}
}

