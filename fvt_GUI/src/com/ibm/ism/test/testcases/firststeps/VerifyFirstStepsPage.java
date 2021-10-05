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
import com.ibm.ism.test.common.IsmTest;
import com.ibm.ism.test.tasks.LoginTasks;
//import com.ibm.ism.test.tasks.NavigateTabTasks;
//import com.ibm.ism.test.tasks.firststeps.FirstStepsTasks;

/**
 *
 */
public class VerifyFirstStepsPage {
/*	static String url = "http://10.10.3.175:9080/ISMWebUI"; */
	static String url = "https://9.3.177.96:9087";
	static String user = "admin";
	static String pwd = "admin";
	static String browser = Platform.gsMozillaFirefox;

	/**
	 * @param args Valid arguments are URL= USER= and PASSWORD=
	 */
	public static void main(String[] args) {
		
		VisualReporter.startLogging("Sheila McDonald-Rich", "ISM FirstSteps", "ISM FVT");
		
		if (args.length > 0) {
			String strs[] = args[0].split("=");
			if (strs[0].equalsIgnoreCase("URL")) {
				url = strs[1];
			} else if (strs[0].equalsIgnoreCase("USER")) {
				user = strs[1];
			} else if (strs[0].equalsIgnoreCase("PASSWORD")) {
				pwd = strs[1];
			} else if (strs[0].equalsIgnoreCase("BROWSER")) {
				browser = strs[1];
			}
		}
		
		
		VisualReporter.logTestCaseStart("ISM FirstSteps");
		
		try {
			WebBrowser.start(browser);
			WebBrowser.loadUrl(url);
			IsmTest.waitForBrowserReady();

			//Log in as Appliance Admin
			IsmTest.gsUserId = "admin";
			IsmTest.gsPassword = "admin";
			IsmTest.gsUserRole = IsmTest.APPLIANCE_ADMIN_ROLE;
			LoginTasks.login(IsmTest.gsUserId, IsmTest.gsPassword);
			//if (FirstStepsTasks.verifyPageLoaded() == false) {
			//	throw new Exception("Log in failed");
			//}
			//LoginTasks.logout();

		} catch (Exception e) {
			VisualReporter.failTest("Unexpected failure", e);
		}

		WebBrowser.shutdown();
		VisualReporter.stopLogging();
		System.exit(0);
	}


}
