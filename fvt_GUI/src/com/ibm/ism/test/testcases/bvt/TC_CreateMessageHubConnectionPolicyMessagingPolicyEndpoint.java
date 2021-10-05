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
package com.ibm.ism.test.testcases.bvt;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.common.CLIHelper;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.messaging.connectionpolicies.ConnectionPoliciesTasks;
import com.ibm.ism.test.tasks.messaging.endpoints.EndpointsTasks;
import com.ibm.ism.test.tasks.messaging.messagehubs.MessageHubsTasks;
import com.ibm.ism.test.tasks.messaging.messagingpolicies.MessagingPoliciesTasks;

public class TC_CreateMessageHubConnectionPolicyMessagingPolicyEndpoint {

	public static void main(String[] args) {
		TC_CreateMessageHubConnectionPolicyMessagingPolicyEndpoint test = new TC_CreateMessageHubConnectionPolicyMessagingPolicyEndpoint();
		System.exit(test.runTest(new ImsBVTTestData(args)) ? 0 : 1);

	}

	public boolean runTest(ImsBVTTestData testData) {
		boolean result = false;
		String testDesc = "BVT Create Message Hub, Connection Policy, Messaging Policy and Endpoint";
		Platform.setEngineToSeleniumWebDriver();
		WebBrowser.start(testData.getBrowser());
		WebBrowser.loadUrl(testData.getURL());
		WebBrowser.maximize();

		VisualReporter.startLogging(testData.getTestcaseAuthor(), testData.getTestcaseDescription(), testData.getTestArea());
		VisualReporter.logTestCaseStart(testData.getTestcaseDescription());
		
		try {
			LoginTasks.login(testData);
			MessageHubsTasks.loadURL(testData);
			MessageHubsTasks.addNewMessageHub(testData.getMessageHubTestData());
			MessageHubsTasks.showMessageHub(testData, testData.getMessageHubTestData().getMessageHubName());
			ConnectionPoliciesTasks.addNewConnectionPolicy(testData.getConnectionPolicyTestData());
			MessagingPoliciesTasks.addNewMessagingPolicy(testData.getMessagingPolicyTestData());
			EndpointsTasks.addNewEndpoints(testData.getEndpointTestData());
			LoginTasks.logout(testData);
			CLIHelper.printMessageHub(testData, testData.getMessageHubTestData().getMessageHubName());
			CLIHelper.printEndpoint(testData, testData.getEndpointTestData().getEndpointName());
			CLIHelper.printConnectionPolicy(testData, testData.getConnectionPolicyTestData().getConnectionPolicyName());
			CLIHelper.printMessagingPolicy(testData, testData.getMessagingPolicyTestData().getNewMessagingPoliciesID());
			result = true;
		} catch (Exception e) {
			VisualReporter.failTest(testDesc + " failed.", e);
			result = false;
		}
		WebBrowser.shutdown();
		VisualReporter.stopLogging();
		return result;
	}

}
