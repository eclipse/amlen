// Copyright (c) 2009-2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
package com.ibm.ism.tools.msg;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.TreeSet;
import java.util.logging.Level;
import java.util.logging.Logger;

import com.ibm.ism.tools.msg.ISMLogEntryObject.LOG_ENTRY_COMPONENT_TYPE;
import com.ibm.ism.tools.msg.ISMLogEntryObject.LOG_POINT_TYPE;
import com.ibm.ism.tools.msg.impl.ISMLogEntryParserImpl;
import com.ibm.ism.tools.msg.impl.ISMLogEntryParserUtil;
import com.ibm.ism.tools.msg.logger.LogManager;
import com.ibm.ism.tools.msg.tms.ISMTMSManager;

public class ISMLogFileParser {

	private TreeSet<ISMLogEntryObject> logEntriesSet = new TreeSet<ISMLogEntryObject>();

	Logger logger = LogManager.getLogger();

	private static ISMLogEntryParser ismLogEntryParser = new ISMLogEntryParserImpl();

	private static ISMTMSManager tmsManager = new ISMTMSManager();

	public static HashMap<String, ISMErrorCodeObject> ErrorCodesMaps = new HashMap<String, ISMErrorCodeObject>();
	public static HashMap<String, ISMLogEntryObject> HelpLogEntryMap = new HashMap<String, ISMLogEntryObject>();

	public void parseComponentsFiles(LOG_ENTRY_COMPONENT_TYPE ctype) {

		switch (ctype.ordinal()) {
		case 0:
			for (int count = 0; count < Constants.ISM_TRANSPORT_COMPONENTS_SRC.length; count++) {
				parseDirectory(Constants.ISM_TRANSPORT_COMPONENTS_SRC[count],
						LOG_ENTRY_COMPONENT_TYPE.TRANSPORT,
						LOG_ENTRY_COMPONENT_TYPE.TRANSPORT);
			}

			break;
		case 1:
			for (int count = 0; count < Constants.ISM_UTILS_COMPONENTS_SRC.length; count++) {
				parseDirectory(Constants.ISM_UTILS_COMPONENTS_SRC[count],
						LOG_ENTRY_COMPONENT_TYPE.UTILS,
						LOG_ENTRY_COMPONENT_TYPE.UTILS);
			}

			break;
		case 2:
			for (int count = 0; count < Constants.ISM_PROTOCOL_COMPONENTS_SRC.length; count++) {
				parseDirectory(Constants.ISM_PROTOCOL_COMPONENTS_SRC[count],
						LOG_ENTRY_COMPONENT_TYPE.PROTOCOL,
						LOG_ENTRY_COMPONENT_TYPE.PROTOCOL);
			}

			break;
		case 3:
			for (int count = 0; count < Constants.ISM_ENGINE_COMPONENTS_SRC.length; count++) {
				parseDirectory(Constants.ISM_ENGINE_COMPONENTS_SRC[count],
						LOG_ENTRY_COMPONENT_TYPE.ENGINE,
						LOG_ENTRY_COMPONENT_TYPE.ENGINE);
			}

			break;
		case 4:
			for (int count = 0; count < Constants.ISM_STORE_COMPONENTS_SRC.length; count++) {
				parseDirectory(Constants.ISM_STORE_COMPONENTS_SRC[count],
						LOG_ENTRY_COMPONENT_TYPE.STORE,
						LOG_ENTRY_COMPONENT_TYPE.STORE);
			}

			break;
		case 5:
			for (int count = 0; count < Constants.ISM_ADMIN_COMPONENTS_SRC.length; count++) {
				parseDirectory(Constants.ISM_ADMIN_COMPONENTS_SRC[count],
						LOG_ENTRY_COMPONENT_TYPE.ADMIN,
						LOG_ENTRY_COMPONENT_TYPE.ADMIN);
			}

			break;
		case 7:
			for (int count = 0; count < Constants.ISM_MONITORING_COMPONENTS_SRC.length; count++) {
				parseDirectory(Constants.ISM_MONITORING_COMPONENTS_SRC[count],
						LOG_ENTRY_COMPONENT_TYPE.MONITORING,
						LOG_ENTRY_COMPONENT_TYPE.MONITORING);
			}

			break;
		case 8:
			for (int count = 0; count < Constants.ISM_MQCONNECTIVITY_COMPONENTS_SRC.length; count++) {
				parseDirectory(Constants.ISM_MQCONNECTIVITY_COMPONENTS_SRC[count],
						LOG_ENTRY_COMPONENT_TYPE.MQCONNECTIVITY,
						LOG_ENTRY_COMPONENT_TYPE.MQCONNECTIVITY);
			}

			break;
		case 9:
			for (int count = 0; count < Constants.ISM_PROXY_COMPONENTS_SRC.length; count++) {
				parseDirectory(Constants.ISM_PROXY_COMPONENTS_SRC[count],
						LOG_ENTRY_COMPONENT_TYPE.PROXY,
						LOG_ENTRY_COMPONENT_TYPE.PROXY);
			}

			break;
		}

	}

