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

import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.MessagingObjects;
import com.ibm.ism.test.appobjects.firststeps.FirstStepsDialogTask;
//import com.ibm.ism.test.appobjects.firststeps.FirstStepsObjects;
import com.ibm.ism.test.common.IsmTest;

/**
 * Provides actions and tasks that can performed on the ISM Monitoring tab.
 * 
 *
 */
public class MessagingTasks {

	public static boolean clickMessagingTab(String textToVerify) {
		MessagingObjects.getTab_Messaging().hover();
		MessagingObjects.getTab_Messaging().click();
		MessagingObjects.getMenuItem_MessageHub().hover();
		MessagingObjects.getMenuItem_MessageHub().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
	public static boolean clickMessageHubTab(String textToVerify) {
		MessagingObjects.getTab_Messaging().hover();
		MessagingObjects.getTab_Messaging().click();
		MessagingObjects.getMenuItem_MessageHub().hover();
		MessagingObjects.getMenuItem_MessageHub().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
public static boolean clickMessageQueueTab(String textToVerify) {
	    MessagingObjects.getTab_Messaging().hover();
	    MessagingObjects.getTab_Messaging().click();
		MessagingObjects.getMenuItem_MessageQueue().hover();
		MessagingObjects.getMenuItem_MessageQueue().click();
		
			
			
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		}
		return true;
	}
	
public static boolean clickMessageMQConnectivityTab(String textToVerify) {
	MessagingObjects.getTab_Messaging().hover();
	MessagingObjects.getTab_Messaging().click();
	MessagingObjects.getMenuItem_MQConnectivity().hover();
	MessagingObjects.getMenuItem_MQConnectivity().click();
	
		
		
	IsmTest.waitForBrowserReady();
	if (textToVerify != null) {
		return WebBrowser.isTextPresent(textToVerify);
	}
	return true;
}

public static boolean clickMessageUserTab(String textToVerify) {
	MessagingObjects.getTab_Messaging().hover();
	MessagingObjects.getTab_Messaging().click();
	MessagingObjects.getMenuItem_MessageUsers().hover();
	MessagingObjects.getMenuItem_MessageUsers().click();
	
		
		
	IsmTest.waitForBrowserReady();
	if (textToVerify != null) {
		return WebBrowser.isTextPresent(textToVerify);
	}
	return true;
}
	

	public static boolean clickTopicAdministrationAction(String textToVerify) {
		//MessagingObjects.getActionLink_TopicAdministration().hover();
		//MessagingObjects.getActionLink_TopicAdministration().click();
		MessagingObjects.getMenu_Messaging().hover();
		MessagingObjects.getMenu_Messaging().click();
		IsmTest.waitForBrowserReady();
		MessagingObjects.getMenuItem_TopicAdministration().hover();
		MessagingObjects.getMenuItem_TopicAdministration().click();
		IsmTest.waitForBrowserReady();
		if (FirstStepsDialogTask.verifyFirstStepsDialog() == true) {
			FirstStepsDialogTask.leaveFirstSteps();
	        IsmTest.delay ();
		} 
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		} else {
			return WebBrowser.isTextPresent("Topic Administration");
		}
	}
	
	public static boolean clickListenerAction(String textToVerify) {
		//MessagingObjects.getActionLink_Listener().hover();
		//MessagingObjects.getActionLink_Listener().click();
		IsmTest.delay ();
		MessagingObjects.getMenu_Messaging().hover();
		MessagingObjects.getMenu_Messaging().click();
		IsmTest.delay ();
		MessagingObjects.getMenuItem_Listener().hover();
		MessagingObjects.getMenuItem_Listener().click();
		IsmTest.waitForBrowserReady();
		if (FirstStepsDialogTask.verifyFirstStepsDialog() == true) {
			FirstStepsDialogTask.leaveFirstSteps();
	        IsmTest.delay ();
		} 
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		} else {
			return WebBrowser.isTextPresent("Listener");
		}
	}
	public static boolean clickFooAction(String textToVerify) {
		MessagingObjects.getActionLink_Foo().hover();
		MessagingObjects.getActionLink_Foo().click();
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		} else {
			return WebBrowser.isTextPresent("Foo");
		}
	}
	
	public static boolean clickBarAction(String textToVerify) {
		MessagingObjects.getActionLink_Bar().hover();
		MessagingObjects.getActionLink_Bar().click();
		IsmTest.waitForBrowserReady();
		if (textToVerify != null) {
			return WebBrowser.isTextPresent(textToVerify);
		} else {
			return WebBrowser.isTextPresent("Bar");
		}
	}
	
}
