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
package com.ibm.ism.test.testcases.cluster.membership;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.cluster.membership.ClusterMembershipTasks;

public class TC_EditClusterMembership {

	public static void main(String[] args) {
		TC_EditClusterMembership test = new  TC_EditClusterMembership();
		System.exit(test.runTest(new IsmClusterMembershipData(args)) ? 0 : 1);
	}
	
	public boolean runTest(IsmClusterMembershipData testData) {
		boolean result = false;
		String testDesc = "Edit Cluster Membership";

		Platform.setEngineToSeleniumWebDriver();
		WebBrowser.start(testData.getBrowser());
		WebBrowser.loadUrl(testData.getURL());

		VisualReporter.startLogging("Some Tester", "ISM Edit Cluster Membership", "ISM FVT");
		VisualReporter.logTestCaseStart(testData.getTestcaseDescription());

		try {
			LoginTasks.login(testData);
			ClusterMembershipTasks.loadURL(testData);
			ClusterMembershipTasks.editClusterMembershipConfiguration(testData);
			ClusterMembershipTasks.verifyClusterMembershipConfiguration(testData);
			ClusterMembershipTasks.joinCluster();
			ClusterMembershipTasks.leaveCluster();
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
