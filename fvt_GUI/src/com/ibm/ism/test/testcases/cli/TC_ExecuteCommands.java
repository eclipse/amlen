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
package com.ibm.ism.test.testcases.cli;

import com.ibm.ism.test.common.CLIHelper;


public class TC_ExecuteCommands {

	public static void main(String[] args) {
		TC_ExecuteCommands test = new TC_ExecuteCommands();
		System.exit(test.runTest(new ImsCLITestData(args)) ? 0 : 1);
	}
	public boolean runTest(ImsCLITestData testData) {
		boolean result = false;
		
		try {
			CLIHelper.executeCommands(testData, testData.getCommands());
			result = true;
		} catch (Exception e) {
			e.printStackTrace();
		}
		return result;
	}

}
