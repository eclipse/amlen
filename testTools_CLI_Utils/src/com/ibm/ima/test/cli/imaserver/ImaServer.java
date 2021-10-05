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
package com.ibm.ima.test.cli.imaserver;

public abstract class ImaServer extends ImaCliHelper{

	public ImaServer(String ipaddress, String username, String password) {
		super(ipaddress, username, password);
	}
	
	
	public ImaServer(String ipaddressA, String ipaddressB, String username, String password) {
		super(ipaddressA, ipaddressB, username, password);
	}
	
	public abstract boolean stopServer(long timeout, boolean force) throws Exception;
	public abstract boolean stopServer(long timeout, boolean force, boolean retry) throws Exception;
	public abstract boolean stopServer(long timeout, boolean force, boolean retry, long recoveryTime) throws Exception;
	public abstract void stopServer() throws Exception;
	public abstract void stopForceServer() throws Exception;
	public abstract void stopMQConnectivity() throws Exception;
	public abstract void stopForceMQConnectivity() throws Exception;
	public abstract boolean startServer(long timeout) throws Exception;
	public abstract void startServer() throws Exception;
	public abstract void startMQConnectivity() throws Exception;
	public abstract String showStatus() throws Exception;
	public abstract String showDetails() throws Exception;
	public abstract boolean isServerRunning() throws Exception;
	public abstract boolean isServerStopped() throws Exception;
	public abstract boolean isMQConnectivityStopped() throws Exception;
	public abstract boolean isMQConnectivityRunning() throws Exception;
	public abstract void gatherDebug(String fileName) throws Exception;
	public abstract String gatherStackTrace(String component) throws Exception;
	public abstract String gatherCoreDump(String component) throws Exception;
	public abstract String[] getFileNames() throws Exception; 
	public abstract boolean isServerInMaintenance() throws Exception;
	public abstract boolean isStatusUnknown() throws Exception;
	public abstract boolean isStatusFailover() throws Exception;

}
