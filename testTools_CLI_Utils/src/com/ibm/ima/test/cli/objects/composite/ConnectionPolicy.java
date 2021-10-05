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

public class ConnectionPolicy {

	private static String SEP =  "\n";

	private String name = null;
	private String description = null;
	private int testIndex = 0;
	private String clientId = null;
	private String clientAddress = null;

	private String userId = null;
	private String group = null;

	public static String TEST_INDEX = "__TEST_INDEX__";
	private String commonNames = null;
	private String protocol = null;
	private Endpoint endpoint = null;


	public ConnectionPolicy(String name, Endpoint endpoint, String protocol, String clientAddress) {
		this.name = name;
		this.protocol = protocol;
		this.clientAddress = clientAddress;
		this.endpoint = endpoint;
	}

	public  String substituteTestIndex(String value) {

		if (value == null) return value;


		String testIndexValue = value.replaceAll(TEST_INDEX, "" + testIndex);
		return testIndexValue;
	}

	public String buildCliCommand() {

		String cmd = "create ConnectionPolicy \"Name=" + name + "\""
				+ " \"Protocol=" + protocol + "\"";

		if (description != null) {
			cmd += " \"Description=" + description + "\"";
		}
		if (clientId != null) {
			cmd += " \"ClientID=" + clientId + "\"";
		}
		if (clientAddress != null) {
			cmd += " \"ClientAddress=" + clientAddress + "\"";
		}
		if (userId != null) {
			cmd += " \"UserID=" + userId + "\"";
		}
		if (group != null) {
			cmd += " \"GroupID=" + group + "\"";
		}
		if (commonNames != null) {
			cmd += " \"CommonNames=" + commonNames + "\"";
		}

		return cmd;
	}

	public String getCliUpdateCommand() {

		String cmd = "update ConnectionPolicy \"Name=" + name + "\""
				+ " \"Protocol=" + protocol + "\"";

		return cmd;

	}

	public String getCliDeleteCommand() {
		return "delete ConnectionPolicy \"Name=" + name + "\"";
	}

	public void writeConnectionPolicySet(String testSuiteName, String testCaseIdentifier, OutputStream zOut) throws IOException {
		
		String testCaseName = testSuiteName + "_" + testCaseIdentifier;
		String cPolicyCommand = testCaseName + " 0 " + buildCliCommand() + SEP;
		OutputStreamWriter out = new OutputStreamWriter(zOut,"utf-8");

		try {
			byte[] utf8Bytes = cPolicyCommand.getBytes("UTF8");
			cPolicyCommand = new String(utf8Bytes, "UTF8");
		} catch (Exception e) {
			e.printStackTrace();
		}

		out.write(cPolicyCommand);
		out.flush();

	}

	public  void writeConnectionPolicyUpdateSet(String testSuiteName, String testCaseIdentifier, OutputStream zOut) throws IOException {

		String testCaseName = testSuiteName + "_" + testCaseIdentifier;
		String cPolicyCommand = testCaseName + " 0 " + getCliUpdateCommand() + SEP;
		OutputStreamWriter out = new OutputStreamWriter(zOut,"utf-8");

		try {
			byte[] utf8Bytes = cPolicyCommand.getBytes("UTF8");
			cPolicyCommand = new String(utf8Bytes, "UTF8");
		} catch (Exception e) {
			e.printStackTrace();
		}

		out.write(cPolicyCommand);
		out.flush();
		
	}

	public void writeConnectionPolicyDelete(String testSuiteName, String testCaseIdentifier, OutputStream zOut) throws IOException {

		String testCaseName = testSuiteName + "_" + testCaseIdentifier;
		String cPolicyCommand = testCaseName + "_CLEAN 0 " + getCliDeleteCommand() + SEP;
		OutputStreamWriter out = new OutputStreamWriter(zOut,"utf-8");

		try {
			byte[] utf8Bytes = cPolicyCommand.getBytes("UTF8");
			cPolicyCommand = new String(utf8Bytes, "UTF8");
		} catch (Exception e) {
			e.printStackTrace();
		}
		
		out.write(cPolicyCommand);
		out.flush();

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


	public String getClientId() {
		return clientId;
	}


	public void setClientId(String clientId) {
		this.clientId = substituteTestIndex(clientId);
	}


	public String getClientAddress() {
		return clientAddress;
	}


	public void setClientAddress(String clientAddress) {
		this.clientAddress = clientAddress;
	}


	public String getUserId() {
		return userId;
	}


	public void setUserId(String userId) {
		this.userId = substituteTestIndex(userId);
	}


	public String getGroup() {
		return group;
	}


	public void setGroup(String group) {
		this.group = substituteTestIndex(group);
	}


	public String getCommonNames() {
		return commonNames;
	}


	public void setCommonNames(String commonNames) {
		this.commonNames = commonNames;
	}


	public String getProtocol() {
		return protocol;
	}


	public void setProtocol(String protocol) {
		this.protocol = protocol;
	}

	public Endpoint getEndpoint() {
		return endpoint;
	}

	public void setEndpoint(Endpoint endpoint) {
		this.endpoint = endpoint;
	}

	public int getTestIndex() {
		return testIndex;
	}




}
