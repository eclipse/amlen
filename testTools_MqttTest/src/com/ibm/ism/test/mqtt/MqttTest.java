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
package com.ibm.ism.test.mqtt;

import java.io.IOException;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Attr;
import org.w3c.dom.DOMException;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import com.ibm.ism.mqtt.IsmMqttConnection;

public class MqttTest {

	
	private static String _ismAddress="10.10.10.10";
	private static int _ismPort=16104;
	private static String _wsProt="/mqtt";
	private static String _clientId="client1";
	private static IsmMqttConnection _connection;
	
	private static MqttTestListener _defaultListener = new MqttTestListener();
	private static boolean _isAsynchronous = false;
	private static ReceiveThread _receive;
	private static boolean _isSynchronous = false;
	
	private static LinkedList<Action> _actions = new LinkedList<Action>();
	
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		
		DocumentBuilderFactory docbuildfact = DocumentBuilderFactory.newInstance();
		try {
			DocumentBuilder docbuilder = docbuildfact.newDocumentBuilder();		
			Document xmldoc = docbuilder.parse(args[0]);
			parseConfig(xmldoc);
			// Initialize the connection object
			_connection = new IsmMqttConnection(_ismAddress, _ismPort, _wsProt, _clientId);
			processActions();
			int numMessagesReceived = 0;
			if (_isAsynchronous) {
				numMessagesReceived += _defaultListener.getNumReceived();
			}
			if (_isSynchronous) {
				numMessagesReceived += _receive.getNumReceived();
			}
			System.out.println("Number of Messages Received: " + numMessagesReceived );
			
		} catch (SAXException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		} catch (IOException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		} catch (ParserConfigurationException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}		
	}

	private static void processActions() {
		// Process the list of actions in order
		while(!_actions.isEmpty()) {
			Action action = _actions.removeFirst();
			String action_type = action.getConfig().get("type");
			try {
				if (action_type == null) {
					System.out.println("Error: The type attribute for the action is missing");
				}
				else if (action_type.equals("setVerbose")) {					
					_connection.setVerbose(Boolean.parseBoolean(action.getParameters().get("verbose")));
				}
				else if (action_type.equals("setListener")) {
					_connection.setListener(_defaultListener);
					_isAsynchronous = true;
				}
				else if (action_type.equals("receive")) {
					// Synchronous receive
					int numMessages = Integer.parseInt(action.getConfig().get("totalmessages"));
					long timeout = 0;
					if (action.getParameters().get("timeout") != null) {
						timeout = Long.parseLong(action.getParameters().get("timeout"));
					}
					_isSynchronous = true;
					_receive = new ReceiveThread(_connection, timeout, numMessages);
					Thread newThread = new Thread(_receive);
					newThread.start();
				}
				else if (action_type.equals("connect")) {
					_connection.connect();
				}
				else if (action_type.equals("subscribe")) {
					int numTopics = getNumTopics(action);
					String topic_name = action.getParameters().get("topic");
					String prefix = getPrefix(action);
					System.out.println(action.toString());
					int qos = Integer.parseInt(action.getParameters().get("qos"));
					for (int i=0; i<numTopics; i++) {
						if (prefix != null) {
							topic_name=prefix+i;
						}
						_connection.subscribe(topic_name, qos);
					}
				}
				else if (action_type.equals("publish")) {
					int numTopics = getNumTopics(action);
					String topic_name = action.getParameters().get("topic");
					String prefix = getPrefix(action);
					int numMessages = Integer.parseInt(action.getConfig().get("totalmessages"));
					String payload = "default_payload";
					
					if (action.getParameters().get("payload") != null) {
						payload = action.getParameters().get("payload");
					}
					
					for (int i=0; i<numMessages; i++) {
						if (prefix != null) {
							int topicNum=i%numTopics;
							topic_name=prefix+topicNum;
						}
						_connection.publish(topic_name, payload+i);
					}	
				}
				else if (action_type.equals("sleep")) {
					Thread.sleep(Integer.parseInt(action.getParameters().get("ms_to_sleep")));	
				}
				else if (action_type.equals("unsubscribe")) {
					int numTopics = getNumTopics(action);
					String topic_name = action.getParameters().get("topic");
					String prefix = getPrefix(action);
					for (int i=0; i<numTopics; i++) {
						if (prefix != null) {
							topic_name=prefix+i;
						}
						_connection.unsubscribe(topic_name);
					}
				}
				else if (action_type.equals("disconnect")) {
					_connection.disconnect();
				}
				else {
					System.out.println("Error: Unknown action type: \"" + action_type + "\"");
				}
			} catch (IOException e) {
				e.printStackTrace();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
		
	}

	private static int getNumTopics(Action action) {
		int numTopics = 1;
		String topic_name = action.getParameters().get("topic");
		
		// If the topic name is not provided, then there may be multiple topics
		// and the topic names will be generated.
		if (topic_name == null || topic_name.equals("")) {
			if (action.getConfig().get("numtopics") != null) {
				numTopics=Integer.parseInt(action.getConfig().get("numtopics"));
			}
		}
		return numTopics;
	}
	
	private static String getPrefix(Action action) {
		String prefix = null;
		String topic_name = action.getParameters().get("topic");
		
		// If the topic name is not provided, then get the prefix
		// to use to generate the topic names
		if (topic_name == null || topic_name.equals("")) {
			prefix = action.getConfig().get("prefix");
			if (prefix == null) {
				System.out.println("Error: The prefix attribute must be specified when the topic name is not provided.");
			}
		}
		return prefix;
	}

	private static void parseConfig(Document xmldoc) {
		
		Element root = xmldoc.getDocumentElement();
		if (root.getNodeName() == "mqttclient") {
			// Go through MQTT Client Config
			NamedNodeMap clientConfig = root.getAttributes();
			
			// First, process the include if it exists to update/override the attributes
			if (clientConfig.getNamedItem("include") != null ){
				parseInclude(xmldoc, clientConfig);
			}
			if (clientConfig.getNamedItem("ismaddress") != null ){
				_ismAddress=clientConfig.getNamedItem("ismaddress").getNodeValue();
			}
			if (clientConfig.getNamedItem("ismport") != null ){
				_ismPort=Integer.parseInt(clientConfig.getNamedItem("ismport").getNodeValue());
			}
			if (clientConfig.getNamedItem("clientid") != null ){
				_clientId=clientConfig.getNamedItem("clientid").getNodeValue();
			}
			if (clientConfig.getNamedItem("wsprotocol") != null ){
				_wsProt=clientConfig.getNamedItem("wsprotocol").getNodeValue();
			}
						
			// Build a list of actions
			NodeList children = root.getChildNodes();
			for (int i=0; i<children.getLength(); i++){ 
				if (children.item(i).getNodeName() == "action") {
					// Get any attributes
					NamedNodeMap actionConfigAttr = children.item(i).getAttributes();
					if (actionConfigAttr.getNamedItem("include") != null) {
						parseInclude(xmldoc, actionConfigAttr);
					}
					Map<String,String> actionConfig = new HashMap<String,String>();
					for (int j=0; j<actionConfigAttr.getLength(); j++) {
						actionConfig.put(actionConfigAttr.item(j).getNodeName(),actionConfigAttr.item(j).getNodeValue());
					}
					// Get any children
					NodeList actionChildren = children.item(i).getChildNodes();
					Map<String,String> actionParams = new HashMap<String,String>();
					for (int j=0; j<actionChildren.getLength(); j++) {
						if (actionChildren.item(j).getNodeName() == "param") {
							String name = actionChildren.item(j).getAttributes().getNamedItem("name").getNodeValue();
							if (name == null) { 
								System.out.println("Error: name attribute must be specified for param element");
							}
							String value = actionChildren.item(j).getTextContent();
							actionParams.put(name, value);
						}
					}
					_actions.add(new Action(actionConfig,actionParams));
				}
			}
		}
		else {
			System.out.println("Error: Invalid root element");
		}
		
	}

	private static void parseInclude(Document xmldoc, NamedNodeMap attributes) {
		// Parse the include file
		DocumentBuilderFactory docbuildfact = DocumentBuilderFactory.newInstance();
		DocumentBuilder docbuilder;
		try {
			docbuilder = docbuildfact.newDocumentBuilder();		
			Document include = docbuilder.parse(attributes.getNamedItem("include").getNodeValue());
			Element includeRoot = include.getDocumentElement();
			NamedNodeMap includeAttrs = includeRoot.getAttributes();
			for (int i=0; i<includeAttrs.getLength(); i++) {
				Attr newAttr = xmldoc.createAttribute(includeAttrs.item(i).getNodeName());
				newAttr.setNodeValue(includeAttrs.item(i).getNodeValue());
				attributes.setNamedItem(newAttr);
			}
		} catch (ParserConfigurationException e) {
			e.printStackTrace();
		} catch (DOMException e) {
			e.printStackTrace();
		} catch (SAXException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}	
	}

}
