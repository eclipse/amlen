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
	package com.ibm.ism.test.testcases.messaging.endpoints;


	import com.ibm.automation.core.Platform;

//	import com.ibm.automation.core.loggers.VisualReporter;
	import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.common.CLIHelper;
	import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.messaging.endpoints.*;

	public class TC_CreateEndpoint {
		public static void main(String[] args) {
			TC_CreateEndpoint test = new TC_CreateEndpoint();
			System.exit(test.runTest(new ImsCreateEndpointsTestData(args)) ? 0 : 1);
		}
		
		public boolean runTest(ImsCreateEndpointsTestData testData) {
			boolean result = false;

			Platform.setEngineToSeleniumWebDriver();
			WebBrowser.start(testData.getBrowser());
			WebBrowser.loadUrl(testData.getURL());		

//			VisualReporter.startLogging("Sheila McDonald-Rich", "IMS Create Endpoint", "IMS FVT");
//			VisualReporter.logTestCaseStart(testData.getTestcaseDescription());

			try {
				CLIHelper.printEndpoints(testData);
				LoginTasks.login(testData);
				EndpointsTasks.loadURL(testData);
				EndpointsTasks.addNewEndpoints(testData);
				LoginTasks.logout(testData);
				CLIHelper.printEndpoint(testData, testData.getEndpointName());
				result = true;
			} catch (Exception e){
//				VisualReporter.failTest(testData.getTestcaseDescription() + " failed.", e);
				result = false;
			}
			WebBrowser.shutdown();
//			VisualReporter.stopLogging();
			return result;
		}
	}



