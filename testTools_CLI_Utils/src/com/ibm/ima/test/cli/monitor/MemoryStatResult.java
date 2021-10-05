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

public class MemoryStatResult {
	private long memoryTotalBytes= 0;
	private long memoryFreeBytes = 0;
	private double memoryFreePercent = 0;
	private long serverVirtualMemoryBytes = 0;
	private long serverResidentSetBytes = 0;
	private long messagePayloads = 0;
	private long publishSubscribe = 0;
	private long destinations = 0;
	private long currentActivity = 0;
	
	public MemoryStatResult(long memoryTotalBytes, long memoryFreeBytes, double memoryFreePercent, 
			long serverVirtualMemoryBytes, long serverResidentSetBytes, long messagePayloads,
			long publishSubscribe, long destinations, long currentActivity)
	{
		this.memoryTotalBytes = memoryTotalBytes;
		this.memoryFreeBytes = memoryFreeBytes;
		this.memoryFreePercent = memoryFreePercent;
		this.serverVirtualMemoryBytes = serverVirtualMemoryBytes;
		this.serverResidentSetBytes = serverResidentSetBytes;
		this.serverResidentSetBytes = serverResidentSetBytes;
		this.messagePayloads = messagePayloads;
		this.publishSubscribe = publishSubscribe;
		this.destinations = destinations;
		this.currentActivity = currentActivity;

	}

	
	public long getMemoryTotalBytes() {
		return memoryTotalBytes;
	}
	public long getMemoryFreeBytes() {
		return memoryFreeBytes;
	}
	public double getMemoryFreePercent() {
		return memoryFreePercent;
	}
	public long getServerVirtualMemoryBytes() {
		return serverVirtualMemoryBytes;
	}
	public long getServerResidentSetBytes() {
		return serverResidentSetBytes;
	}
	public long getMessagePayloads() {
		return messagePayloads;
	}
	public long getPublishSubscribe() {
		return publishSubscribe;
	}
	public long getDestinations() {
		return destinations;
	}
	public long getCurrentActivity() {
		return currentActivity;
	}


	public String toString()
	{
		StringBuffer buf = new StringBuffer();
		
		buf.append("MemoryTotalBytes=" + memoryTotalBytes + "\n");
		buf.append("MemoryFreeBytes=" + memoryFreeBytes + "\n");
		buf.append("MemoryFreePercent=" + memoryFreePercent + "\n");
		buf.append("ServerVirtualMemoryBytes=" + serverVirtualMemoryBytes + "\n");
		buf.append("ServerResidentSetBytes=" + serverResidentSetBytes + "\n");
		buf.append("MessagePayloads=" + messagePayloads + "\n");
		buf.append("PublishSubscribe=" + publishSubscribe + "\n");
		buf.append("Destinations=" + destinations + "\n");
		buf.append("CurrentActivity=" + currentActivity + "\n");
		
		return buf.toString();
	}
}
