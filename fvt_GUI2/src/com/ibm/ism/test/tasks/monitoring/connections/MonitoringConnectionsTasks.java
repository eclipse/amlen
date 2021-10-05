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
package com.ibm.ism.test.tasks.monitoring.connections;

import com.ibm.automation.core.Platform;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.common.IsmGuiTestException;
import com.ibm.ism.test.common.IsmTestData;

public class MonitoringConnectionsTasks {
	
	private static final String HEADER_CONNECTION_MONITOR = "Connection Monitor";
	private static final String HEADER_CONNECTION_CHART = "Connection Chart";
	
	public static void verifyPageLoaded() throws Exception {
		WebBrowser.waitForReady();
		if  (!WebBrowser.waitForText(HEADER_CONNECTION_MONITOR, 10)) {
			throw new IsmGuiTestException(HEADER_CONNECTION_MONITOR + " not found!!!");
		}
		if  (!WebBrowser.waitForText(HEADER_CONNECTION_CHART, 10)) {
			throw new IsmGuiTestException(HEADER_CONNECTION_CHART + " not found!!!");
		}
	}
	
	public static void loadURL(IsmTestData testData) throws Exception {
		WebBrowser.loadURL(testData.getURL() + "/monitoring.html?nav=connectionStatistics");
		Platform.sleep(3);
		verifyPageLoaded();
	}
}
