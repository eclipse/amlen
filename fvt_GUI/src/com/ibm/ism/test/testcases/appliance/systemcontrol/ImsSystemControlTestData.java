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
package com.ibm.ism.test.testcases.appliance.systemcontrol;

import com.ibm.ism.test.common.IsmTestData;

public class ImsSystemControlTestData extends IsmTestData {

	public static final String KEY_NEW_NODE_NAME = "NEW_NODE_NAME";
	public static final String KEY_NEW_RUN_MODE = "NEW_RUN_MODE";
	public static final String KEY_CURRENT_STATUS = "CURRENT_STATUS";
	public static final String KEY_AFTER_RESTART_STATUS = "AFTER_RESTART_STATUS";
	
	public ImsSystemControlTestData(String args[]) {
		super(args);
	}
	
	public String getNewNodeName() {
		return getProperty(KEY_NEW_NODE_NAME);
	}

	public String getNewRunMode() {
		return getProperty(KEY_NEW_RUN_MODE);
	}

	public String getCurrentStatus() {
		return getProperty(KEY_CURRENT_STATUS);
	}

	public String getAfterRestartStatus() {
		return getProperty(KEY_AFTER_RESTART_STATUS);
	}
	
	public void setAfterRestartStatus(String status) {
		setProperty(KEY_AFTER_RESTART_STATUS, status);
	}
	
}
