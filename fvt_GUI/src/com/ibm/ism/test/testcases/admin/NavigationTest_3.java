// Copyright (c) 2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
package com.ibm.ism.test.testcases.admin;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.common.IsmTest;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.NavigateAllTabTasks2;
//import com.ibm.ism.test.appobjects.firststeps.FirstStepsDialogTask;


/**
 * This is a simple navigation test of the ISM administration GUI.
 * 
 *
 */
public class NavigationTest_3 {
	static String url = "http://10.10.3.80:9080/ISMWebUI";
	static String user = "admin";
	static String pwd = "admin";

	/**
	 * @param args Valid arguments are URL= USER= and PASSWORD=
	 */
	public static void main(String[] args) {
		
		VisualReporter.startLogging("Sheila McDonald-Rich", "ISM Navigation Test2", "ISM FVT");
		
		if (args.length > 0) {
			String strs[] = args[0].split("=");
			if (strs[0].equalsIgnoreCase("URL")) {
				url = strs[1];
			} else if (strs[0].equalsIgnoreCase("USER")) {
				user = strs[1];
			} else if (strs[0].equalsIgnoreCase("PASSWORD")) {
				pwd = strs[1];
			}
		}
		
		
		VisualReporter.logTestCaseStart("ISM Navigation Test2");
		
		try {
			WebBrowser.start(Platform.gsMozillaFirefox);
			WebBrowser.loadUrl(url);
			IsmTest.waitForBrowserReady();

			//Log in as Appliance Admin
			IsmTest.gsUserId = "admin";
			IsmTest.gsPassword = "admin";
			IsmTest.gsUserRole = IsmTest.APPLIANCE_ADMIN_ROLE;
			LoginTasks.login(IsmTest.gsUserId, IsmTest.gsPassword);
			//NavigateTabTasks.navigateWelcome("Welcome to IBM Internet Scale Messaging");
//			if (FirstStepsDialogTask.verifyFirstStepsDialog() == false) {
//				throw new Exception("Not First Steps Dialog1");
//			}
			NavigateAllTabTasks2.navigateHome("Welcome to IBM Internet Scale Messaging");
//			if (FirstStepsDialogTask.verifyFirstStepsDialog() == false) {
//	  		throw new Exception("Not First Steps Dialog2");
//			}
//			FirstStepsDialogTask.leaveFirstSteps();
			NavigateAllTabTasks2.navigateAppliance(null);
//			if (FirstStepsDialogTask.verifyFirstStepsDialog() == false) {
//				throw new Exception("Not First Steps Dialog3");
//			}
//			FirstStepsDialogTask.leaveFirstSteps();
			NavigateAllTabTasks2.navigateMonitoring(null);
			NavigateAllTabTasks2.navigateMessaging(null);
//			LoginTasks.logout();

			// Log in as Observer
//			IsmTest.gsUserId = "Observer";
//			IsmTest.gsPassword = "passw0rd";
//  		IsmTest.gsUserRole = IsmTest.NORMAL_USER_ROLE;
//			LoginTasks.login(IsmTest.gsUserId, IsmTest.gsPassword);
//			NavigateAllTabTasks.navigateMonitoring(null);
//			LoginTasks.logout();
//			IsmTest.delay();
		
		} catch (Exception e) {
			VisualReporter.failTest("Unexpected failure", e);
		}

		WebBrowser.shutdown();
		VisualReporter.stopLogging();
		System.exit(0);
	}
}

