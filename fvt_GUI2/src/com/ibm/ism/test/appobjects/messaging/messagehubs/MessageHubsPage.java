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
package com.ibm.ism.test.appobjects.messaging.messagehubs;

import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;

import com.ibm.automation.core.web.SeleniumCore;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;


public class MessageHubsPage  {
	
	public static SeleniumTestObject getLink_Edit() {
		return new SeleniumTestObject("//a[@id='editMHDetailsName']");
	}
	
	  public static SeleniumTestObject getGridIcon_MsgHubAdd() {
			return new SeleniumTestObject("//span[@id='messageHubGridGrid_add']");
	  }
	  
	  public static SeleniumTestObject getGridIcon_ConnPolAdd() {
			return new SeleniumTestObject("//span[@id='ismConnectionPolicyListGrid_add']");
	  }
	  
	  public static SeleniumTestObject getGridIcon_MsgPolAdd() {
			return new SeleniumTestObject("//span[@id='ismMessagingPolicyListGrid_add']");
			
	  }
	  
	  public static SeleniumTestObject getGridIcon_EndPointAdd() {
			return new SeleniumTestObject("//span[@id='ismEndpointListGrid_add']");
	  }
	  
	  public static SeleniumTextTestObject getGridTextBox_Filter() {
		  return new SeleniumTextTestObject("//*[@id='messageHubGridGrid_FilterInputContainer']/input");
	  }
	  
	  public static SeleniumTestObject getGridCell(int row, int column) {
		  if (row == 0) {
			  return new SeleniumTextTestObject("//*[@id='messageHubGridGrid']/div[3]/div[2]/div/table/tbody/tr/td[" + column + "]");
		  } else {
			  return new SeleniumTextTestObject("//*[@id='messageHubGridGrid']/div[3]/div[2]/div[" + row + "]/table/tbody/tr/td[" + column + "]");
		  }
	  }
	  
	  //public static SeleniumTestObject getErrorMessage() {
		//  return new SeleniumTestObject("//span[@id='idx_widget_SingleMessage_0_messageTitle']");
	  //}
	  
	  
	  	public static WebElement getErrorMessage() {
	  		return SeleniumCore.getWebDriverBrowser().getWebDriverAPI().findElement(By.id("idx_widget_SingleMessage_0_messageTitle"));
	  	}
	  
}
