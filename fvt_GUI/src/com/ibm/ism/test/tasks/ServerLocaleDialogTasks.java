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

import com.ibm.ism.test.appobjects.ServerLocaleDialogObjects;
import com.ibm.ism.test.common.IsmTest;

/**
 * Provides actions and tasks that can performed on the Ethernet Interfaces dialog.
 * 
 *
 */
public class ServerLocaleDialogTasks {

	public static boolean close() {
		ServerLocaleDialogObjects.getButton_Close().hover();
		ServerLocaleDialogObjects.getButton_Close().click();
		return IsmTest.waitForBrowserReady();
	}

	public static boolean save() {
		ServerLocaleDialogObjects.getButton_Save().hover();
		ServerLocaleDialogObjects.getButton_Save().click();
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean focusServerLocale() {
		ServerLocaleDialogObjects.getTextEntry_ServerLocale().focus();
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean setServerLocale(String value) {
		ServerLocaleDialogObjects.getTextEntry_ServerLocale().focus();
		ServerLocaleDialogObjects.getTextEntry_ServerLocale().setText(value);
		return IsmTest.waitForBrowserReady();
	}

}
