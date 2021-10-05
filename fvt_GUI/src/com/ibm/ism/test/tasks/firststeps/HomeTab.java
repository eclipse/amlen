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

package com.ibm.ism.test.tasks.firststeps;

import com.ibm.automation.core.web.WebBrowser;

public class HomeTab {
	
	private static final String PAGE_HEADER = "Common configuration and customization tasks";
	private static final String CUSTOMIZE_APPLIANCE_HEADER = "Customize appliance settings";
	private static final String SECURE_APPLIANCE_HEADER = "Secure your appliance";
	private static final String CREATE_USERS_AND_GROUPS_HEADER = "Create users and groups";
	private static final String CONFIGURE_IBM_MESSAGESIGHT_HEADER = "Configure IBM MessageSight to accept connections";
	
	public static boolean verifyPageLoaded() {
		WebBrowser.waitForReady();
		
		if (WebBrowser.waitForText(PAGE_HEADER, 30) == false) {
			return false;
		}
		if (WebBrowser.waitForText(CUSTOMIZE_APPLIANCE_HEADER, 30) == false) {
			return false;
		}
		if (WebBrowser.waitForText(SECURE_APPLIANCE_HEADER, 30) == false) {
			return false;
		}
		
		if (WebBrowser.waitForText(CREATE_USERS_AND_GROUPS_HEADER, 30) == false) {	
			return false;
		}
		if (WebBrowser.waitForText(CONFIGURE_IBM_MESSAGESIGHT_HEADER, 30) == false) {
			return false;
		}
		return true;
	}
	
//public class HomeTab {
	
//	public static boolean verifyPageLoaded() {
//		WebBrowser.waitForReady();
//		return WebBrowser.isTextPresent("Recommended configuration and customization tasks");
//	}

}
