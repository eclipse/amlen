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

public class MemoryRecord extends MonitorRecord {
	public final long memoryTotalBytes;
	public final long memoryFreeBytes;
	public final int memoryFreePercent;
	public final long serverVirtualMemoryBytes;
	public final long serverResidentSetBytes;
	public final long messagePayloads;
	public final long publishSubscribe;
	public final long destinations;
	public final long currentActivity;
	public final long clientStates;
	
	MemoryRecord(ImaJson json) throws IsmTestException {
		super(json);
		// Parse the rest
		memoryTotalBytes = Long.valueOf(json.getString("MemoryTotalBytes"));
		memoryFreeBytes = Long.valueOf(json.getString("MemoryFreeBytes"));
		memoryFreePercent = json.getInt("MemoryFreePercent", -1);
		serverVirtualMemoryBytes = Long.valueOf(json.getString("ServerVirtualMemoryBytes"));
		serverResidentSetBytes = Long.valueOf(json.getString("ServerResidentSetBytes"));
		messagePayloads = Long.valueOf(json.getString("MessagePayloads"));
		publishSubscribe = Long.valueOf(json.getString("PublishSubscribe"));
		destinations = Long.valueOf(json.getString("Destinations"));
		currentActivity = Long.valueOf(json.getString("CurrentActivity"));
		clientStates = Long.valueOf(json.getString("ClientStates"));
		//
	}
}
