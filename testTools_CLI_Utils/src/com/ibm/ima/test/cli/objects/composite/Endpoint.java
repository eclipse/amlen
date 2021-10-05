/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.test.cli.objects.composite;

import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.List;

/**
 * Endpoint can be used to model an Enpoint object. From an Endpoint
 * instance the commands needed to create, update or delete the Endpoint
 * can be obtained. The commands may be in the form of the real command
 * that would run on the server or the run_cli format.
 *
 */
public class Endpoint {

	private static String SEP =  "\n";

	private String name = null;
	private String description = null;
	private String enabled = "True";
	private String securityProfile = null;
	private String port = null;
	private String protocol = null;
	private String networkInterface = null;
	private String maxMsgSize = null; 
	private String hubName = null;
	private MessageHub hub = null;

	private String connPolicyNames = null;
	private String msgPolicyNames = null;

	private List<ConnectionPolicy> connPolicyList = null;
	private List<MessagingPolicy> msgPolicyList = null;

	private String testCaseName = null;
	private int exitCode = 0;


	/** 
	 * Default constructor. Create an empty Endpoint instance.
	 */
	public Endpoint() {

	}

	public Endpoint(String testCaseName, String name) {
		this.testCaseName = testCaseName;
		this.name = name;
	}

	/**
	 * Create a new Endpoint instance for a specific test case.
	 * 
	 * @param testCaseName    The name of the test case
	 * @param name            The name of the Endpoint
	 * @param port
	 * @param protocol
	 * @param connPolicyList  A list of Connection Policies 
	 * @param msgPolicyList
	 */
	public Endpoint(String testCaseName, 
			String name, 
			String port, 
			String protocol, 
			List<ConnectionPolicy> connPolicyList, 
			List<MessagingPolicy> msgPolicyList) {

		this.testCaseName = testCaseName;
		this.name = name;
		this.port = port;
		this.protocol = protocol;
		setConnPolicyList(connPolicyList);
		setMsgPolicyList(msgPolicyList);

	}

	/**
	 * Clear all attributes except the Name of the Endpoint. Useful
	 * if you wish to do an update.
	 * 
	 */
	public void reset() {

		description = null;
		enabled = null;
		securityProfile = null;
		port = null;
		protocol = null;
		networkInterface = null;
		maxMsgSize = null; 
		hubName = null;
		hub = null;
		connPolicyNames = null;
		msgPolicyNames = null;

		connPolicyList = null;
		msgPolicyList = null;


	}


	/**
	 * Obtain the CLI command to create an Enpoint that corresponds to
	 * this instance. All required parameters will be used and optional
	 * parameters will be checked for null before being used.
	 * 
	 * @return  A CLI command that can create an Endpont - and that 
	 *          corresponds to this instance.
	 *          
	 */
	private String getCLICreateUpdate(boolean update) {


		///////////////////////////////////////////////////////
		/**************** REQUIRED FOR CREATE ****************/
		///////////////////////////////////////////////////////

		StringBuffer cmdBuf = new StringBuffer();

		if (update) {
			cmdBuf.append("update Endpoint");
		} else {
			cmdBuf.append("create Endpoint");
		}

		// Only Name is required for update
		if (enabled != null) {
			cmdBuf.append(" \"Name=" + name + "\"");
		}

		if (enabled != null) {
			cmdBuf.append(" \"Port=" + port + "\"");
		}

		if (enabled != null) {
			cmdBuf.append(" \"Protocol=" + protocol + "\"");
		}

		if (connPolicyNames != null) {
			cmdBuf.append(" \"ConnectionPolicies=" + connPolicyNames + "\"");
		}

		if (msgPolicyNames != null) {
			cmdBuf.append(" \"MessagingPolicies=" + msgPolicyNames + "\"");
		}

		///////////////////////////////////////////////////////
		/************** END REQUIRED FOR CREATE **************/
		///////////////////////////////////////////////////////

		if (enabled != null) {
			cmdBuf.append(" \"Enabled=" + enabled + "\"");
		}

		if (hubName != null && !update) {
			cmdBuf.append(" \"MessageHub=" + hubName + "\"");
		}

		if (securityProfile != null) {
			cmdBuf.append(" \"SecurityProfile=" + securityProfile + "\"");
		}

		if (description != null) {
			cmdBuf.append(" \"Description=" + description + "\"");
		}

		if (networkInterface != null) {
			cmdBuf.append(" \"Interface=" + networkInterface + "\"");
		}

		if (maxMsgSize != null) {
			cmdBuf.append(" \"MaxMessageSize=" + maxMsgSize + "\"");
		}

		return cmdBuf.toString();

	}


	/**
	 * Get the base delete command
	 * 
	 * @return  the base part of the delete command
	 */
	private String getCliDeleteCommand() {
		return "delete Endpoint \"Name=" + name + "\"";
	}
	
	private void writeCliCommand(String cliCommand, OutputStream zOut) throws IOException {
		OutputStreamWriter out = new OutputStreamWriter(zOut,"utf-8");
		try {
			byte[] utf8Bytes = cliCommand.getBytes("UTF8");
			cliCommand = new String(utf8Bytes, "UTF8");
		} catch (Exception e) {
			e.printStackTrace();
		}

		out.write(cliCommand);
		out.flush();		
	}

	/**
	 * Write the appropriate create CLI command that is compatible 
	 * with run_cli.sh
	 * 
	 * @param out  The OutputStreamWrite where the command should be written to
	 * @throws IOException
	 */
	public void writeCliCreateCommand(OutputStream zOut) throws IOException {
		String cliCommand = testCaseName + " " + exitCode + " " + getCLICreateUpdate(false) + SEP;
		writeCliCommand(cliCommand, zOut);
	}
	
