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
package com.ibm.ism.test.testcases.license;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ima.test.cli.util.CLICommand;
import com.ibm.ima.test.cli.util.SSHExec;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.firststeps.FirstStepsTasks;
import com.ibm.ism.test.tasks.license.LicenseAgreementTasks;

public class TC_AcceptLicense {

	/**
	 * @param args Test arguments
	 */
	public static void main(String[] args) {
		TC_AcceptLicense test = new  TC_AcceptLicense();
		System.exit(test.runTest(new IsmLicenseTestData(args)) ? 0 : 1);
	}
	
	public boolean runTest(IsmLicenseTestData testData) {
		boolean result = false;
		String testDesc = "Accept License";
		//@TODO: Move the following code block to its own class
		try {
			/*
			ImaServerStatus imaStatusHelper = new ImaServerStatus(testData.getA1Host(), testData.getUser(), testData.getPassword());
			imaStatusHelper.connectToServer();
			String status = imaStatusHelper.showStatus();
			*/
			SSHExec sshExec = new SSHExec(testData.getA1Host(), testData.getUser(), testData.getPassword());
			sshExec.connect();
			CLICommand cmd = sshExec.exec("status imaserver");
			System.out.println(cmd.getCommand() + " returned RC=" + cmd.getRetCode());
			if (cmd.getStdoutList() != null) {
				System.out.println("stdout:");
				for (String line : cmd.getStdoutList()) {
					System.out.println(line);
				}
			} else {
				System.out.println("  stdout: \"" + cmd.getStdout() + "\"");
			}
			if (cmd.getStderrList() != null) {
				System.out.println("stderr:");
				for (String line : cmd.getStderrList()) {
					System.out.println(line);
				}
			} else {
				System.out.println("  stderr: \"" + cmd.getStderr() + "\"");
			}
			if (cmd.getRetCode() == 0) {
				System.out.println("RC=" + cmd.getRetCode() + " means that the License has been accepted.");
				sshExec.disconnect();
				if (cmd.getStderr().length() > 0) {
					System.err.println("Warning: this is a bug.");
					System.err.println("  stderr: \"" + cmd.getStderr() + "\"");
				} else {
					return true;
				}
			} else {
				if (cmd.getRetCode() != 1 || !cmd.getStderr().contains(LicenseAgreementTasks.CLI_TEXT_LICENSE_NOT_YET_ACCEPTED)) {
					System.out.println("RC=" + cmd.getRetCode());
					System.out.println("Expected RC=1");
					System.out.println("The stderr of status command: \"" + cmd.getStderr() + "\"");
					System.out.println("Expected stderr: \"" + LicenseAgreementTasks.CLI_TEXT_LICENSE_NOT_YET_ACCEPTED + "\"");
					sshExec.disconnect();
					return false;
				}
			}
			sshExec.disconnect();
		} catch (Exception e) {
			System.out.println(e.getMessage());
			return false;
		}
		
		Platform.setEngineToSeleniumWebDriver();
		WebBrowser.start(testData.getBrowser());
		WebBrowser.loadUrl(testData.getURL());
		
		VisualReporter.startLogging(testData.getTestcaseAuthor(), testData.getTestcaseDescription(), testData.getTestArea());
		VisualReporter.logTestCaseStart(testDesc);

		try {
			LoginTasks.login(testData);
			LicenseAgreementTasks.verifyPageLoaded(testData);
			LicenseAgreementTasks.acceptLicense();
			FirstStepsTasks.verifyPageLoaded();
			LoginTasks.logout();
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
