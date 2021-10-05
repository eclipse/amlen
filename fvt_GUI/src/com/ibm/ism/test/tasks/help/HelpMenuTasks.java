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

import com.ibm.ism.test.appobjects.help.HelpMenuObjects;
import com.ibm.ism.test.common.IsmGuiTestException;

public class HelpMenuTasks {

	public static void showHelpMenu() throws Exception {
		HelpMenuObjects.getMenu_Help().hover();
		HelpMenuObjects.getMenu_Help().click();
	}
	
	public static void selectAbout() throws Exception {
		if (HelpMenuObjects.getMenuItemAbout().waitForExistence(5)) {
			HelpMenuObjects.getMenuItemAbout().hover();
			HelpMenuObjects.getMenuItemAbout().click();
		} else {
			throw new IsmGuiTestException("Help About menu item is not visible.");
		}
	}
}
