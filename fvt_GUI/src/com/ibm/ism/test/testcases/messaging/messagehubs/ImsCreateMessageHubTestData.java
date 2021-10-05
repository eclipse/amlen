package com.ibm.ism.test.testcases.messaging.messagehubs;


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
import com.ibm.ism.test.common.IsmTestData;

public class ImsCreateMessageHubTestData extends IsmTestData {

	public static final String KEY_MESSAGE_HUB_NAME = "MESSAGE_HUB_NAME";
	public static final String KEY_MESSAGE_HUB_DESC = "MESSAGE_HUB_DESC";
	
	
	public ImsCreateMessageHubTestData(String[] args) {
		super(args);
	}
	
	public String getMessageHubName() {
		return getProperty(KEY_MESSAGE_HUB_NAME);
	}
	
	public void setMessageHubName(String name) {
		setProperty(KEY_MESSAGE_HUB_NAME, name);
	}
	
	public String getMessageHubDescription() {
		return getProperty(KEY_MESSAGE_HUB_DESC);
	}
		
	
}

