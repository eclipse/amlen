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
package com.ibm.ism.test.appobjects.messaging.users;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;


public class MessagingUsersandGroupsPage {


    public static SeleniumTestObject getGridIcon_MsgUserAdd() {
		return new SeleniumTestObject("//span[@id='msgusersGrid_add']");
	}
    
    public static SeleniumTestObject getGridIcon_MsgUserEdit() {
		return new SeleniumTestObject("//span[@id='msgusersGrid_edit']");
	}
	
    public static SeleniumTestObject getGridIcon_MsgUserDelete() {
		return new SeleniumTestObject("//span[@id='msgusersGrid_delete']");
	}
    
	public static SeleniumTestObject getGridIcon_MsgGroupAdd() {
		return new SeleniumTestObject("//span[@id='msggroupsGrid_add']");
	}
	
	public static SeleniumTestObject getGridIcon_MsgGroupEdit() {
		return new SeleniumTestObject("//span[@id='msggroupsGrid_edit']");
	}
	
	public static SeleniumTestObject getGridIcon_MsgGroupDelete() {
		return new SeleniumTestObject("//span[@id='msggroupsGrid_delete']");
	}
	
	public static SeleniumTestObject getGridIcon_LDAPAdd() {
		return new SeleniumTestObject("//span[@id='exldapGrid_add']");
	}
	
	public static SeleniumTestObject getGridIcon_LDAPEdit() {
		return new SeleniumTestObject("//span[@id='exldapGrid_edit']");
	}
	
	public static SeleniumTestObject getGridIcon_LDAPDelete() {
		return new SeleniumTestObject("//span[@id='exldapGrid_delete']");
	}
	
	public static SeleniumTestObject getButton_EnableLDAP() {
		return new SeleniumTestObject("//span[@id='enableLDAPButton']");
	}
}

