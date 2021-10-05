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
package com.ibm.ism.test.testcases.appliance.users;

import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
//import org.openqa.selenium.WebElement;
import org.openqa.selenium.firefox.*;

import com.ibm.automation.core.Platform;
//import com.ibm.automation.core.web.SeleniumCore;
//import com.ibm.automation.core.web.WebBrowser;


public class TC_CreateUser_New {
	public static void main(String[] args) {
		WebDriver driver = new FirefoxDriver();
		driver.get("https://10.10.4.175:9087");
		driver.findElement(By.id("login_userIdInput")).sendKeys("admin");
		driver.findElement(By.id("login_passwordInput")).sendKeys("admin");
		driver.findElement(By.id("login_submitButton")).click();
		
		Platform.sleep(3);
		driver.close();
	}
}