	public void parseDirectory(String directoryName,
			LOG_ENTRY_COMPONENT_TYPE type, LOG_ENTRY_COMPONENT_TYPE subtype) {
		String componentFullPathName = Constants.SRC_ROOT + File.separator
				+ directoryName;

		logger.log(Level.INFO, "Process component directory: "
				+ componentFullPathName);

		try {
			File componentFile = new File(componentFullPathName);
			parseFile(componentFile, type, subtype);

		} catch (Exception exception) {
			String msg = "Failed to parse file: " + componentFullPathName;
			msg += "\n" + exception.toString();
			logger.log(Level.SEVERE, msg);
			exception.printStackTrace();
		}

		// Save TMS
		tmsManager.saveTMSFile(subtype);

	}

	public void parseFile(File afile, LOG_ENTRY_COMPONENT_TYPE type,
			LOG_ENTRY_COMPONENT_TYPE subtype) {
		if (afile != null) {
			if (afile.isDirectory()) {
				File[] fileList = afile.listFiles();
				for (File afileo : fileList) {
					parseFile(afileo, type, subtype);
				}
			} else if (afile.isFile()) {
				if (afile.getName().endsWith(".c")
						|| afile.getName().endsWith(".cpp")

						|| afile.getName().endsWith(".java")
						|| afile.getName().endsWith(".h"))
					// Check to see if the file is in the skip list
					if (!ISMFilterManager.getSkippedFilesList().contains(
							afile.getName())) {

						processISMFile(afile.getAbsolutePath(), type, subtype);
					}

			}
		} else {
			System.out.println("The File Is NULL.");
			return;
		}

	}

