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
package com.ibm.ism.test.common;

import com.ibm.automation.core.web.WebBrowser;

/**
 *
 */
public class IsmTest {
	
	public static int giBrowserWaitTime = 1;
	public static int giDelayTime = 0;
	
	public static String gsUserId = null;
	public static String gsPassword = null;
	public static String gsUserRole = null;
	
	public static final String APPLIANCE_ADMIN_ROLE = "Appliance Admin";
	public static final String MESSAGING_ADMIN_ROLE = "Messaging Admin";
	public static final String NORMAL_USER_ROLE = "Normal User";
	
	public static void delay() {
		delay(giDelayTime * 1000);
	}
	
	public static void delay(int iMilliseconds) {
		try {
			Thread.sleep(iMilliseconds);
		} catch (InterruptedException e) {
			// Do nothing
		}		
	}
	
	public static boolean waitForBrowserReady() {
		return WebBrowser.waitForReady(giBrowserWaitTime);
	}
	

}
