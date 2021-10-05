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
package com.ibm.ima.test.cli.monitor;

public class StoreStatResult {
	
	private double memoryUsedPercent = 0;
	private double diskUsedPercent = 0;
	private long diskFreeBytes = 0;
	
	public StoreStatResult(double memoryUsedPercent, double diskUsedPercent, long diskFreeBytes)
	{
		this.memoryUsedPercent = memoryUsedPercent;
		this.diskUsedPercent = diskUsedPercent;
		this.diskFreeBytes = diskFreeBytes;
	}
	
	
	public double getMemoryUsedPercent() {
		return memoryUsedPercent;
	}
	public double getDiskUsedPercent() {
		return diskUsedPercent;
	}
	public long getDiskFreeBytes() {
		return diskFreeBytes;
	}


	public String toString()
	{
		StringBuffer buf = new StringBuffer();
		
		buf.append("MemoryUsedPercent=" + memoryUsedPercent + "\n");
		buf.append("DiskUsedPercent=" + diskUsedPercent + "\n");
		buf.append("DiskFreeBytes=" + diskFreeBytes + "\n");
		
		return buf.toString();
	}
}