	public void processISMFile(String filepath, LOG_ENTRY_COMPONENT_TYPE type,
			LOG_ENTRY_COMPONENT_TYPE subtype) {
		File testFile = new File(filepath);

		try {

			BufferedReader in = new BufferedReader(new FileReader(testFile));

			String inline = "";

			StringBuffer sb = new StringBuffer();
			boolean entireLine = false;
			boolean isLogLine = false;
			ISMLogEntryObject logEntry = null;
			int lineNum = 0;
			boolean ignoredup = false;
			while ((inline = in.readLine()) != null) {
				lineNum++;

				inline = inline.trim();

				if (inline.indexOf(Constants.IGNORE_DUP) >= 0) {
					ignoredup = true;
				}
				// Skip the comments
				if (inline.startsWith("#"))
					continue;

				if (ISMLogEntryParserUtil.isLogLine(inline)) {
					isLogLine = true;
				} else {
					if (sb.length() > 0) {
						isLogLine = true;
					} else {
						isLogLine = false;
					}
				}

				if (!isLogLine)
					continue;
				// Remove the continuation char
				if (inline.endsWith("\\")) {
					inline = inline.substring(0, inline.lastIndexOf("\\"));
					inline = inline.trim();
				}
				if (sb.length() > 0 && sb.toString().endsWith("\"")
						&& inline.startsWith("\"")) {
					inline = inline.substring(1);
					sb.replace(0, sb.toString().length(), sb.toString()
							.substring(0, sb.toString().length() - 1));
				}

				sb.append(inline);

				if (inline.endsWith(";") ||
					inline.startsWith("RC") && inline.indexOf("ISMRC_")>0) {
					entireLine = true;
				}

				// After parsing, reinit sb
				if (entireLine) {
					// Process the line
					String eLine = sb.toString().trim();

					if (!eLine.startsWith("LOGCAT")&& !eLine.startsWith("LOGGER")&&  (eLine.indexOf("LOG") >= 0
							|| eLine.indexOf("DISPLAY_MSG") >= 0 || eLine.indexOf("GET_MSG") >= 0
							|| eLine.startsWith("ADD_HELPMSG") ) ) {
						logEntry = new ISMLogEntryObject(filepath, lineNum,
								type, subtype, LOG_POINT_TYPE.SIMPLE);

						// logEntry = parseSimpleTraceInvoke(eLine, logEntry);

						logEntry = ismLogEntryParser.parseLLMLogLine(eLine,
								logEntry);
						logEntry.setIgnoreDup(ignoredup);
						addAndValidateEntry(logEntry);
						ignoredup = false;
					}else if(eLine.startsWith("RC") && eLine.indexOf("ISMRC_")>0  ){
						logEntry = new ISMLogEntryObject(filepath, lineNum,
								type, subtype, LOG_POINT_TYPE.ERROR_CODE);
						//Default category for RC is error.
						logEntry.setCategory("Error");
						logEntry = ismLogEntryParser.parseLLMLogLine(eLine,
								logEntry);
						logEntry.setIgnoreDup(ignoredup);
						addAndValidateEntry(logEntry);
						//ignoredup = false;

					}
					sb.setLength(0);
					entireLine = false;
					isLogLine = false;

				}

			}
			// System.out.println("Ended Reading for file: " + filepath);
			logger.log(Level.INFO, "End reading for file: " + filepath);

			in.close();
		} catch (Exception e) {
			e.printStackTrace();
		}

	}

	public void addAndValidateEntry(ISMLogEntryObject logEntry) {

		/* Need to make sure that the type is matched with the id */

		if (!logEntriesSet.contains(logEntry)) {
			if (logEntry.isValid())
				logEntriesSet.add(logEntry);
			// VALIDATE AGAINST TMS.
			//tmsManager.processLogEntry(logEntry);
		} else {

			// If ignore duplicate is on
			if (logEntry.isIgnoreDup() && !logEntry.isValid()) {
				return;
			}
			// if log message ID is in the skip list. Ignore it.
			if (ISMFilterManager.getSkippedMsgIDList().contains(
					logEntry.getMsgKey())) {
				return;
			}

			// The Object existed due the to key. Validate the message
			for (ISMLogEntryObject sLEntry : logEntriesSet) {
				if (sLEntry.equals(logEntry)) {

					if (sLEntry.isIgnoreDup()) {
						continue;
					}

					// Validate the message if the type is simple
					if (logEntry.getLogPointType() == LOG_POINT_TYPE.SIMPLE) {
						if (!sLEntry.validateMsg(logEntry.getAllMsgText())) {
							StringBuffer logMsg = new StringBuffer();
							logMsg.append("Duplicate Entry Found: "
									+ logEntry.getLogPointType() + "\n");
							logMsg.append("sLEntry: " + sLEntry.toString()
									+ "\n");
							logMsg.append("logEntry: " + logEntry.toString()
									+ "\n");
							logger.log(Level.WARNING, logMsg.toString());
							System.out.println(logMsg.toString());
						}
					} else {

						StringBuffer logMsg = new StringBuffer();
						logMsg.append("Duplicate Entry Found: "
								+ logEntry.getLogPointType() + "\n");
						logMsg.append("sLEntry: " + sLEntry.toString() + "\n");
						logMsg
								.append("logEntry: " + logEntry.toString()
										+ "\n");
						logger.log(Level.WARNING, logMsg.toString());
						System.out.println(logMsg.toString());
					}

				}
			}
		}
	}

