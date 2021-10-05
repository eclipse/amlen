/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.test.cli.util;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/**
 * 
 * CLICommand is used to contain the input and output of a command
 * that is invoked via SSH.
 *
 */
public class CLICommand {
	
	private String command = null;
	private Map<String,String> options = null;
	private String output = null;
	private String stderr = null;
	private ArrayList<String> outputList = null;
	private ArrayList<String> stderrList = null;
	private int retCode = 0;
	private Throwable error = null;
	
	
	/**
	 * Create a default instance of CLICommand
	 */
	public CLICommand() {
		
	}
	
	/**
	 * Create an instance of CLICommand using a specific
	 * command that was executed
	 * 
	 * @param command  The command that was executed
	 */
	public CLICommand(String command) {
		this.command = command;
	}
	
	public void addOption(String key, String value) {
		if (options == null) {
			options = new HashMap<String, String>();
		}
		options.put(key, value);
	}
	
	public Map<String,String> getOptions() {
		return options;
	}

	public String getCommand() {
		return command;
	}

	public void setCommand(String command) {
		this.command = command;
	}

	public String getOutput() {
		return output;
	}
	
	public ArrayList<String> getOutputList() {
		return outputList;
	}
	
	public String getStdout() {
		return output;
	}

	public ArrayList<String> getStdoutList() {
		return outputList;
	}
	
	public String getStderr() {
		return stderr;
	}
	
	public ArrayList<String> getStderrList() {
		return stderrList;
	}

	public void setOutput(String output) {
		this.output = output;
	}
	
	public void setOutput(ArrayList<String> list) {
		outputList = list;
		if (output == null) {
			StringBuilder sb = new StringBuilder();
			int i = 0;
			for (String s : outputList) {
				sb.append(s);
				i++;
				if (i < outputList.size()) {
					sb.append(",");
				}
			}
			output = sb.toString();
		}
	}
	
	public void setStdout(String stdout) {
		output = stdout;
	}

	public void setStdout(ArrayList<String> list) {
		setOutput(list);
	}
	
	public void setStderr(String stderr) {
		this.stderr = stderr;
	}
	
	public void setStderr(ArrayList<String> list) {
		stderrList = list;
	}

	public int getRetCode() {
		return retCode;
	}

	public void setRetCode(int retCode) {
		this.retCode = retCode;
	}

	public Throwable getError() {
		return error;
	}

	/**
	 * If an error occurred while trying to invoke the command
	 * via SSH set it.
	 * 
	 * @param error  The error that occured
	 */
	public void setError(Throwable error) {
		this.error = error;
	}
	
	

}
