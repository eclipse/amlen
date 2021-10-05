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
//package com.ibm.ism.test.tasks.firststeps;
package com.ibm.ism.test.tasks;

import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.firststeps.FirstStepsObjects;
import com.ibm.ism.test.common.IsmTest;

public class FirstStepsTasks {
	
	//private static final String PAGE_HEADER = "Get started with IBM Internet Scale Messaging"; 
	private static final String PAGE_HEADER = "Get started with IBM MessageSight";
	private static final String CHANGE_PASSWORD_HEADER = "Change the default password for the \"admin\" user ID";
	private static final String CONFIGURE_ETHERNET_HEADER = "Configure an initial Ethernet interface for client connections";
	//private static final String RUN_VERIFICATION_HEADER = "Run Messaging Verification Application";
	private static final String COMPLETE_FIRST_STEPS = "Complete First Steps";
	
	public static boolean verifyPageLoaded() {
		WebBrowser.waitForReady();
		//return WebBrowser.isTextPresent("Get started with IBM Internet Scale Messaging");
		if (WebBrowser.waitForText(PAGE_HEADER, 30) == false) {
			return false;
		}
		if (WebBrowser.waitForText(CHANGE_PASSWORD_HEADER, 30) == false) {
			return false;
		}
		if (WebBrowser.waitForText(CONFIGURE_ETHERNET_HEADER, 30) == false) {
			return false;
		}
		//if (WebBrowser.waitForText(RUN_VERIFICATION_HEADER, 30) == false) {
		if (WebBrowser.waitForText(COMPLETE_FIRST_STEPS, 30) == false) {	
			return false;
		}
		return true;
	}
	
	
	public static boolean changeAdminPassword(String sNewPassword, String sExpectedOutput) {
		WebBrowser.waitForReady();
		try {
			if (!WebBrowser.waitForText(CHANGE_PASSWORD_HEADER, 30)) {
				return false;
			}
			FirstStepsObjects.getTextEntry_NewPassword().setText(sNewPassword);
			IsmTest.delay();
			FirstStepsObjects.getTextEntry_ConfirmPassword().setText(sNewPassword);
			IsmTest.delay();
			FirstStepsObjects.getButton_SavePassword().click();
			if (sExpectedOutput != null) {
				IsmTest.delay(3000);
				//String sOutput = FirstStepsObjects.getText_ChangePasswordOutput().getProperty(SeleniumTestObject.constants.gsValueProp);
				//if (sOutput.contains(sExpectedOutput)) {
				//	return true;
				//} else {
				//	VisualReporter.logDebugInfo("changeAdminPassword(): Actual output text: \"" 
				//			+ sOutput + "\" Expected output: \"" + sExpectedOutput + "\"");
				//	return false;
				//}
				return WebBrowser.waitForText(sExpectedOutput, 30);
			}
			return true;
		} catch (Exception e) {
			VisualReporter.logError("changeAdminPassword: Unexpected exception " + e.getMessage());
		}
		return false;
	}

}
