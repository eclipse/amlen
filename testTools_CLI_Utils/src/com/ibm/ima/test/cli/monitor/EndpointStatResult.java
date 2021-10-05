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

public class EndpointStatResult {
	
	private String name = null;
	private String ipAddr = null;
	private boolean enabled = false;
	private int total = 0;
	private int active = 0;
	private int messages = 0;
	private long bytes = 0;
	private String lastErrorCode = null;
	private Date configTime = null;
	private int badConnections = 0;
	
	public EndpointStatResult(String name, String ipAddr, boolean enabled, int total,
			int active, int messages, long bytes, String lastErrorCode, Date configTime, int badConnections)
	{
		this.name = name;
		this.ipAddr = ipAddr;
		this.enabled = enabled;
		this.total = total;
		this.active = active;
		this.messages = messages;
		this.bytes = bytes;
		this.lastErrorCode = lastErrorCode;
		this.configTime = configTime;
		this.badConnections = badConnections;
	}
	
	public String getName() {
		return name;
	}
	public String getIpAddr() {
		return ipAddr;
	}
	public boolean isEnabled() {
		return enabled;
	}
	public int getTotal() {
		return total;
	}
	public int getActive() {
		return active;
	}
	public int getMessages() {
		return messages;
	}
	public long getBytes() {
		return bytes;
	}
	public String getLastErrorCode() {
		return lastErrorCode;
	}
	public Date getConfigTime() {
		return configTime;
	}
	public int getBadConnections() {
		return badConnections;
	}

	public String toString()
	{
		StringBuffer buf = new StringBuffer();
		
		buf.append("Name=" + name + "\n");
		buf.append("IpAddr=" + ipAddr + "\n");
		buf.append("Enabled=" + enabled + "\n");
		buf.append("Total=" + total + "\n");
		buf.append("Active=" + active + "\n");
		buf.append("Messages=" + messages + "\n");
		buf.append("Bytes=" + bytes + "\n");
		buf.append("LastErrorCode=" + lastErrorCode + "\n");
		buf.append("configTime=" + configTime + "\n");
		buf.append("BadConnections=" + badConnections + "\n");
		
		return buf.toString();
	}

}
