// Copyright (c) 2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
package com.ibm.ism.test.tasks.firststeps;

import com.ibm.automation.core.web.WebBrowser;

public class ApplianceTab {
	public static boolean verifyPageLoaded() {
	WebBrowser.waitForReady();
	return WebBrowser.isTextPresent("Recommended configuration and customization tasks");
}
	
//	private static final String APPLIANCE_USERS_HEADER = "Users";
//	private static final String APPLIANCE_NETWORK_SETTINGS_HEADER = "Network Settings";
//	private static final String APPLIANCE_DATE_AND_TIME_HEADER = "Server Date and Time";
//	private static final String APPLIANCE_SECURITY_SETTINGS_HEADER = "Security Settings";
//	private static final String APPLIANCE_SYSTEM_CONTROL_HEADER = "System Control";
//	private static final String APPLIANCE_HIGH_AVAILABILITY_HEADER = "High Availability";
//	private static final String APPLIANCE_WEB_UI_SETTINGS_HEADER = "Web UI Settings";
	
	
	
//	public static boolean verifyPageLoaded() {
//		WebBrowser.waitForReady();
		
//		if (WebBrowser.waitForText(APPLIANCE_USERS_HEADER, 30) == false) {
//			return false;
//		}
//		if (WebBrowser.waitForText(APPLIANCE_NETWORK_SETTINGS_HEADER, 30) == false) {
//			return false;
//		}
//		if (WebBrowser.waitForText(APPLIANCE_DATE_AND_TIME_HEADER, 30) == false) {
//			return false;
//		}
//		
//		if (WebBrowser.waitForText(APPLIANCE_SECURITY_SETTINGS_HEADER, 30) == false) {	
//			return false;
//		}
//		if (WebBrowser.waitForText(APPLIANCE_SYSTEM_CONTROL_HEADER, 30) == false) {
//			return false;
//		}
//		if (WebBrowser.waitForText(APPLIANCE_HIGH_AVAILABILITY_HEADER, 30) == false) {
//			return false;
//		}
//		if (WebBrowser.waitForText(APPLIANCE_WEB_UI_SETTINGS_HEADER, 30) == false) {
//			return false;
//		}
		
		
//		return true;
//	}
	

}
