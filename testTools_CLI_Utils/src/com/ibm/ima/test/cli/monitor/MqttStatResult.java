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

public class MqttStatResult {
	
	private String clientId = null;
	private boolean isConnected = false;
	private Date lastConnectedTime = null;
	
	
	public MqttStatResult(String clientId, boolean isConnected, Date lastConnectedTime)
	{
		this.clientId = clientId;
		this.isConnected = isConnected;
		this.lastConnectedTime = lastConnectedTime;
	}
	
	public String getClientId() {
		return clientId;
	}
	public boolean isConnected() {
		return isConnected;
	}
	public Date getLastConnectedTime() {
		return lastConnectedTime;
	}


	public String toString()
	{
		StringBuffer buf = new StringBuffer();
		buf.append("ClientId=" + clientId + "\n");
		buf.append("IsConnected=" + isConnected + "\n");
		buf.append("LastConnectedTime=" + lastConnectedTime + "\n");
		
		return buf.toString();
	}
}
