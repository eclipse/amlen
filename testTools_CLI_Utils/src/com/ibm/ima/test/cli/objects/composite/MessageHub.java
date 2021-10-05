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

public class MessageHub {

	private static String SEP =  "\n";

	private String name = "HUB";
	private String description = null;

	private List<Endpoint> endpoints = new ArrayList<Endpoint>();


	public MessageHub (String name, String description) {
		this.name = name;
		this.description = description;
	}

	public String getName() { 
		return name; 
	}

	public String getDescription() { 
		return description; 
	}

	public void addEndpoint(Endpoint endpoint) {
		endpoints.add(endpoint);
	}

	public List<Endpoint> getEndpoints() {
		return this.endpoints;
	}

	public String getCliSetCommand() {
		String cmd = "create MessageHub \"Name=" + name + "\"";
		if (description != null) {
			cmd = cmd + " \"Description=" + description + "\"";
		}
		return cmd;
	}

	public String getCliUpdateCommand() {

		String cmd = "update MessageHub \"Name=" + name + "\""
				+ " \"Description=" + description + "-updated\"";

		return cmd;

	}

	public String getCliDeleteCommand() {
		return "delete MessageHub \"Name=" + name + "\"";
	}

	public void writeHubSet(String testSuiteName, String testCaseIdentifier, OutputStream zOut) throws IOException {

		String testCaseName = testSuiteName + "_" + testCaseIdentifier;
		String hubCommand = testCaseName + " 0 " +  getCliSetCommand() + SEP;
		OutputStreamWriter out = new OutputStreamWriter(zOut,"utf-8");

		try {
			byte[] utf8Bytes = hubCommand.getBytes("UTF8");
			hubCommand = new String(utf8Bytes, "UTF8");
		} catch (Exception e) {
			e.printStackTrace();
		}

		out.write(hubCommand);
		out.flush();

	}

	public  void writeHubUpdateSet(String testSuiteName, String testCaseIdentifier, OutputStream zOut) throws IOException {

		String testCaseName = testSuiteName + "_" + testCaseIdentifier;
		String hubCommand = testCaseName + " 0 " + getCliUpdateCommand() + SEP;
		OutputStreamWriter out = new OutputStreamWriter(zOut,"utf-8");

		try {
			byte[] utf8Bytes = hubCommand.getBytes("UTF8");
			hubCommand = new String(utf8Bytes, "UTF8");
		} catch (Exception e) {
			e.printStackTrace();
		}

		out.write(hubCommand);
		out.flush();

	}

	public void writeHubDelete(String testSuiteName, String testCaseIdentifier, OutputStream zOut) throws IOException {
		
		String testCaseName = testSuiteName + "_" + testCaseIdentifier;
		String hubCommand = testCaseName + "_CLEAN 0 " +  getCliDeleteCommand() + SEP;
		OutputStreamWriter out = new OutputStreamWriter(zOut,"utf-8");

		try {
			byte[] utf8Bytes = hubCommand.getBytes("UTF8");
			hubCommand = new String(utf8Bytes, "UTF8");

		} catch (Exception e) {
			e.printStackTrace();
		}

		out.write(hubCommand);
		out.flush();

	}

}
