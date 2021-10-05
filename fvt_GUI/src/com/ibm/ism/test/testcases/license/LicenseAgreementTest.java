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
package com.ibm.ism.test.testcases.license;

import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.firefox.FirefoxDriver;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.common.IsmTest;
import com.ibm.ism.test.tasks.HomeTasks;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.firststeps.FirstStepsTasks;
import com.ibm.ism.test.tasks.license.LicenseAgreementTasks;



/**
 *
 */
public class LicenseAgreementTest {

	/**
	 * @param args Test arguments
	 */
	public static void main(String[] args) {
		LicenseAgreementTest test = new LicenseAgreementTest();
		System.exit(test.runTest(new IsmLicenseTestData(args)) ? 0 : 1);
	}
	
	public boolean runTest(IsmLicenseTestData testData) {
		boolean result = false;
		String testDesc = "Accept License";

		Platform.setEngineToSeleniumWebDriver();
		WebBrowser.start(testData.getBrowser());
		WebBrowser.loadUrl(testData.getURL());
		
		VisualReporter.startLogging("Mike Tran", "IMS License Agreement", "IMS FVT");
		VisualReporter.logTestCaseStart(testDesc);

		try {
			LoginTasks.login(testData);
			LicenseAgreementTasks.verifyPageLoaded(testData);
			LicenseAgreementTasks.acceptLicense();
			String runTime = testData.getRuntime();
			System.out.println("RunTime Source for CCI = "+runTime);
			if ((testData.getRuntime() != null) && !testData.getRuntime().equals("CCI")) {
			  FirstStepsTasks.verifyPageLoaded();
			} else {
			  HomeTasks.verifyPageLoaded();
			}
			IsmTest.delay (20);
			LoginTasks.logout();
			result = true;
		} catch (Exception e){
			VisualReporter.failTest(testDesc + " failed.", e);
			result = false;
		}
		WebBrowser.shutdown();
		VisualReporter.stopLogging();
		return result;
	}

	public boolean runTest2(IsmLicenseTestData testData) {
		//String testDesc = "Accept License";
		
		//VisualReporter.startLogging("Mike Tran", "ISM License Agreement", "ISM FVT");
		//VisualReporter.logTestCaseStart(testDesc);
		//VisualReporter.logInfo("URL: " + testData.getURL());
		boolean result = false;
		//FirefoxProfile profile = new FirefoxProfile();
		//WebDriver driver = new FirefoxDriver(profile);
		WebDriver driver = new FirefoxDriver();
		//driver.get(testData.getURL());
		driver.navigate().to(testData.getURL());
		
		try {
			//Wait for Login page
			int i = 0;
			while (i < 15 && !driver.getTitle().contains("Log in")) {
				try {
					Thread.sleep(1000);
					i++;
				} catch (Exception e) {
					//do nothing
				}
			}
			
			WebElement inputUserID = driver.findElement(By.id("login_userIdInput"));
			inputUserID.sendKeys(testData.getUser());
			WebElement inputPassword = driver.findElement(By.id("login_passwordInput"));
			inputPassword.sendKeys(testData.getPassword());
			WebElement buttonLogin = driver.findElement(By.id("login_submitButton"));
			buttonLogin.click();
			i = 0;
			while (i < 15 && !driver.getTitle().contains("License Agreement")) {
				try {
					Thread.sleep(1000);
					i++;
				} catch (Exception e) {
				}
			}
	
			WebElement buttonAgree = driver.findElement(By.id("li_acceptButton"));
			buttonAgree.click();
			result = true;
		} catch (Exception e) {
			System.out.println(e.getMessage());
		}
		driver.quit();
		return result;
	}
	

}