	public Collection<ISMLogEntryObject> getLogEntries() {

		return logEntriesSet;

	}

	public List<ISMLogEntryObject> getComponentLogEntryList(
			LOG_ENTRY_COMPONENT_TYPE ctype) {
		List<ISMLogEntryObject> logEntriesList = new ArrayList<ISMLogEntryObject>();
		for (ISMLogEntryObject logEntry : logEntriesSet) {
			if (logEntry.getSubComponentType() == ctype) {
				// System.out.println("Component: "+logEntry.getComponentType().name()+". MsgSuffix: "+
				// logEntry.getMessageSuffix());
				logEntriesList.add(logEntry);
			}
		}
		return logEntriesList;
	}

	public List<String> getAvailMsgKeyList(LOG_ENTRY_COMPONENT_TYPE ctype) {
		List<String> retList = new ArrayList<String>();

		retList = getAvailMsgKeyList(getComponentLogEntryList(ctype), ctype);

		return retList;

	}

	public List<String> getAvailMsgKeyList(List<ISMLogEntryObject> entries,
			LOG_ENTRY_COMPONENT_TYPE ctype) {
		List<ISMLogEntryObject> rangeObjects = new ArrayList<ISMLogEntryObject>();
		List<String> availMsgKeyList = new ArrayList<String>();
		int cmin = 0, cmax = 0;
		String msgPrefix = "";
		String formatStr = "";

		switch (ctype.ordinal()) {
		case 0:
			cmin = Constants.TRANSPORT_MSG_KEY_MIN;
			cmax = Constants.TRANSPORT_MSG_KEY_MAX;
			break;
		case 1:
			cmin = Constants.UTILS_MSG_KEY_MIN;
			cmax = Constants.UTILS_MSG_KEY_MAX;
			break;
		case 2:
			cmin = Constants.PROTOCOL_MSG_KEY_MIN;
			cmax = Constants.PROTOCOL_MSG_KEY_MAX;
			break;
		case 3:
			cmin = Constants.ENGINE_MSG_KEY_MIN;
			cmax = Constants.ENGINE_MSG_KEY_MAX;
			break;
		case 4:
			cmin = Constants.STORE_MSG_KEY_MIN;
			cmax = Constants.STORE_MSG_KEY_MAX;
			break;
		case 5:
			cmin = Constants.ADMIN_MSG_KEY_MIN;
			cmax = Constants.ADMIN_MSG_KEY_MAX;
			break;
		case 6:
			cmin = Constants.GUI_MSG_KEY_MIN;
			cmax = Constants.GUI_MSG_KEY_MAX;
			break;
		case 7:
			cmin = Constants.MONITORING_MSG_KEY_MIN;
			cmax = Constants.MONITORING_MSG_KEY_MAX;
			break;
		case 8:
			cmin = Constants.MQCONNECTIVITY_MSG_KEY_MIN;
			cmax = Constants.MQCONNECTIVITY_MSG_KEY_MAX;
			break;
		default:
			cmin = Constants.UTILS_MSG_KEY_MIN;
			cmax = Constants.UTILS_MSG_KEY_MAX;
			break;
		}

		formatStr = "%04d";
//		if (ctype == LOG_ENTRY_COMPONENT_TYPE.TRANSPORT) {
//			cmin = Constants.TRANSPORT_MSG_KEY_MIN;
//			cmax = Constants.TRANSPORT_MSG_KEY_MAX;
//			formatStr = "%04d";
//
//		}
		for (int count = cmin; count <= cmax; count++) {
			String msgKey = msgPrefix + String.format(formatStr, count);
			rangeObjects.add(new ISMLogEntryObject(msgKey, ctype, ctype,
					LOG_POINT_TYPE.SIMPLE));

		}
		rangeObjects.removeAll(entries);

		for (ISMLogEntryObject availentry : rangeObjects) {
			availMsgKeyList.add(""
					+ String.format(formatStr, availentry.getMessageSuffix()));
		}
		// Get the Reserved List
		List<String> compReservedIDList = ISMFilterManager
				.getComponentReservedIDs(ctype);
		if (compReservedIDList != null) {
			// Remove all IDs which are in the reserved List.
			availMsgKeyList.removeAll(compReservedIDList);

		}
		return availMsgKeyList;

	}

