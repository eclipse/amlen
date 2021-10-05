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
package com.ibm.ism.tools.msg.tms;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Set;
import java.util.TreeSet;
import java.util.Map.Entry;
import java.util.logging.Level;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerConfigurationException;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import com.ibm.ism.tools.msg.ISMFilterManager;
import com.ibm.ism.tools.msg.ISMLogEntryObject;
import com.ibm.ism.tools.msg.ISMLogEntryObject.LOG_ENTRY_COMPONENT_TYPE;
import com.ibm.ism.tools.msg.impl.ISMLogEntryParserUtil;
import com.ibm.ism.tools.msg.logger.LogManager;

public class ISMTMSManager {

	private static final String ISM_TMS_TRANSPORT_PATH = "ism_transport_tms.xml";
	private static final String ISM_TMS_SERVERUTILS_PATH = "ism_utils_tms.xml";
	private static final String ISM_TMS_PROTOCOL_PATH = "ism_protocol_tms.xml";
	private static final String ISM_TMS_ENGINE_PATH = "ism_engine_tms.xml";
	private static final String ISM_TMS_STORE_PATH = "ism_store_tms.xml";
	private static final String ISM_TMS_ADMIN_PATH = "ism_admin_tms.xml";
	private static final String ISM_TMS_GUI_PATH = "ism_webui_tms.xml";
	private static final String ISM_TMS_MONITORING_PATH = "ism_monitoring_tms.xml";
	private static final String ISM_TMS_MQCONNECTIVITY_PATH = "ism_mqcbridge_tms.xml";
	private static final String ISM_TMS_PROXY_PATH = "ism_proxy_tms.xml";

	private static final String ISM_TMS_PATH = "ism.xml";

	private Hashtable<String, TMSCompDoc> documentCompMap = new Hashtable<String, TMSCompDoc>();

	private static boolean isAutoEntryAdd = false;

	private static Document masterdoc;

	public ISMTMSManager() {

		Document doc = parseFile(ISM_TMS_TRANSPORT_PATH);
		TMSCompDoc compDoc = new TMSCompDoc(LOG_ENTRY_COMPONENT_TYPE.TRANSPORT,
				ISM_TMS_TRANSPORT_PATH, doc);
		documentCompMap.put(LOG_ENTRY_COMPONENT_TYPE.TRANSPORT.name(), compDoc);

		doc = parseFile(ISM_TMS_SERVERUTILS_PATH);
		compDoc = new TMSCompDoc(LOG_ENTRY_COMPONENT_TYPE.UTILS,
				ISM_TMS_SERVERUTILS_PATH, doc);
		documentCompMap.put(LOG_ENTRY_COMPONENT_TYPE.UTILS.name(), compDoc);

		doc = parseFile(ISM_TMS_PROTOCOL_PATH);
		compDoc = new TMSCompDoc(LOG_ENTRY_COMPONENT_TYPE.PROTOCOL,
				ISM_TMS_PROTOCOL_PATH, doc);
		documentCompMap.put(LOG_ENTRY_COMPONENT_TYPE.PROTOCOL.name(), compDoc);

		doc = parseFile(ISM_TMS_ENGINE_PATH);
		compDoc = new TMSCompDoc(LOG_ENTRY_COMPONENT_TYPE.ENGINE,
				ISM_TMS_ENGINE_PATH, doc);
		documentCompMap.put(LOG_ENTRY_COMPONENT_TYPE.ENGINE.name(), compDoc);

		doc = parseFile(ISM_TMS_ADMIN_PATH);
		compDoc = new TMSCompDoc(LOG_ENTRY_COMPONENT_TYPE.ADMIN,
				ISM_TMS_ADMIN_PATH, doc);
		documentCompMap.put(LOG_ENTRY_COMPONENT_TYPE.ADMIN.name(), compDoc);


		doc = parseFile(ISM_TMS_STORE_PATH);
		compDoc = new TMSCompDoc(LOG_ENTRY_COMPONENT_TYPE.STORE,
				ISM_TMS_STORE_PATH, doc);
		documentCompMap.put(LOG_ENTRY_COMPONENT_TYPE.STORE.name(), compDoc);

		doc = parseFile(ISM_TMS_MONITORING_PATH);
		compDoc = new TMSCompDoc(LOG_ENTRY_COMPONENT_TYPE.MONITORING,
				ISM_TMS_MONITORING_PATH, doc);
		documentCompMap.put(LOG_ENTRY_COMPONENT_TYPE.MONITORING.name(), compDoc);

		doc = parseFile(ISM_TMS_MQCONNECTIVITY_PATH);
		compDoc = new TMSCompDoc(LOG_ENTRY_COMPONENT_TYPE.MQCONNECTIVITY,
				ISM_TMS_MQCONNECTIVITY_PATH, doc);
		documentCompMap.put(LOG_ENTRY_COMPONENT_TYPE.MQCONNECTIVITY.name(), compDoc);

		doc = parseFile(ISM_TMS_PROXY_PATH);
		compDoc = new TMSCompDoc(LOG_ENTRY_COMPONENT_TYPE.PROXY,
				ISM_TMS_PROXY_PATH, doc);
		documentCompMap.put(LOG_ENTRY_COMPONENT_TYPE.PROXY.name(), compDoc);
	}

