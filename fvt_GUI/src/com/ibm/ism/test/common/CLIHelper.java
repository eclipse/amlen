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
package com.ibm.ism.test.common;

import java.util.ArrayList;

import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.util.CLICommand;

public class CLIHelper {
	
	public static void executeCommand(IsmTestData testData, String command) throws Exception {
		ArrayList<String> commands = new ArrayList<String>();
		commands.add(command);
		executeCommands(testData, commands);
	}
	
	public static void executeCommands(IsmTestData testData, ArrayList<String> commands) throws Exception {
		boolean connected = false;
		int commandsExecuted = 0;
		ImaConfig imaConfig = null;
		String sshHost = testData.getSSHHost();
		if (sshHost == null ) {
			sshHost = testData.getA1Host();
		}
		if (sshHost == null ) {
			sshHost = testData.getA2Host();
		}
		
		if (sshHost == null) {
			System.err.println("ERROR: Cannot determine host for ssh.");
			return;
		}
		
		String sshUser = testData.getSSHUser();
		if (sshUser == null) {
			sshUser = testData.getUser();
		}
		
		if (sshUser == null) {
			System.err.println("ERROR: Cannot determine user for ssh.");
			return;
		}
		
		String sshPassword = testData.getSSHPassword();
		if (sshPassword == null) {
			sshPassword = testData.getPassword();
		}
		
		if (sshPassword == null) {
			System.err.println("ERROR: Cannot determine user for ssh.");
			return;
		}
		try {
			
			imaConfig = new ImaConfig(sshHost, sshUser, sshPassword);
			imaConfig.connectToServer();
			connected = true;
			for (String command : commands) {
				String strs[] = command.split(" ");
				if (strs[0].equals("sleep")) {
					try {
						Integer sleep = new Integer(strs[1]);
						Thread.sleep(sleep.intValue());
					} catch (Exception e) {
					}
				} else {
					CLICommand cmd = imaConfig.executeCommand(command, null);
					commandsExecuted++;
					System.out.println("'" + cmd.getCommand() + "' command was executed.  The return code is " + cmd.getRetCode());
					printStdout(cmd);
					printStderr(cmd);
				}
			}
			imaConfig.disconnectServer(); 
		} catch (Exception e) {
			if (!connected) {
				System.err.println("Cannot connect to " + sshHost + " with " + sshUser + "//" + sshPassword);
			} else {
				System.err.println("Cannot execute command on " + sshHost + " with " + sshUser + "//" + sshPassword
						+ " Number of commands executed: " + commandsExecuted);
			}
			e.printStackTrace();
		}				
		
	}
	
	private static void printStdout(CLICommand cmd) {
		if (cmd.getStdoutList() != null) {
			for (String line : cmd.getStdoutList()) {
				System.out.println(line);
			}
		}		
	}
	
	private static void printStderr(CLICommand cmd) {
		if (cmd.getStderrList() != null) {
			for (String line : cmd.getStderrList()) {
				System.err.println(line);
			}
		}
	}
	
	public static void printIMAServerStatus(IsmTestData testData) throws Exception {
		executeCommand(testData, "status imaserver");
	}
	
	public static void printIMAServerVersion(IsmTestData testData) throws Exception {
		executeCommand(testData, "show imaserver");
	}

	public static void printWebUIHost(IsmTestData testData) throws Exception {
		executeCommand(testData, "imaserver get WebUIHost");
	}
	
	public static void printWebUIPort(IsmTestData testData) throws Exception {
		executeCommand(testData, "imaserver get WebUIPort");
	}
	
	public static void printMessageHubs(IsmTestData testData) throws Exception {
		executeCommand(testData, "imaserver list MessageHub");
	}
	
	public static void printMessageHub(IsmTestData testData, String name) throws Exception {
		executeCommand(testData, "imaserver show MessageHub Name=" + name);
	}
	
	public static void printEndpoints(IsmTestData testData) throws Exception {
		executeCommand(testData, "imaserver list Endpoint");
	}
	
	public static void printEndpoint(IsmTestData testData, String name) throws Exception {
		executeCommand(testData, "imaserver show Endpoint Name=" + name);
	}
	
	public static void printConnectionPolicy(IsmTestData testData, String name) throws Exception {
		executeCommand(testData, "imaserver show ConnectionPolicy Name=" + name);
	}
	
	public static void printConnectionPolicies(IsmTestData testData) throws Exception {
		executeCommand(testData, "imaserver list ConnectionPolicy");
	}
	
	public static void printMessagingPolicy(IsmTestData testData, String name) throws Exception {
		executeCommand(testData, "imaserver show MessagingPolicy Name=" + name);
	}
	
	public static void printMessagingPolicies(IsmTestData testData) throws Exception {
		executeCommand(testData, "imaserver list MessagingPolicy");
	}
	
	public static void printQueue(IsmTestData testData, String name) throws Exception {
		executeCommand(testData, "imaserver show Queue Name=" + name);
	}
	
	public static void printClusterMembership(IsmTestData testData) throws Exception {
		executeCommand(testData, "imaserver show ClusterMembership");
	}
	
	
	public static void changeRunModeToMaintenance(IsmTestData testData) throws Exception {
		ArrayList<String> commands = new ArrayList<String>();
		commands.add(new String("imaserver runmode maintenance"));
		commands.add(new String("status imaserver"));
		commands.add(new String("imaserver stop"));
		commands.add(new String("status imaserver"));
		commands.add(new String("imaserver status"));
		commands.add(new String("status imaserver"));
		executeCommands(testData, commands);
	}
	
	public static void changeRunModeToProduction(IsmTestData testData) throws Exception {
		ArrayList<String> commands = new ArrayList<String>();
		commands.add(new String("imaserver runmode production"));
		commands.add(new String("status imaserver"));
		commands.add(new String("imaserver stop"));
		commands.add(new String("status imaserver"));
		commands.add(new String("imaserver status"));
		commands.add(new String("status imaserver"));
		executeCommands(testData, commands);
	}
	
}
