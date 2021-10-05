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
package com.ibm.ism.tools.msg;

import java.util.ArrayList;
import java.util.List;

import com.ibm.ism.tools.msg.ISMLogEntryObject.LOG_ENTRY_COMPONENT_TYPE;
import com.ibm.ism.tools.msg.tms.ISMTMSManager;



public class ISMMsgTool {


	public static ISMLogFileParser util = new ISMLogFileParser();

	public static void main(String args[]) {

		String componentNames = null;

		boolean isGetMsgNumberRange = false;
		boolean isGetMsgAvail = false;
		boolean isValidateTMS=false;
		String src_location=null;
		int numberofMsgAvailReq = 1;
		List<LOG_ENTRY_COMPONENT_TYPE> inputTypes = new ArrayList<LOG_ENTRY_COMPONENT_TYPE>();
		if (args.length == 0) {
			printUsage();
			return;
		}
		for (int i = 0; i < args.length; i++) {

			if (args[i].equals("-component")) {
				componentNames = args[++i];
			} else if (args[i].equals("-src")) {
				src_location = args[++i];

			} else if (args[i].equals("-TMSvalidate")) {
				isValidateTMS = true;

			} else if (args[i].equals("-TMSautoadd")) {
				ISMTMSManager.setAutoAdd(true);

			} else if (args[i].equals("-getMsgNumRange")) {
				isGetMsgNumberRange = true;

			} else if (args[i].equals("-getAvailMsgId")) {
				isGetMsgAvail=true;
				try {
					numberofMsgAvailReq = Integer.parseInt(args[++i]);
				} catch (Exception e) {
					numberofMsgAvailReq = 1;
				}
			}
		}
		if (componentNames == null || src_location ==null) {
			printUsage();
			return;
		} else {
			Constants.SRC_ROOT = src_location;
			System.out.println("SRC_ROOT: "+ Constants.SRC_ROOT);

			if ("all".equalsIgnoreCase(componentNames)) {
				componentNames = Constants.ALL_COMPONENTS_STR;
			}
			String[] components = componentNames.split(";");
			LOG_ENTRY_COMPONENT_TYPE ctype = null;
			for (String componentName : components) {

				if (componentName.equals("TRANSPORT")) {
					ctype = LOG_ENTRY_COMPONENT_TYPE.TRANSPORT;

				}else if (componentName.equals("UTILS")) {
					ctype = LOG_ENTRY_COMPONENT_TYPE.UTILS;

				}else if (componentName.equals("PROTOCOL")) {
					ctype = LOG_ENTRY_COMPONENT_TYPE.PROTOCOL;

				}else if (componentName.equals("ENGINE")) {
					ctype = LOG_ENTRY_COMPONENT_TYPE.ENGINE;

				}else if (componentName.equals("STORE")) {
					ctype = LOG_ENTRY_COMPONENT_TYPE.STORE;

				}else if (componentName.equals("ADMIN")) {
					ctype = LOG_ENTRY_COMPONENT_TYPE.ADMIN;

				}else if (componentName.equals("GUI")) {
					ctype = LOG_ENTRY_COMPONENT_TYPE.GUI;

				}else if (componentName.equals("MONITORING")) {
					ctype = LOG_ENTRY_COMPONENT_TYPE.MONITORING;

				}else if (componentName.equals("MQCONNECTIVITY")) {
					ctype = LOG_ENTRY_COMPONENT_TYPE.MQCONNECTIVITY;

				} else if (componentName.equals("PROXY")) {
					ctype = LOG_ENTRY_COMPONENT_TYPE.PROXY;

				} else if (componentName.equals("NONE")) {
					ctype = LOG_ENTRY_COMPONENT_TYPE.NONE;

				} else {
					System.err.println("ERROR: INVALID COMPONENT. The valid components are TRANSPORT, UTILS, PROTOCOL, ENGINE, STORE, ADMIN, MQCONNECTIVITY, PROXY.");
					System.exit(0);
				}
				inputTypes.add(ctype);
				util.parseComponentsFiles(ctype);

				if (isGetMsgAvail) {
					List<String> availMsgList = util.getAvailableMsgKey(numberofMsgAvailReq,
							ctype);
					System.out.println();
					System.out.println("Available Msg # for Component: "
							+ ctype.name());

					for (String availentry : availMsgList) {

						System.out.println("\tAvail Msg #: "+availentry );
					}

				}
				if (isGetMsgNumberRange) {
					System.out.println();
					System.out.println(ctype.name()
							+ " Component Msg Number Range: "
							+ util.getComponentMsgNumberRange(ctype));
				}



			}

			if(isValidateTMS){
				util.validateAllTMS();
			}
			util.combinTMSFiles();
		}

	}

	static void printUsage() {
		System.out.println();
		System.out
				.println("Usage: java -cp <msgtooljarpath> com.ibm.llm.tools.msg.LLMMsgTool -src <source_location> -component <componentnames> <commands>");
		System.out.println("where: ");
		System.out.println("src\t\t- Required. Path to the directory contains all the components");
		System.out.println("componentnames\t\t- Required. Either all or TRANSPORT. Use ';' to specify multiple components.");
		System.out.println("commands\t\t- can be the following:");
		System.out.println("\t-getMsgNumRange : get the message number range of the component");
		System.out.println("\t-getAvailMsgId <numrequestids> : get the next available id(s). 'numrequestids' are the number of msg ids that you to get. Default to 1 if numrequestids is not specified.");
		System.out.println("\t-TMSvalidate : validate the log entries with TMS XML files");
		System.out.println("\t-TMSautoadd : Automatically add a <Message> block in the TMS file for a log entry.");
	}

}