	public void combineTMSFiles() {
		masterdoc = parseFile(ISM_TMS_PATH);

		for (Entry<String, TMSCompDoc> mapEntry : documentCompMap.entrySet()) {
			TMSCompDoc compdoc = mapEntry.getValue();
			if (compdoc != null) {
				// saveXMLDocument(compdoc.getTmsfilepath(), compdoc.getDoc());
				Document doc = compdoc.getDoc();
				NodeList keyEntries = doc.getElementsByTagName("Message");

				for (int idx = 0; idx < keyEntries.getLength(); idx++) {
					/* add to master doc */
					Node item = keyEntries.item(idx);
					addMessageNode(masterdoc.importNode(item, true), masterdoc);
				}

			}
		}

		saveXMLDocument(ISM_TMS_PATH, masterdoc);
	}

	public static String getTMSCompFilePath(LOG_ENTRY_COMPONENT_TYPE logtype) {
		String retValue = null;

		switch(logtype){

			case TRANSPORT:
				retValue = ISM_TMS_TRANSPORT_PATH;
				break;
			case ENGINE:
				retValue = ISM_TMS_ENGINE_PATH;
				break;
			case STORE:
				retValue = ISM_TMS_STORE_PATH;
				break;
			case PROTOCOL:
				retValue = ISM_TMS_PROTOCOL_PATH;
				break;
			case ADMIN:
				retValue = ISM_TMS_ADMIN_PATH;
				break;
			case GUI:
				retValue = ISM_TMS_GUI_PATH;
				break;
			case MQCONNECTIVITY:
				retValue = ISM_TMS_MQCONNECTIVITY_PATH;
				break;
			case UTILS:
			default:
				retValue = ISM_TMS_SERVERUTILS_PATH;
				break;

		}

		return retValue;
	}

	/**
	 * Parses XML file and returns XML document.
	 *
	 * @param fileName
	 *            XML file to parse
	 * @return XML document or <B>null</B> if error occured
	 */
	public Document parseFile(String fileName) {

		LogManager.getLogger().log(Level.INFO, "Parsing XML file:" + fileName);

		DocumentBuilder docBuilder;
		Document doc = null;
		DocumentBuilderFactory docBuilderFactory = DocumentBuilderFactory
				.newInstance();
		docBuilderFactory.setIgnoringElementContentWhitespace(true);

		try {
			docBuilder = docBuilderFactory.newDocumentBuilder();
		} catch (ParserConfigurationException e) {
			LogManager.getLogger().log(Level.INFO,
					"Wrong parser configuration: " + e.getMessage());
			return null;
		}
		File sourceFile = new File(fileName);
		try {

			doc = docBuilder.parse(sourceFile);
		} catch (SAXException e) {
			LogManager.getLogger().log(Level.INFO,
					"Failed to parse the TMS XML File: " + e.getMessage());
			System.err.println("Failed to parse the TMS XML File: "
					+ e.getMessage());

			return null;
		} catch (IOException e) {
			LogManager.getLogger().log(Level.INFO,
					"Failed to parse the TMS XML File: " + e.getMessage());
			System.err.println("Failed to parse the TMS XML File: "
					+ e.getMessage());

		}

		return doc;
	}

