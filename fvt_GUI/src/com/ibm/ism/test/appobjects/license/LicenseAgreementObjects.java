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
package com.ibm.ism.test.appobjects.license;

import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;

import com.ibm.automation.core.web.SeleniumCore;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;

/**
 * This class provides access to all UI objects on the License Agreement page.
 * 
 *
 */
public class LicenseAgreementObjects {

	public static SeleniumTestObject getButton_Agree() {
		return new SeleniumTestObject("//span[@id='li_acceptButton']");
	}

	public static SeleniumTestObject getButtonLabel_Agree() {
		return new SeleniumTestObject("//span[@id='li_acceptButton_label']");
	}
	
	public static SeleniumTestObject getButton_DoNotAgree() {
		return new SeleniumTestObject("//span[@id='li_declineButton']");
	}

	public static SeleniumTestObject getButtonLabel_DoNotAgree() {
		return new SeleniumTestObject("//span[@id='li_declineButton_label']");
	}

	public static SeleniumTestObject getButton_Print() {
		return new SeleniumTestObject("//span[@id='li_printButton']");
	}

	public static SeleniumTestObject getButtonLabel_Print() {
		return new SeleniumTestObject("//span[@id='li_printButton_label']");
	}
	
	public static SeleniumTestObject getLink_ReadNonIBMTerms() {
		return new SeleniumTestObject("//a[@id='li_nonIBMTermsLink']");
	}
	
	public static SeleniumTextTestObject getTextArea_LicenseContent() {
		return new SeleniumTextTestObject("//textarea[@id='li_licenseContent']");
	}
	
	public static SeleniumTestObject getPulldownList_LanguageChoice() {
		return new SeleniumTestObject("//table[@id='langChoice']");
	}
	
	public static SeleniumTestObject getAcceptLicenseDialogTitle() {
		return new SeleniumTestObject("//span[@id='acceptLicenseDialog_title']");
	}
	
	public static WebElement getWebElement_AcceptLicenseDialogTitle() {
		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("acceptLicenseDialog_title"));
	}	
	
	public static SeleniumTestObject getAcceptLicenseDialogCloseButton() {
		//return new SeleniumTestObject("//span[@id='acceptLicenseDialog_closeButton']");
		return new SeleniumTestObject("//span[@id='dijit_form_Button_0']");
	}
}
