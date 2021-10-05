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
package com.ibm.ism.test.testcases.license;

import com.ibm.ism.test.common.IsmTestData;

/**
 * Test data for License
 * 
 *
 */
public class IsmLicenseTestData extends IsmTestData {

	public static final String KEY_VERIFY_LICENSE_ACCEPTED = "VERIFY_LICENSE_ACCEPTED";
	
	public IsmLicenseTestData(String[] args)  {
		super(args);
	}
	
	public boolean isVerifyLicenseAccepted() {
		String value = this.getProperty(KEY_VERIFY_LICENSE_ACCEPTED);
		if (value != null) {
			return this.getProperty(KEY_VERIFY_LICENSE_ACCEPTED).equalsIgnoreCase("true");
		}
		return false;
	}
}