	public void processLogEntry(ISMLogEntryObject logEntry) {
		boolean foundEntryinTMS = false;
		// Check if we need to skip this log Entry
		if (logEntry == null) {
			return;
		} else {
			if (ISMFilterManager.getSkippedMsgIDList().contains(
					logEntry.getMsgKey())) {
				return;
			}
		}

		Document doc = null;
		TMSCompDoc compDoc = null;
		// NodeList keyEntries = tmsfile.getElementsByTagName("Message");
		if (logEntry.getComponentType() != null) {
			if (logEntry.getComponentType().name().equals("SHARED")) {
				compDoc = documentCompMap.get(logEntry.getSubComponentType()
						.name());
			} else {
				compDoc = documentCompMap.get(logEntry.getComponentType()
						.name());
			}
			if (compDoc == null)
				return;
			doc = compDoc.getDoc();
			if (doc == null) {
				return;
			}
		} else {
			return;
		}

		NodeList keyEntries = doc.getElementsByTagName("Message");
		boolean isExplaination = false;
		boolean isOperatorReponse = false;

		for (int idx = 0; idx < keyEntries.getLength(); idx++) {
			Node item = keyEntries.item(idx);
			NamedNodeMap attrs = item.getAttributes();
			Node node = attrs.getNamedItem("ID");

			// TODO: Checking for duplicate: Same Prefix+MsgID but different
			// severity
			// 1. Create a temp ID with Severity
			String tmsMsgID = node.getTextContent();
			String tmpTMSMsgID = tmsMsgID.substring(0, tmsMsgID.length() - 1);
			String tmsMsgSeverity = tmsMsgID.substring(tmsMsgID.length() - 1);
			boolean noSeverity = false;

			try {
				Integer.parseInt(tmsMsgSeverity);
				tmpTMSMsgID = tmsMsgID;
				tmsMsgSeverity = "";
				noSeverity = true;
			} catch (NumberFormatException nfe) {
			}
			String logMsgID = logEntry.getTMSMessageKey(false);
			String logTMSMsgID = logMsgID.substring(0, logMsgID.length() - 1);

			if (logTMSMsgID.equals(tmpTMSMsgID)
					&& (noSeverity == false && !tmsMsgSeverity.equals(""
							+ logEntry.getSeverity()))) {
				// We have MsgID with different Severity. Display the warning.
				LogManager
						.getLogger()
						.log(
								Level.WARNING,
								"TMS Entry Message ID have different severity from the entry message ID from code: TMS Message ID="
										+ tmsMsgID
										+ " , Message ID from Code="
										+ logEntry.getTMSMessageKey(false));
				System.out
						.println("WARNING: TMS Entry Message ID have different severity from the entry message ID from code: TMS Message ID="
								+ tmsMsgID
								+ " , Message ID from Code="
								+ logEntry.getTMSMessageKey(false));
			} else if ((logEntry.getTMSMessageKey(false).equals(node
					.getTextContent()))
					|| (noSeverity == true && (logTMSMsgID.equals(tmpTMSMsgID)))) {
				foundEntryinTMS = true;
				NodeList children = item.getChildNodes();
				// TODO Replace Category
				// Set Category
				if (logEntry.getCategory() != null) {
					Node catnode = attrs.getNamedItem("category");
					if (catnode == null) {
						Attr categoryAttr = doc.createAttribute("category");
						categoryAttr.setValue(logEntry.getCategory());
						attrs.setNamedItem(categoryAttr);
					} else {
						catnode.setNodeValue(logEntry.getCategory());
					}

					compDoc.setSaved(false);

				}

				for (int childidx = 0; childidx < children.getLength(); childidx++) {
					Node child = children.item(childidx);
					if ("MsgText".equals(child.getNodeName())) {
						//child.setTextContent(logEntry.getTMSMessage());
						ISMLogEntryParserUtil.parseRawStringIntoTMSXMLNode(doc, child, logEntry.getTMSMessage());
						compDoc.setSaved(false);
						// needSave = true;
					} else if ("Explanation".equals(child.getNodeName())) {
						// Validate the Explaination
						if (child.getTextContent() == null
								|| child.getTextContent().equals("")) {
							isExplaination = false;
						} else {
							isExplaination = true;
						}

					} else if ("OperatorResponse".equals(child.getNodeName())) {
						if (child.getTextContent() == null
								|| child.getTextContent().equals("")) {
							isOperatorReponse = false;
						} else {
							isOperatorReponse = true;
						}
					}
				}

			}
		}

		if (!foundEntryinTMS && logEntry.isNeedInTMS()) {

			// System.out.println("Entry is not found in the TMS file: "+logEntry
			// .getTMSPGMKey());
			LogManager.getLogger().log(
					Level.WARNING,
					"Entry is not found in the TMS file: "
							+ logEntry.getMsgKey());
			System.out.println("ERROR: Entry is not found in the TMS file: "
					+ logEntry.getMsgKey());
			if (isAutoEntryAdd) {
				Node msgNode = createMessageNode(logEntry, doc);
				addMessageNode(msgNode, doc);
				compDoc.setSaved(false);
				LogManager.getLogger().log(
						Level.INFO,
						"The log entry with ID " + logEntry.getMsgKey()
								+ " is added to TMS file "
								+ compDoc.getTmsfilepath());
				System.out.println("INFO: The log entry with ID "
						+ logEntry.getMsgKey() + " is added to TMS file "
						+ compDoc.getTmsfilepath());
				foundEntryinTMS = true;
				isExplaination = false;
				isOperatorReponse = false;
			}

		}

		if (foundEntryinTMS && (!isExplaination || !isOperatorReponse)) {
			if (!isExplaination) {
				LogManager.getLogger().log(
						Level.SEVERE,
						"TMS Message <Explanation> is empty for log entry with ID "
								+ logEntry.getMsgKey());
				System.out
						.println("ERROR: TMS Message <Explanation> is empty for log entry with ID "
								+ logEntry.getMsgKey());
			}
			if (!isOperatorReponse) {
				LogManager.getLogger().log(
						Level.SEVERE,
						"TMS Message <OperatorResponse> is empty for log entry with ID  "
								+ logEntry.getMsgKey());
				System.out
						.println("ERROR: TMS Message <OperatorResponse> is empty for log entry with ID  "
								+ logEntry.getMsgKey());
			}

		}

		// if(needSave) saveXMLDocument(compDoc.getTmsfilepath(),
		// compDoc.getDoc());

	}

