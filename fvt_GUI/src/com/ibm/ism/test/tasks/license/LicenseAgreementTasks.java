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
package com.ibm.ism.test.tasks.license;

import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.license.LicenseAgreementObjects;
import com.ibm.ism.test.common.IsmGuiTestException;
import com.ibm.ism.test.testcases.license.IsmLicenseTestData;

/**
 * This class provides tasks that can be performed on the License Agreement page.
 * 
 *
 */
public class LicenseAgreementTasks {
	
	private static final String HEADER_LICENSE_AGREEMENT = "License Agreement";
	private static final String TITLE_LICENSE_ACCEPTED = "License Accepted";
	private static final String TEXT_LICENSE_ACCEPTED = "The license has been accepted.  The IBM MessageSight server is starting.";
	
	public static final String CLI_TEXT_LICENSE_NOT_YET_ACCEPTED = "IBM MessageSight is not fully functional until you accept IBM MessageSight license from the IBM MessageSight Web UI.";

	public static void verifyPageLoaded(IsmLicenseTestData testData) throws Exception {
		verifyPageLoaded(testData.isDebug());
	}

	public static void verifyPageLoaded(boolean debug) throws Exception {
		if  (!WebBrowser.waitForText(HEADER_LICENSE_AGREEMENT, 30)) {
			throw new IsmGuiTestException(HEADER_LICENSE_AGREEMENT + " not found!!!");
		}
	}
	
	public static void acceptLicense() throws Exception{
		// First dump the license content
		System.err.println(LicenseAgreementObjects.getTextArea_LicenseContent().getText());
		LicenseAgreementObjects.getButton_Agree().hover();
		LicenseAgreementObjects.getButton_Agree().click();
		if  (!WebBrowser.waitForText(TEXT_LICENSE_ACCEPTED, 30)) {
			throw new IsmGuiTestException("\"" + TEXT_LICENSE_ACCEPTED + "\" not found!!!");
		}
		String title = LicenseAgreementObjects.getWebElement_AcceptLicenseDialogTitle().getText();
		if (!title.equals(TITLE_LICENSE_ACCEPTED)) {
			throw new IsmGuiTestException("Dialog title is \"" + title + "\".  Expected title \"" + TITLE_LICENSE_ACCEPTED + "\"");
		}
		if (LicenseAgreementObjects.getAcceptLicenseDialogCloseButton().waitForExistence(5)) {
			LicenseAgreementObjects.getAcceptLicenseDialogCloseButton().hover();
			LicenseAgreementObjects.getAcceptLicenseDialogCloseButton().click();
			//@TODO: Why NullPointerException when bouldClick() method was used  
			//10:02:35 AM - 00:00:07:333 - PASS - Click widget: "li_acceptButton"
			//	java.lang.NullPointerException
			//		at com.ibm.automation.core.web.SeleniumCore$seleniumHelper.doubleClick(SeleniumCore.java:1447)
			//		at com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject.doubleClick(SeleniumTestObject.java:480)
			//		at com.ibm.ism.test.tasks.license.LicenseAgreementTasks.acceptLicense(LicenseAgreementTasks.java:55)
			//		
			//LicenseAgreementObjects.getAcceptLicenseDialogCloseButton().doubleClick();
		} else {
			throw new IsmGuiTestException("Close button does not exist!!!"); 
		}
	}

}
