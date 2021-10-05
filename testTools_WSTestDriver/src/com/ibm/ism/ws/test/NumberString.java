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

public class NumberString {
	final String partialName;
	final int digits;
	int value = 0;

	NumberString(String partialName, int digits) {
		this.partialName = partialName;
		this.digits = digits;
	}

	NumberString(String partialName, int digits, int initialValue) {
		this.partialName = partialName;
		this.digits = digits;
		this.value = initialValue;
	}

	public String getName(int counter) {
		String count = "000000000"+counter;
		count = count.substring(count.length() - digits);
		return count;
	}
	
	public String getName() {
		String count = "000000000"+value;
		count = count.substring(count.length() - digits);
		return partialName+count;
	}
	
	public String incrementAndGetName() {
		String count = "000000000"+ (++value);
		count = count.substring(count.length() - digits);
		return partialName+count;
	}
	
	public void setNumber(int value) {
		this.value = value;
	}
	
	public String toString() {
		String result = "NumberString: {\n";
		result += "\tpartialName="+partialName+"\n";
		result += "\tdigits="+digits+"\n";
		result += "\tvalue="+value+"\n";
		return result;
	}
}
