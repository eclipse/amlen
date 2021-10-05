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

package com.ibm.ima.test.cli.testplan.xml;

public class MQTTTransmitter implements IPolicyValidator {


	private final static String actionString = "rmdt";
	private final static String type = "CompositeAction";
	private final static String driverName = "wsDriver";
	private final static String rootElementName = "IsmWSTest";

	private String clientId = null;
	private String user = null;
	private String password = null;
	private String topic = null;
	private String port = null;
	private Action rootAction = null;
	private boolean built = false;


	public MQTTTransmitter(String topic, int port) {

		this.rootAction = new Action(actionString, type);
		this.topic = topic;
		this.port = Integer.toString(port);

	}

	public void addConnection() {

		// Connection Factory...
		Action connection = new Action("CreateConnection", "CreateConnection");
		rootAction.addAction(connection);

		ActionChild connectionChild = ActionChild.createActionParam("structure_id", "Con1");
		connection.addActionChild(connectionChild);

		connectionChild = ActionChild.createActionParam("connection_id", "CF1");
		connection.addActionChild(connectionChild);

		connectionChild = new ActionChild("include", "../common/ConnectionType.xml");
		connection.addActionChild(connectionChild);

		connectionChild = new ActionChild("include", "../common/MQTT_server.xml");
		connection.addActionChild(connectionChild);

		if (clientId != null) {
			
			connectionChild = new ActionChild("ApiParameter", clientId);
			connectionChild.addAttr("name", "clientId");
			connection.addActionChild(connectionChild);

		}
		
		connectionChild = new ActionChild("ApiParameter", port);
		connectionChild.addAttr("name", "port");
		connection.addActionChild(connectionChild);


		connectionChild = new ActionChild("ApiParameter", "mqtt");
		connectionChild.addAttr("name", "protocol");
		connection.addActionChild(connectionChild);


		connectionChild = new ActionChild("ApiParameter", "true");
		connectionChild.addAttr("name", "cleanSession");
		connection.addActionChild(connectionChild);

		if (user != null) {
			
			connectionChild = new ActionChild("ApiParameter", user);
			connectionChild.addAttr("name", "user");
			connection.addActionChild(connectionChild);

			connectionChild = new ActionChild("ApiParameter", password);
			connectionChild.addAttr("name", "password");
			connection.addActionChild(connectionChild);

		}	

	}

	public void addCreateMessage() {

		Action message = new Action("CreateMessage", "CreateMessage");
		rootAction.addAction(message);

		ActionChild messageChild = ActionChild.createActionParam("structure_id", "txmsg1");
		message.addActionChild(messageChild);

		messageChild = ActionChild.createActionParam("connection_id", "CF1");
		message.addActionChild(messageChild);


		messageChild = new ActionChild("ApiParameter", "TEXT");
		messageChild.addAttr("name", "msgType");
		message.addActionChild(messageChild);

		messageChild = new ActionChild("ApiParameter", "TEXT: test content 1");
		messageChild.addAttr("name", "payload");
		message.addActionChild(messageChild);

	}

	public void addSyncComponentsSender() {

		Action sender = new Action("sync_components_sender", "SyncComponentAction");
		rootAction.addAction(sender);

		ActionChild senderChild = ActionChild.createActionParam("component_name", "sender");
		sender.addActionChild(senderChild);

		senderChild = ActionChild.createActionParam("component_list", "sender;receiver");
		sender.addActionChild(senderChild);

		senderChild = ActionChild.createActionParam("timeout", "30000");
		sender.addActionChild(senderChild);

	}


	public void addSendMessage() {

		Action message = new Action("SendMessage_a", "SendMessage");
		rootAction.addAction(message);

		ActionChild messageChild = ActionChild.createActionParam("connection_id", "CF1");
		message.addActionChild(messageChild);

		messageChild = ActionChild.createActionParam("message_id", "txmsg1");
		message.addActionChild(messageChild);

		messageChild = ActionChild.createActionParam("topic", topic);
		message.addActionChild(messageChild);

		messageChild = ActionChild.createActionParam("QoS", "0");
		message.addActionChild(messageChild);

	}

	public void addCloseConnection()  {

		Action closeConnection = new Action("CloseConnection", "CloseConnection");
		rootAction.addAction(closeConnection);

		ActionChild closeConnectionChild = ActionChild.createActionParam("connection_id", "CF1");
		closeConnection.addActionChild(closeConnectionChild);

	}


	public void buildReceiver() {

		addConnection();
		addCreateMessage();
		addSyncComponentsSender();
		addSendMessage();
		addCloseConnection();

	}


	public String getClientId() {
		return clientId;
	}

	public void setClientId(String clientId) {
		this.clientId = clientId;
	}

	public String getUser() {
		return user;
	}

	public void setUser(String user) {
		this.user = user;
	}

	public String getPassword() {
		return password;
	}

	public void setPassword(String password) {
		this.password = password;
	}

	public String getTopic() {
		return topic;
	}

	public void setTopic(String topic) {
		this.topic = topic;
	}

	public Action getRootAction() {
		if (built == false) {
			buildReceiver();
			built = true;
		}
		return rootAction;
	}


	public String getXmlName() {
		return "mqtt_rmdt_policy_validation_" + port + ".xml";
	}


	public String getDriverName() {
		return driverName;
	}

	public String getRootElementName() {
		return rootElementName;
	}


}