	public List<String> getAvailableMsgKey(int num,
			LOG_ENTRY_COMPONENT_TYPE ctype) {
		List<String> tempavaillist = new ArrayList<String>();

		List<String> availMsgKeyList = getAvailMsgKeyList(ctype);
		if (availMsgKeyList.size() == 0)
			return null;

		for (int count = 0; count < num; count++) {
			tempavaillist.add(availMsgKeyList.remove(0));

			if (availMsgKeyList.size() == 0)
				break;

		}

		return tempavaillist;
	}

	public String getComponentMsgNumberRange(LOG_ENTRY_COMPONENT_TYPE ctype) {
		int cmin = 0;
		int cmax = 0;
		switch (ctype.ordinal()) {
		case 0:
			cmin = Constants.TRANSPORT_MSG_KEY_MIN;
			cmax = Constants.TRANSPORT_MSG_KEY_MAX;
			break;
		case 1:
			cmin = Constants.UTILS_MSG_KEY_MIN;
			cmax = Constants.UTILS_MSG_KEY_MAX;
			break;
		case 2:
			cmin = Constants.PROTOCOL_MSG_KEY_MIN;
			cmax = Constants.PROTOCOL_MSG_KEY_MAX;
			break;
		case 3:
			cmin = Constants.ENGINE_MSG_KEY_MIN;
			cmax = Constants.ENGINE_MSG_KEY_MAX;
			break;
		case 4:
			cmin = Constants.STORE_MSG_KEY_MIN;
			cmax = Constants.STORE_MSG_KEY_MAX;
			break;
		case 5:
			cmin = Constants.ADMIN_MSG_KEY_MIN;
			cmax = Constants.ADMIN_MSG_KEY_MAX;
			break;
		case 6:
			cmin = Constants.GUI_MSG_KEY_MIN;
			cmax = Constants.GUI_MSG_KEY_MAX;
			break;
		case 7:
			cmin = Constants.MONITORING_MSG_KEY_MIN;
			cmax = Constants.MONITORING_MSG_KEY_MAX;
			break;
		case 8:
			cmin = Constants.MQCONNECTIVITY_MSG_KEY_MIN;
			cmax = Constants.MQCONNECTIVITY_MSG_KEY_MAX;
			break;
		default:
			cmin = Constants.UTILS_MSG_KEY_MIN;
			cmax = Constants.UTILS_MSG_KEY_MAX;
			break;
		}
		return String.format("%04d", cmin) + "-" + cmax;
	}

	public void validateTMS(LOG_ENTRY_COMPONENT_TYPE type) {

		for (ISMLogEntryObject logEntry : logEntriesSet) {
			if (logEntry.getComponentType() == type)
				tmsManager.processLogEntry(logEntry);

		}
		tmsManager.saveTMSFile(type);

	}

	public void validateAllTMS() {

		for (ISMLogEntryObject logEntry : logEntriesSet) {
			tmsManager.processLogEntry(logEntry);

		}
		tmsManager.saveAllTMSFiles();

		/* TODO: Checking for Orphan from TMS Files. */
		tmsManager.TMSOrphanEntryChecking(logEntriesSet);

	}

	public void combinTMSFiles() {
		tmsManager.combineTMSFiles();
	}

	public int getErrorCodeNumber(String errorCodeStr){
		int retValue = -1;
		if(errorCodeStr!=null){
			for (ISMLogEntryObject logEntry : logEntriesSet) {
				if (logEntry.getLogPointType() == LOG_POINT_TYPE.ERROR_CODE){
					if(errorCodeStr.equalsIgnoreCase(logEntry.getErrorCodeName())){
						retValue=logEntry.getErrorCode();
					}

				}
			}
		}
		return retValue;

	}

}
