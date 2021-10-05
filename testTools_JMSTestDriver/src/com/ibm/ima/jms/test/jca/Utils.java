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
package com.ibm.ima.jms.test.jca;

public final class Utils {
	private Utils() {}
	
	public static boolean isWindows() {
		final String OS = System.getProperty("os.name");
		System.out.println("OS: " + OS);
		return OS.contains("indows");
	}
	
	public static String WASPATH() {
		if (isWindows())
			return "C:/cygwin/WAS";
		else
			return "/WAS";
	}
}
