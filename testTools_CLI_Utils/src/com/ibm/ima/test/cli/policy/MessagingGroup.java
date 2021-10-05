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

package com.ibm.ima.test.cli.policy;

import java.io.IOException;
import java.io.OutputStream;


public class MessagingGroup {
	
	private static String SEP =  "\n";
	
	private String id;
	private String description = null;
	

	
	public MessagingGroup(String id) {
		this.id = id;
	}
	

	
	public String getID() { return id; }
	public String getDescription() { return description; }

	
	public String getCliAddCommand() {
		String cmd = "group add \"GroupID=" + id + "\"";
		if (description != null) {
			cmd = cmd + " \"Description=" + description + "\"";
		}

			//cmd = cmd + " \"Group=" + id + "\"";
		
		return cmd;
	}
	
	public String getCliDeleteCommand() {
		return "group delete \"GroupID=" + id + "\"";
	}
	
	public  void writeCliAddCommand(String testSuiteName, String testCaseIdentifier, OutputStream out) throws IOException {
		
		String testCaseName = testSuiteName + "_" + testCaseIdentifier;
			String groupCommand = testCaseName + " 0 " + getCliAddCommand() + SEP;
	    	 byte[] byteArray = groupCommand.getBytes();
	         out.write(byteArray);
	}
	
	public  void writeCliDeleteCommand(String testSuiteName, String testCaseIdentifier, OutputStream out) throws IOException {
		
		String testCaseName = testSuiteName + "_" + testCaseIdentifier;
			String groupCommand = testCaseName + "_CLEAN 0 " + getCliDeleteCommand() + SEP;
	    	 byte[] byteArray = groupCommand.getBytes();
	         out.write(byteArray);
	}
	
	
}
