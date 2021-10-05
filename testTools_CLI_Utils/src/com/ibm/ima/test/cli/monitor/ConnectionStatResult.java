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

import java.util.Date;

public class ConnectionStatResult {
	
	
	private String name = null;
	private String protocol = null;
	private String clientAddr = null;
	private String userId = null;
	private String endpoint = null;
	private int port = 0;
	private Date connectTime = null;
	private long duration = 0;
	private long readBytes = 0;
	private long readMsg = 0;
	private long writebytes = 0;
	private long writeMsg = 0;
	
	public ConnectionStatResult(String name, String protocol, String clientAddr, String userId,
			String endpoint, int port, Date connectTime, long duration, long readBytes, long readMsg, 
			long writebytes, long writeMsg)
	{
		this.name = name;
		this.protocol = protocol;
		this.clientAddr = clientAddr;
		this.userId = userId;
		this.port = port;
		this.connectTime = connectTime;
		this.duration = duration;
		this.readBytes = readBytes;
		this.readMsg = readMsg;
		this.writebytes = writebytes;
		this.writeMsg = writeMsg;
	}
	
	
	public String getName() {
		return name;
	}
	public String getProtocol() {
		return protocol;
	}
	public String getClientAddr() {
		return clientAddr;
	}
	public String getUserId() {
		return userId;
	}
	public int getPort() {
		return port;
	}
	public Date getConnectTime() {
		return connectTime;
	}
	public long getDuration() {
		return duration;
	}
	public long getReadBytes() {
		return readBytes;
	}
	public long getReadMsg() {
		return readMsg;
	}
	public long getWritebytes() {
		return writebytes;
	}
	public long getWriteMsg() {
		return writeMsg;
	}
	
	public String getEndpoint()
	{
		return endpoint;
	}
	
	
	public String toString()
	{
		StringBuffer buf = new StringBuffer();
		
		buf.append("Name=" + name + "\n");
		buf.append("Protocol=" + protocol + "\n");
		buf.append("ClientAddr=" + clientAddr + "\n");
		buf.append("UserId=" + userId + "\n");
		buf.append("Endpoint=" + endpoint + "\n");
		buf.append("Port=" + port + "\n");
		buf.append("ConnectTime=" + connectTime + "\n");
		buf.append("Duration=" + duration + "\n");
		buf.append("ReadBytes=" + readBytes + "\n");
		buf.append("ReadMsg=" + readMsg + "\n");
		buf.append("Writebytes=" + writebytes + "\n");
		buf.append("WriteMsg=" + writeMsg + "\n");
	
		return buf.toString();
	}


}
