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
import java.util.ArrayList;

public class MessagingUser {
	private String id;
	private String password;
	private String description = null;
	private ArrayList<MessagingGroup> groups = new ArrayList<MessagingGroup>();
	public static final String USER_TYPE = "Messaging";
	private static String SEP =  "\n";

	
	public MessagingUser(String id, String password) {
		this.id = id;
		this.password = password;
	}
	
	public MessagingUser(String id, String password, String description) {
		this.id = id;
		this.password = password;
		this.description = description;
	}
	
	public String getID() { 
		return id; 
	}
	
	public String getPassword() { 
		return password; 
	}
	
	public String getDescription() { 
		return description; 
	}
	
	public ArrayList<MessagingGroup> getGroups() { 
		return groups; 
	}
	
	public void addGroup(MessagingGroup group) {
		groups.add(group);
	}
	
	/**
	 * Return the CLI command to add the user in the following format:
	 *    user add "Name=<id>" "UserType=messaging" "password=<password>" "description=<description>" "group=<group1>"...
	 * @return
	 */
	public String getCliAddCommand() {
		String cmd = "user add \"UserId=" + id + "\""
						+ " \"Password=" + password + "\"" 
						+ " \"Type=Messaging\"";
		if (description != null) {
			cmd = cmd + " \"Description=" + description + "\"";
		}
		for (MessagingGroup group : groups) {
			cmd = cmd + " \"GroupMembership=" + group.getID() + "\"";
		}
		
		return cmd;
	}
	
	public String getCliDeleteCommand() {
		return "user delete \"UserId=" + id + "\"" + " \"Type=messaging\"";
	}
	
	public void writeAddCommand(String testSuiteName, String testCaseIdentifier, OutputStream out) throws IOException {
		
		String testCaseName = testSuiteName + "_" + testCaseIdentifier;
		String groupCommand = testCaseName + " 0 " + getCliAddCommand() + SEP;
		byte[] byteArray = groupCommand.getBytes();
		out.write(byteArray);
		
	}
	
	public void writeDeleteCommand(String testSuiteName, String testCaseIdentifier, OutputStream out) throws IOException {
		
		String testCaseName = testSuiteName + "_" + testCaseIdentifier;
		String groupCommand = testCaseName + "_CLEAN 0 " + getCliDeleteCommand() + SEP;
		byte[] byteArray = groupCommand.getBytes();
		out.write(byteArray);
		
	}

}
