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
package com.ibm.ima.test.cli.util;

import java.io.FileInputStream;
import java.util.ArrayList;
import java.util.Properties;

import com.ibm.ima.test.cli.config.ImaConfig;

public class CLICommandTest {
	private Properties props;
	private Properties propsInputCommand = null;
	private ArrayList<CLICommand> inputCommandList = null;
	private ArrayList<CLICommand> outputCommandList = null;
	
	public static final String KEY_HOST = "HOST";
	public static final String KEY_USER = "USER";
	public static final String KEY_PASSWORD = "PASSWORD";
	public static final String KEY_INPUT_PROPERTIES_FQFN = "INPUT_PROPERTIES_FQFN";
	public static final String KEY_OUTPUT_PROPERTIES_FQFN = "OUTPUT_PROPERTIES_FQFN";

	
	public CLICommandTest(String[] args) {
		props = new Properties();
		for (String arg : args) {
			String strs[] = arg.split("=");
			props.setProperty(strs[0], strs[1]);
		}
	}
	
	public ArrayList<CLICommand> getInputCommandList() {
		return inputCommandList;
	}
	
	public ArrayList<CLICommand> getOutputCommandList() {
		return outputCommandList;
	}
	
	public void addInputCommand(CLICommand cliCommand) {
		if (inputCommandList == null) {
			inputCommandList = new ArrayList<CLICommand>();
		}
		inputCommandList.add(cliCommand);
	}
	
	public void addOutputCommand(CLICommand cliCommand) {
		if (outputCommandList == null) {
			outputCommandList = new ArrayList<CLICommand>();
		}
		outputCommandList.add(cliCommand);
	}
	
	public ArrayList<CLICommand> buildInputCommands() throws Exception {
		for (int commandNumber = 1; commandNumber <= getInputCommandCount(); commandNumber++) {
			CLICommand cliCommand = new CLICommand(getInputCommand(commandNumber));
			for (int optionNumber = 1; optionNumber <= getInputCommandOptionCount(commandNumber); optionNumber++) {
				cliCommand.addOption(getInputCommandOptionKey(commandNumber, optionNumber),
						this.getInputCommandOptionValue(commandNumber, optionNumber));
			}
			addInputCommand(cliCommand);
		}
		return getInputCommandList();
	}
	
	public ArrayList<CLICommand> exececuteCommands() throws Exception {
		ImaConfig imaConfig = new ImaConfig(getHost(), getUser(), getPassword());
		imaConfig.connectToServer();
		for (CLICommand cliCommand : inputCommandList) {
			CLICommand cmd = imaConfig.executeCommand(cliCommand.getCommand(), cliCommand.getOptions());
			addOutputCommand(cmd);
		}
		imaConfig.disconnectServer();
		return getOutputCommandList();
	}

	public void setProperty(String key, String value) {
		props.setProperty(key, value);
	}
	
	public String getProperty(String key) {
		return props.getProperty(key);
	}
	
	public String getHost() {
		return getProperty(KEY_HOST);
	}

	public String getUser() {
		return getProperty(KEY_USER);
	}

	public String getPassword() {
		return getProperty(KEY_PASSWORD);
	}
	
	public String getInputPropertiesFQFN() {
		return getProperty(KEY_INPUT_PROPERTIES_FQFN);
	}
	
	public Properties getInputCLICommandProperties() throws Exception {
		if (propsInputCommand != null) {
			return propsInputCommand;
		}
		propsInputCommand = new Properties();
		propsInputCommand.load(new FileInputStream(getInputPropertiesFQFN()));
		return propsInputCommand;
	}
	
	public int getInputCommandCount() throws Exception {
		String s = getInputCLICommandProperties().getProperty("CLICommandCount");
		return new Integer(s).intValue();
	}
	
	public String getInputCommand(int commandNumber) throws Exception {
		return getInputCLICommandProperties().getProperty(
				"CLICommand." + commandNumber + ".Input.Command");
	}
	
	public int getInputCommandOptionCount(int commandNumber) throws Exception {
		String s = getInputCLICommandProperties().getProperty(
				"CLICommand." + commandNumber + ".Input.Option.Count");
		return new Integer(s).intValue();
	}
	
	public String getInputCommandOptionKey(int commandNumber, int optionNumber) throws Exception {
		return getInputCLICommandProperties().getProperty(
				"CLICommand." + commandNumber + ".Input.Option." + optionNumber + ".Key");
	}

	public String getInputCommandOptionValue(int commandNumber, int optionNumber) throws Exception {
		return getInputCLICommandProperties().getProperty(
				"CLICommand." + commandNumber + ".Input.Option." + optionNumber + ".Value");
	}
	
	public String getOutputPropertiesFQFN() {
		return getProperty(KEY_OUTPUT_PROPERTIES_FQFN);
	}
	
}
