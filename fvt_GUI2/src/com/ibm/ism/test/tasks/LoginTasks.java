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
package com.ibm.ism.test.tasks;

import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.ui.ExpectedConditions;
import org.openqa.selenium.support.ui.WebDriverWait;

import com.ibm.automation.core.Platform;
//import com.ibm.automation.core.loggers.VisualReporter;
//import com.ibm.automation.core.loggers.control.CoreLogger;
import com.ibm.automation.core.web.SeleniumCore;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.LoginObjects;
import com.ibm.ism.test.common.IsmTestData;


/**
 * Provides actions and tasks that can performed on the ISM login and logout related objects.
 * 
 *
 */
public class LoginTasks {
	
	private static final String LABEL_USER_ID = "User ID";
	private static final String LABEL_PASSWORD = "Password";
	public static final String PAGE_TITLE = "IBM MessageSight";
	private static long maxWaitInMillis = 30000;
	
	public static void login(IsmTestData testData, String textToVerify) throws Exception {
		login(testData);
		if (textToVerify == null) {
			Platform.sleep(3);
		} else {
			if (!WebBrowser.waitForText(textToVerify, 10)) {
				throw new Exception("Login failed text verification. String expected: " + textToVerify);
			}
				
		}
	}

	@SuppressWarnings("deprecation")
	public static void login(IsmTestData testData) throws Exception {
		WebDriver driver = SeleniumCore.getWebDriverBrowser().getWebDriverAPI();
		try {
			WebElement button = new WebDriverWait( driver, (int)( maxWaitInMillis / 1000 + 1 ) ).until( ExpectedConditions.visibilityOfElementLocated( By.id(LoginObjects.ID_LOGIN_BUTTON)));
			if (button.isEnabled() == false) {
				//VisualReporter.logError("Login button is not enabled, URL " + driver.getCurrentUrl());
				     //CoreLogger.error("Login button is not enabled");
				//CoreLogger.testStart("Login button is not enabled");
			}
			//VisualReporter.logScreenCapture("Login page is loaded for URL " + driver.getCurrentUrl(), false);
			     //CoreLogger.info("Login page is loaded for URL");
			//CoreLogger.testStart("Login page is loaded for URL");
		} catch (Exception e) {
			e.printStackTrace();
			//VisualReporter.logError("Login page not loaded for URL " + driver.getCurrentUrl());
			     //CoreLogger.error("Login page not loaded for URL");
			//CoreLogger.testStart("Login page not loaded for URL");
			//VisualReporter.logScreenCapture("Login page not loaded for URL " + driver.getCurrentUrl(), true);
			    //CoreLogger.error("Login page not loaded");
			//CoreLogger.testStart("Login page is not loaded");
			throw e;
		}
		
		
		if (!login(testData.getUser(), testData.getPassword(), testData.isDebug())) {
			throw new Exception("Login failed for user=" + testData.getUser() + " password=" + testData.getPassword() + " URL=" + testData.getURL());
		}			
	}

	public static boolean login(String user, String password) {
		return login(user, password, false);
	}
	
	/**
	 * Login the WebUI using the supplied user and password.
	 * @param user
	 * @param password
	 * @return true for success, false if failed
	 */
	@SuppressWarnings("deprecation")
	public static boolean login(String user, String password, boolean debug) {
		boolean result = false;
		try {
			if (!WebBrowser.waitForText(LABEL_USER_ID, 30)) {
				return false;
			}
			if (!WebBrowser.waitForText(LABEL_PASSWORD, 30)) {
				return false;
			}
			if (debug) {
				//VisualReporter.logScreenCapture("Login page");
				      //CoreLogger.info("Login page"); 
				//CoreLogger.testStart("Login page");
			}
			LoginObjects.getTextEntry_UserId().setText(user);
			Platform.sleep(1);
			LoginObjects.getTextEntry_Password().setText(password);
			Platform.sleep(1);
			LoginObjects.getButton_Login().click();
			Platform.sleep(3);
			      //CoreLogger.info("Login with User admin and Password");
			//CoreLogger.testStart("Login with User admin and Password");
			result = true;
		} catch (Exception e) {
			//VisualReporter.logScreenCapture("Login failed", true);
			      //CoreLogger.info("Login failed");
			//CoreLogger.testStart("Login failed");
			result = false;
		}
		return result;
	}
	
	/**
	 * Logout of the WebUI using the top right corner menu.
	 * @return true for success, false if failed
	 */
	public static boolean logout() {
		boolean result = false;
		try {
			Platform.sleep(3);
			LoginObjects.getMenu_User().click();
			if (LoginObjects.getMenuItem_Logout().waitForExistence(5)) {
				//LoginObjects.getMenuItem_Logout().hover();
				//CoreLogger.info("Time to logout");
				     //CoreLogger.testStart("Time to logout");
				LoginObjects.getMenuItem_Logout().click();
			} else {
				return false;
			}
			if (!WebBrowser.waitForText(LABEL_USER_ID, 5)) {
				return false;
			}
			if (!WebBrowser.waitForText(LABEL_PASSWORD, 5)) {
				return false;
			}
			result = true;
		} catch (Exception e) {
			//VisualReporter.failTest("Logout failed", e);
			     //CoreLogger.error("Logout failed");
			//CoreLogger.testStart("Logout failed");
			//@FIXME Check exception before failing the test.
			result = true;
		}
		return result;
	}
	
	public static void logout(IsmTestData testData) throws Exception {
		if (!logout()) {
			throw new Exception("Logout failed.  Current login user=" + testData.getUser() + " URL=" + testData.getURL());
		}
	}
}