	public void saveTMSFile(LOG_ENTRY_COMPONENT_TYPE type) {
		if (type != null) {
			TMSCompDoc compdoc = documentCompMap.get(type.name());
			if (compdoc != null && !compdoc.isSaved()) {
				saveXMLDocument(compdoc.getTmsfilepath(), compdoc.getDoc());
			}
		}
	}

	public void saveAllTMSFiles() {
		for (Entry<String, TMSCompDoc> mapEntry : documentCompMap.entrySet()) {
			TMSCompDoc compdoc = mapEntry.getValue();
			if (compdoc != null && !compdoc.isSaved()) {
				saveXMLDocument(compdoc.getTmsfilepath(), compdoc.getDoc());
			}
		}
	}

	/**
	 * Saves XML Document into XML file.
	 *
	 * @param fileName
	 *            XML file name
	 * @param doc
	 *            XML document to save
	 * @return <B>true</B> if method success <B>false</B> otherwise
	 */
	public boolean saveXMLDocument(String fileName, Document doc) {

		LogManager.getLogger().log(Level.INFO,
				"Saving TMS XML file... " + fileName);
		System.out.println("Saving TMS XML file... " + fileName);
		// open output stream where XML Document will be saved
		File xmlOutputFile = new File(fileName);
		FileOutputStream fos;
		Transformer transformer;
		try {
			fos = new FileOutputStream(xmlOutputFile);
		} catch (FileNotFoundException e) {
			LogManager.getLogger().log(Level.SEVERE,
					"Failed to save TMS XML file: " + e.getLocalizedMessage());
			System.out.println("Error occured: " + e.getMessage());
			return false;
		}
		// Use a Transformer for output
		TransformerFactory transformerFactory = TransformerFactory
				.newInstance();
		try {
			transformer = transformerFactory.newTransformer();
			transformer.setOutputProperty(OutputKeys.INDENT, "yes");
			transformer.setOutputProperty(OutputKeys.METHOD, "xml");
			transformer.setOutputProperty(OutputKeys.ENCODING, "utf-8");

			if (doc.getDoctype() != null) {
				String systemValue = (new File(doc.getDoctype().getSystemId()))
						.getName();
				transformer.setOutputProperty(OutputKeys.DOCTYPE_SYSTEM,
						systemValue);
			}
		} catch (TransformerConfigurationException e) {
			LogManager.getLogger().log(Level.SEVERE,
					"Transformer configuration error: " + e.getMessage());
			System.out.println("Transformer configuration error: "
					+ e.getMessage());
			return false;
		}
		DOMSource source = new DOMSource(doc);
		StreamResult result = new StreamResult(fos);
		// transform source into result will do save
		try {
			transformer.transform(source, result);
		} catch (TransformerException e) {
			LogManager.getLogger().log(Level.SEVERE,
					"Transformer error: " + e.getMessage());
			System.out.println("Error transform: " + e.getMessage());
		}
		LogManager.getLogger().log(Level.INFO,
				"TMS XML file saved:  " + fileName);
		System.out.println("XML file saved.");

		return true;
	}

	public void addMessageNode(Node msgNode, Document doc) {
		NodeList nodeList = doc.getElementsByTagName("TMSSource");
		if (nodeList.getLength() > 0) {
			Node tmsSourceNode = nodeList.item(0);
			tmsSourceNode.appendChild(msgNode);
		}

	}

