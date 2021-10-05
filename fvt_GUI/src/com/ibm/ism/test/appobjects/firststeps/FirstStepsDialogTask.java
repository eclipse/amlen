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
package com.ibm.ism.test.appobjects.firststeps;


import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.firststeps.FirstStepsDialogObjects;
//import com.ibm.ism.test.appobjects.license.LicenseAgreementObjects;
import com.ibm.ism.test.common.IsmTest;

/**
 * Provides actions and tasks that can performed on the Ethernet Interfaces dialog.
 * 
 *
 */
public class FirstStepsDialogTask {

	public static boolean leaveFirstSteps() {
		FirstStepsDialogObjects.getButton_LeaveFirstSteps().hover();
		FirstStepsDialogObjects.getButton_LeaveFirstSteps().focus();
		FirstStepsDialogObjects.getButton_LeaveFirstSteps().click();
		return IsmTest.waitForBrowserReady();
	}

	public static boolean completeFirstSteps() {
		FirstStepsDialogObjects.getButton_CompleteFirstSteps().hover();
		FirstStepsDialogObjects.getButton_CompleteFirstSteps().focus();
		FirstStepsDialogObjects.getButton_CompleteFirstSteps().click();
		return IsmTest.waitForBrowserReady();
	}  
	
	public static boolean verifyFirstStepsDialog() {
		
		WebBrowser.waitForReady();
		return WebBrowser.isTextPresent("Are you sure that you want to leave First Steps?");
		}
	
	public static boolean confirmFirstSteps() {
		WebBrowser.waitForReady();
		try {
			
			FirstStepsDialogObjects.getButton_CompleteFirstSteps().hover();
			FirstStepsDialogObjects.getButton_CompleteFirstSteps().focus();
			FirstStepsDialogObjects.getButton_CompleteFirstSteps().click();
			return WebBrowser.isTextPresent("Are you sure that you want to leave First Steps?");
			
		} catch (Exception e) {
			VisualReporter.logError("Confirm First Steps: Unexpected exception " + e.getMessage());
		}
		return false;
	}
		
//		IsmTest.waitForBrowserReady();
//		if (WebBrowser.isTextPresent("Are you sure you want to leave First Steps?")){
//			FirstStepsObjects.getButton_LeaveFirstSteps().hover();
//			FirstStepsObjects.getButton_LeaveFirstSteps().click();
//	    }	
			
		
//		IsmTest.waitForBrowserReady();
//		if (textToVerify != null) {
//			return WebBrowser.isTextPresent(textToVerify);
//		}
//		return true;
//	}
	
}



