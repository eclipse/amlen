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
package com.ibm.ima.mqcon.rules;

import com.ibm.ima.mqcon.utils.StaticUtilities;

public class MqTopicToImaTopicWithBadRule {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		boolean passed = MqTopicMqJavaToImaTopicMqttTest.run(args);		
		if (passed) {
			StaticUtilities.printSuccess();
		} else {
			StaticUtilities.printFail();
		}
	}

}