	public Node createMessageNode(ISMLogEntryObject logEntry, Document doc) {

		Node msgNode = doc.createElement("Message");
		Node msgTextNode = doc.createElement("MsgText");
		Node msgExplanationNode = doc.createElement("Explanation");
		Node msgOperatorResponseNode = doc.createElement("OperatorResponse");
		Node paragraphNode = doc.createElement("p");
		Node paragraphNode2 = doc.createElement("p");

		// Set Message Attributes
		NamedNodeMap msgAttributes = msgNode.getAttributes();
		Attr msgidattr = doc.createAttribute("ID");
		msgidattr.setValue(logEntry.getTMSMessageKey(false));
		msgAttributes.setNamedItem(msgidattr);

		Attr prefixattr = doc.createAttribute("prefix");
		prefixattr.setValue("no");
		msgAttributes.setNamedItem(prefixattr);

		// Set Category
		if (logEntry.getCategory() != null) {
			Attr categoryAttr = doc.createAttribute("category");
			categoryAttr.setValue(logEntry.getCategory());
			msgAttributes.setNamedItem(categoryAttr);
		}
		// Set MsgText Attributes
		NamedNodeMap msgTextAttributes = msgTextNode.getAttributes();
		Attr msgTextPgmKey = doc.createAttribute("pgmKey");
		msgTextPgmKey.setValue(logEntry.getTMSMessageKey(true));
		msgTextAttributes.setNamedItem(msgTextPgmKey);

		Attr msgTextVarFormat = doc.createAttribute("varFormat");
		// String fileName = logEntry.getFileName();
		/*
		 * if (fileName.endsWith("java")) { msgTextVarFormat.setValue("Java"); }
		 * else { msgTextVarFormat.setValue("C"); }
		 */
		msgTextVarFormat.setValue("ICU");

		msgTextAttributes.setNamedItem(msgTextVarFormat);

		//msgTextNode.setTextContent(logEntry.getTMSMessage());
		ISMLogEntryParserUtil.parseRawStringIntoTMSXMLNode(doc, msgTextNode, logEntry.getTMSMessage());


		msgNode.appendChild(msgTextNode);

		msgExplanationNode.appendChild(paragraphNode);
		msgNode.appendChild(msgExplanationNode);

		msgOperatorResponseNode.appendChild(paragraphNode2);
		msgNode.appendChild(msgOperatorResponseNode);

		return msgNode;

	}

	public static void setAutoAdd(boolean autoadd) {
		isAutoEntryAdd = autoadd;
	}

	public void TMSOrphanEntryChecking(TreeSet<ISMLogEntryObject> logEntriesSet) {
		// Collection<TMSCompDoc> tmsDocCollection = documentCompMap.values();
		Set<String> docKeys = documentCompMap.keySet();
		Iterator<String> tmsDocKeyIterator = docKeys.iterator();

		while (tmsDocKeyIterator.hasNext()) {
			String docCompStr = tmsDocKeyIterator.next();
			TMSCompDoc tmsDoc = documentCompMap.get(docCompStr);
			// Parse the ID and check with the logEntriesSet
			Document doc = tmsDoc.getDoc();
			NodeList keyEntries = doc.getElementsByTagName("Message");

			for (int idx = 0; idx < keyEntries.getLength(); idx++) {
				Node item = keyEntries.item(idx);
				NamedNodeMap attrs = item.getAttributes();
				Node node = attrs.getNamedItem("ID");

				String tmsMsgID = node.getTextContent();
				String tmpTMSMsgID = tmsMsgID.substring(0,
						tmsMsgID.length() - 1);
				String tmsMsgSeverity = tmsMsgID
						.substring(tmsMsgID.length() - 1);
				// boolean noSeverity=false;

				try {
					Integer.parseInt(tmsMsgSeverity);
					tmpTMSMsgID = tmsMsgID;
					tmsMsgSeverity = "";
					// noSeverity=true;
				} catch (NumberFormatException nfe) {
				}

				boolean entryFound = false;
				boolean compExist = false;
				for (ISMLogEntryObject logEntry : logEntriesSet) {
					if (logEntry.getSubComponentType() == ISMLogEntryObject
							.getComponentType(docCompStr)) {
						compExist = true;
						String logMsgID = logEntry.getTMSMessageKey(false);
						/* Determine if the logMsgID has Severity */
						String logTMSMsgID = null;

						logTMSMsgID = logMsgID.substring(0, logMsgID.length());
						// if(logEntry.getTMSMessageKey(false).equals(tmsMsgID)){
						if (logTMSMsgID.equals(tmpTMSMsgID)) {
							entryFound = true;
							break;

						}

					}
				}
				if (!entryFound && compExist) {
					System.out
							.println("WARNING: Orphan TMS Entry: " + tmsMsgID);
				}

			}

		}

	}

}
