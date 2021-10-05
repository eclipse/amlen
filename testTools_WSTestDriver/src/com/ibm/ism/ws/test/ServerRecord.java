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

public class ServerRecord extends MonitorRecord {
	public final long activeConnections;
	public final long totalConnections;
	public final long msgRead;
	public final long msgWrite;
	public final long bytesRead;
	public final long bytesWrite;
	public final long totalEndpoints;
	public final long badConnCount;
	
	ServerRecord(ImaJson json) throws IsmTestException {
		super(json);
		// Parse the rest
		activeConnections = Long.valueOf(json.getString("ActiveConnections"));
		totalConnections = Long.valueOf(json.getString("TotalConnections"));
		msgRead = Long.valueOf(json.getString("MsgRead"));
		msgWrite = Long.valueOf(json.getString("MsgWrite"));
		bytesRead = Long.valueOf(json.getString("BytesRead"));
		bytesWrite = Long.valueOf(json.getString("BytesWrite"));
		totalEndpoints = Long.valueOf(json.getString("TotalEndpoints"));
		badConnCount = Long.valueOf(json.getString("BadConnCount"));
		
	}
	//BadConnections
}