	/**
	 * Write the appropriate update CLI command that is compatible with 
	 * run_cli.sh
	 * 
	 * @param out  The OutputStreamWrite where the command should be written to
	 * @throws IOException
	 */
	public void writeCliUpdateCommand(OutputStream zOut) throws IOException {
		String cliCommand = testCaseName + " " + exitCode + " " + getCLICreateUpdate(true) + SEP;
		writeCliCommand(cliCommand, zOut);
	}
	
	/**
	 * Write the appropriate delete CLI command that is compatible 
	 * with run_cli.sh
	 * 
	 * @param out  The OutputStreamWrite where the command should be written to
	 * @throws IOException
	 */
	public  void writeCliDeleteCommand(OutputStream zOut) throws IOException {
		String cliCommand = testCaseName + "_CLEAN " + exitCode + " " + getCliDeleteCommand() + SEP;
		writeCliCommand(cliCommand, zOut);
	}
	
	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}

	public String getDescription() {
		return description;
	}

	public void setDescription(String description) {
		this.description = description;
	}

	public String getEnabled() {
		return enabled;
	}

	public void setEnabled(String enabled) {
		this.enabled = enabled;
	}

	public String getSecurityProfile() {
		return securityProfile;
	}

	public void setSecurityProfile(String securityProfile) {
		this.securityProfile = securityProfile;
	}

	public String getPort() {
		return port;
	}

	public void setPort(String port) {
		this.port = port;
	}

	public String getProtocol() {
		return protocol;
	}

	public void setProtocol(String protocol) {
		this.protocol = protocol;
	}

	public String getNetworkInterface() {
		return networkInterface;
	}

	public void setNetworkInterface(String networkInterface) {
		this.networkInterface = networkInterface;
	}

	public String getMaxMsgSize() {
		return maxMsgSize;
	}

	public void setMaxMsgSize(String maxMsgSize) {
		this.maxMsgSize = maxMsgSize;
	}

	public String getHubName() {
		return hubName;
	}

	public void setHubName(String hubName) {
		this.hubName = hubName;
	}

	public MessageHub getHub() {
		return hub;
	}

	public void setHub(MessageHub hub) {
		this.hub = hub;
	}

	public String getConnPolicyNames() {
		return connPolicyNames;
	}

	public void setConnPolicyNames(String connPolicyNames) {
		this.connPolicyNames = connPolicyNames;
	}

	public String getMsgPolicyNames() {
		return msgPolicyNames;
	}

	public void setMsgPolicyNames(String msgPolicyNames) {
		this.msgPolicyNames = msgPolicyNames;
	}

	public List<ConnectionPolicy> getConnPolicyList() {
		return connPolicyList;
	}

	public void setConnPolicyList(List<ConnectionPolicy> connPolicyList) {

		this.connPolicyList = connPolicyList;
		this.connPolicyNames = null;

		boolean writeComma = false;

		if (connPolicyList != null) {

			StringBuffer sBuf = new StringBuffer();

			for (ConnectionPolicy connPolicy : connPolicyList) {
				if (writeComma) {
					sBuf.append("," + connPolicy.getName());
				} else {
					sBuf.append(connPolicy.getName());
				}
				writeComma = true;
			}

			this.connPolicyNames = sBuf.toString();

		}

	}

	public void addConnPolicy(ConnectionPolicy connPolicy) {

		if (connPolicy == null) return;

		if (this.connPolicyList == null) {
			List<ConnectionPolicy> connPolicyList = new ArrayList<ConnectionPolicy>();
			connPolicyList.add(connPolicy);
			setConnPolicyList(connPolicyList);
		} else {
			this.connPolicyList.add(connPolicy);
			setConnPolicyList(this.connPolicyList);
		}

	}


	public List<MessagingPolicy> getMsgPolicyList() {
		return msgPolicyList;
	}

	/**
	 * Set a List of MessagingPolicy objects for the Endpoint
	 * 
	 * @param msgPolicyList  A List of MessagingPolicy objects
	 */
	public void setMsgPolicyList(List<MessagingPolicy> msgPolicyList) {

		this.msgPolicyList = msgPolicyList;
		this.msgPolicyNames = null;

		boolean writeComma = false;

		if (msgPolicyList != null) {

			StringBuffer sBuf = new StringBuffer();

			for (MessagingPolicy msgPolicy : msgPolicyList) {
				if (writeComma) {
					sBuf.append("," + msgPolicy.getName());
				} else {
					sBuf.append(msgPolicy.getName());
				}
				writeComma = true;
			}

			this.msgPolicyNames = sBuf.toString();

		}

	}

	public void addMsgPolicy(MessagingPolicy msgPolicy) {

		if (msgPolicy == null) return;

		if (this.msgPolicyList == null) {
			List<MessagingPolicy> msgPolicyList = new ArrayList<MessagingPolicy>();
			msgPolicyList.add(msgPolicy);
			setMsgPolicyList(msgPolicyList);
		} else {
			this.msgPolicyList.add(msgPolicy);
			setMsgPolicyList(this.msgPolicyList);
		}

	}

	public String getTestCaseName() {
		return testCaseName;
	}

	public void setTestCaseName(String testCaseName) {
		this.testCaseName = testCaseName;
	}

	public int getExitCode() {
		return exitCode;
	}

	public void setExitCode(int exitCode) {
		this.exitCode = exitCode;
	}


}
