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

import com.ibm.ism.test.appobjects.EthernetInterfacesDialogObjects;
import com.ibm.ism.test.common.IsmTest;

/**
 * Provides actions and tasks that can performed on the Ethernet Interfaces dialog.
 * 
 *
 */
public class EthernetInterfacesDialogTasks {

	public static boolean close() {
		EthernetInterfacesDialogObjects.getButton_Close().hover();
		EthernetInterfacesDialogObjects.getButton_Close().click();
		return IsmTest.waitForBrowserReady();
	}

	public static boolean save() {
		EthernetInterfacesDialogObjects.getButton_Save().hover();
		EthernetInterfacesDialogObjects.getButton_Save().click();
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean focusDefaultGateway() {
		EthernetInterfacesDialogObjects.getTextEntry_DefaultGateway().focus();
		return IsmTest.waitForBrowserReady();
	}
	
	public static boolean setDefaultGateway(String value) {
		EthernetInterfacesDialogObjects.getTextEntry_DefaultGateway().focus();
		EthernetInterfacesDialogObjects.getTextEntry_DefaultGateway().setText(value);
		return IsmTest.waitForBrowserReady();
	}

}
