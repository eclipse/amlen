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
package com.ibm.ima.mqcon.utils;

public class StaticUtilities {

	/**
	 * pauses execution for specified time in ms
	 * handles InterruptedException
	 * @param sleepTime
	 */
	public static void sleep (long sleepTime) {
		try {
			Thread.sleep(sleepTime);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public static void printSuccess() {
		System.out.println("'Test.*Success!'");
		System.exit(0);
	}
	
	public static void printFail(){
		System.out.println("Test has FAILED");
		System.exit(1);
	}
}
