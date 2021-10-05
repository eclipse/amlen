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
package com.ibm.ism.test.testcases.cli;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;

import com.ibm.ism.test.common.IsmTestData;

public class ImsCLITestData extends IsmTestData {
	
	public static final String COMMAND_LIST_FILE = "COMMAND_LIST_FILE";
	public static final String LOCALE = "LOCALE";

	public ImsCLITestData(String[] args) {
		super(args);
	}
	
	public String getLocale() {
		return getProperty(LOCALE);
	}
	
	public ArrayList<String> getCommands() throws Exception {
		
		String commandListFile = getProperty(COMMAND_LIST_FILE);
		
		if (commandListFile == null) {
			return getAllCommands();
		}
		FileInputStream fstream = new FileInputStream(commandListFile);
		BufferedReader br = new BufferedReader(new InputStreamReader(fstream));

		ArrayList<String> commands = new ArrayList<String>();
		String strLine;
		while ((strLine = br.readLine()) != null){
			commands.add(strLine);
		}
		br.close();
		return commands;
	}
	
	public ArrayList<String> getAllCommands() {
		ArrayList<String> commands = new ArrayList<String>();
		commands.add("locale set " + getLocale());
		commands.add("locale get ");
		commands.add("help commands");
		commands.add("help show");
		commands.add("help status");
		commands.add("mq help");
		commands.add("status volume");
		commands.add("status voltage");
		commands.add("status uptime");
		commands.add("help status raid");
		commands.add("status raid all");
		commands.add("status raid foreign-config");
		commands.add("status raid battery");
		commands.add("status nvdimm");
		commands.add("status battery");
		commands.add("status memory");
		commands.add("status imaserver");
		commands.add("status flash");
		commands.add("status fan");
		commands.add("status ethernet-interface");
		commands.add("show components");
		commands.add("show properties");
		commands.add("show types");
		commands.add("show version");
		commands.add("help datetime");
		commands.add("datetime get");
		commands.add("set-ntp-server");
		commands.add("get-ntp-server");
		commands.add("set-ntp-server example.com");
		commands.add("get-ntp-server");
		commands.add("set-ntp-server");
		commands.add("get-ntp-server");
		commands.add("timezone set UTC");
		commands.add("timezone get");
		commands.add("show timezones");
		commands.add("get-dns-servers");
		commands.add("set-dns-servers example.com example.org");
		commands.add("get-dns-servers");
		commands.add("set-dns-servers");
		commands.add("get-dns-servers");
		
		commands.add("imaserver set webuihost 192.0.2.0");
		commands.add("imaserver apply Certificate \"TrustedCertificate=TVT.pem\" \"SecurityProfileName=TVT\"");
		commands.add("imaserver apply Certificate \"CertFileName=TVT.pem\" \"CertFilePassword=TVT\" \"KeyFileName=TVT.pem\" \"KeyFilePassword=test\"");
		
		commands.add("imaserver apply Certificate \"TrustedCertificate=TVT.pem\" \"SecurityProfileName=TVT\"");
		commands.add("firmware upgrade http://test.com");
		commands.add("imaserver set \"SNMPAgentAddress=333.4444.22.33\"");
		
		commands.add("help imaserver");
		
		commands.add("imaserver backup help");
		commands.add("imaserver close help");
		commands.add("imaserver commit help");
		commands.add("imaserver create help");
		commands.add("imaserver delete help");
		commands.add("imaserver forget help");
		commands.add("imaserver get help");
		commands.add("imaserver group help");
		commands.add("imaserver harole help");
		commands.add("imaserver help");
		commands.add("imaserver list help");
		commands.add("imaserver purge help");
		commands.add("imaserver restore help");
		commands.add("imaserver rollback help");
		commands.add("imaserver runmode help");
		
		commands.add("imaserver set help");
		commands.add("imaserver set LogLevel help");
		commands.add("imaserver set ConnectionLog help");
		commands.add("imaserver set SecurityLog help");
		commands.add("imaserver set AdminLog help");
		commands.add("imaserver set MQConnectivityLog help");
		commands.add("imaserver set WebUIPort help");
		commands.add("imaserver set WebUIHost help");
		commands.add("imaserver set SSHHost help");
		commands.add("imaserver set FIPS help");
		commands.add("imaserver set PluginDebugServer help");
		commands.add("imaserver set PluginRemoteDebugPort help");
		commands.add("imaserver set PluginMaxHeapSize help");
		commands.add("imaserver set EnableDiskPersistence help");
		commands.add("imaserver set SNMPEnabled help");
		commands.add("imaserver set SNMPAgentAddress help");

		commands.add("imaserver show help");
		commands.add("imaserver start help");
		commands.add("imaserver status help");
		commands.add("imaserver stop help");
		commands.add("imaserver test help");
		commands.add("imaserver trace help");
		commands.add("imaserver update help");
		commands.add("imaserver user help");
		
		return commands;
	}
	
}
