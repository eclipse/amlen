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

public class ServerStatResult {
	
	private int activeConnections = 0;
	private int totalConnections = 0;
	private long msgRead = 0;
	private long msgWrite = 0;
	private long bytesRead = 0;
	private long bytesWrite = 0;
	private int badConnCount = 0;
	private int totalEndpoints = 0;
	
	public ServerStatResult(int activeConnections,int totalConnections, long msgRead, long msgWrite,
			long bytesRead, long bytesWrite, int badConnCount, int totalEndpoints)
	{
		this.activeConnections = activeConnections;
		this.totalConnections = totalConnections;
		this.msgRead = msgRead;
		this.msgWrite = msgWrite;
		this.bytesRead = bytesRead;
		this.bytesWrite = bytesWrite;
		this.badConnCount = badConnCount;
		this.totalEndpoints = totalEndpoints;
	}
	
	public int getActiveConnections() {
		return activeConnections;
	}
	public int getTotalConnections() {
		return totalConnections;
	}
	public long getMsgRead() {
		return msgRead;
	}
	public long getMsgWrite() {
		return msgWrite;
	}
	public long getBytesRead() {
		return bytesRead;
	}
	public long getBytesWrite() {
		return bytesWrite;
	}
	public int getBadConnCount() {
		return badConnCount;
	}
	public int getTotalEndpoints() {
		return totalEndpoints;
	}


	public String toString()
	{
		StringBuffer buf = new StringBuffer();
		buf.append("activeConnections" + activeConnections + "\n");
		buf.append("totalConnections" + totalConnections + "\n");
		buf.append("msgRead" + msgRead + "\n");
		buf.append("msgWrite" + msgWrite + "\n");
		buf.append("bytesRead" + bytesRead + "\n");
		buf.append("bytesWrite" + bytesWrite + "\n");
		buf.append("badConnCount" + badConnCount + "\n");
		buf.append("totalEndpoints" + totalEndpoints + "\n");
	
		return buf.toString();
	}
}
