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
package com.ibm.ism.test.appobjects;

import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;

/**
 * Provides access to all test objects on the ISM Messaging tab.
 * 
 *
 */
public class MessagingObjects {
	public static SeleniumTestObject getTab_Messaging() {
		return new SeleniumTestObject("//span[text()='Messaging']");
	}
	public static SeleniumTestObject getActionLink_TopicAdministration() {
		return new SeleniumTestObject("//a[@id='actionNav_topicAdministration_trigger']");
	}
	public static SeleniumTestObject getActionLink_Foo() {
		return new SeleniumTestObject("//a[@id='actionNav_foo_trigger']");
	}
	public static SeleniumTestObject getActionLink_Bar() {
		return new SeleniumTestObject("//a[@id='actionNav_bar_trigger']");
	}
	public static SeleniumTestObject getMenuItem_TopicAdministration() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/messaging/topicAdministration_text']");
	}
	public static SeleniumTestObject getMenuItem_Listener() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/messaging/listeners_text']");
//		return new SeleniumTestObject("//td[text()='Listener Configuration']");
	}
	public static SeleniumTestObject getMenu_Messaging() {
		return new SeleniumTestObject("//span[text()='Messaging']");
		
	}
	public static SeleniumTestObject getMenuItem_MessageHub() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/messaging/messageHubs_text']");
	
	}
	public static SeleniumTestObject getMenuItem_MessageQueue() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/messaging/messagequeues_text']");
	}
	
	public static SeleniumTestObject getMenuItem_MQConnectivity() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/messaging/mqConnectivity_text']");
	}
	
	public static SeleniumTestObject getMenuItem_MessageUsers() {
		return new SeleniumTestObject("//td[@id='navMenuItem_action/messaging/userAdministration_text']");
	}
	
}
