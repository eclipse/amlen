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
package com.ibm.ism.test.tasks.help;

import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.appobjects.help.AboutDialogObjects;
import com.ibm.ism.test.common.IsmGuiTestException;

public class AboutDialogTasks {

	public static boolean isShowing() throws Exception {
		return AboutDialogObjects.getDialog().waitForExistence(5);
	}
	
	public static boolean isBuildLevel(String buildLevel) throws Exception {
		return WebBrowser.waitForText(buildLevel, 10);
	}
	
	public static void verifyBuildLevel(String buildLevel) throws Exception {
		if (!isShowing()) {
			throw new IsmGuiTestException("About dialog is not showing.");
		}
		if (!isBuildLevel(buildLevel)) {
			throw new IsmGuiTestException("Build level " + buildLevel + " not found.");
		}
	}
}
