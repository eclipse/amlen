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
package com.ibm.ism.test.testcases.ldap;

import java.util.Map;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.util.CLICommand;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.monitoring.connections.MonitoringConnectionsTasks;

/**
 *
 */
public class TC_UpdateLDAP {
	
	public static void main(String[] args) {
		TC_UpdateLDAP test = new TC_UpdateLDAP();
		System.exit(test.runTest(new ImsLDAPTestData(args)) ? 0 : 1);
	}

	public boolean runTest(ImsLDAPTestData testData) {
		boolean result = false;
		String testDesc = "Update LDAP";
		ImaConfig imaConfig = null;
		
		if (testData.isCLIUpdateLDAP()) {
			try {
				imaConfig = new ImaConfig(testData.getA1Host(), testData.getUser(), testData.getPassword());
				imaConfig.connectToServer();
				if (testData.isDebug()) {
					StringBuilder sb = new StringBuilder(256);
					sb.append("imaserver update LDAP");
					Map<String, String> options = testData.getLDAPCommandOptions();
					if (options != null) {
						for (Map.Entry<String, String> e : options.entrySet()) {
							sb.append(" \"" + e.getKey() + "=" + e.getValue() + "\"");
						}
					}
					System.out.println(sb.toString());
				}
				CLICommand cmd = imaConfig.updateLDAP(testData.getLDAPCommandOptions());
				if (testData.isDebug()) {
					System.out.println(testDesc + " rc=" + cmd.getRetCode());
					if (cmd.getStdoutList() != null) {
						for (String line : cmd.getStdoutList()) {
							System.out.println(line);
						}
					}
					if (cmd.getStderrList() != null) {
						for (String line : cmd.getStderrList()) {
							System.out.println(line);
						}
					}
				}
				imaConfig.disconnectServer();
				return (cmd.getRetCode() == 0) ? true : false;				
			} catch (Exception e) {
				System.out.println(testDesc + " FAILED: " + e);
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
			if (testData.isCLIUpdateLDAP()) {
				//TODO: Login the Web UI to verify that LDAP was updated
				//LoginTasks.login(testData.getUser(), testData.getPassword());
				//MonitoringConnectionsTasks.loadURL(testData);
				//LoginTasks.logout();
			} else {
				//TODO: update LDAP in the Web UI
				LoginTasks.login(testData);
				MonitoringConnectionsTasks.loadURL(testData);
				LoginTasks.logout();
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
