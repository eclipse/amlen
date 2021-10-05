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
package com.ibm.ism.ws.test;

import java.util.Properties;

public class TestProperties extends Properties {

	/**
	 * 
	 */
	private static final long serialVersionUID = 192837465L;
	
	public TestProperties() {
		super();
	}

	public String getPropertyOrEnv(String key) {
		String result = getProperty(key);
		if (null == result) {
			result = System.getenv(key);
		}
		return result;
	}
}
