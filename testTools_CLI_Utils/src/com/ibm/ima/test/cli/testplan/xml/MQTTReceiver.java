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

public class MQTTReceiver implements IPolicyValidator {
	
	
	private String type = "CompositeAction";
	private String clientId = null;
	private String user = null;
	private String password = null;
	private String topic = null;
	private String port = null;
	private Action rootAction = null;
	private boolean built = false;
	
	
	
	public MQTTReceiver(String topic, int port) {
		
		this.rootAction = new Action("rmdr", this.type);
		this.topic = topic;
		this.port = Integer.toString(port);
		
	}
	
	
	public void addSyncAction() {
		
		// Connection Factory...
		Action syncAction = new Action("syncReset", "SyncAction");
		rootAction.addAction(syncAction);
		
		ActionChild syncActionChild = ActionChild.createActionParam("request", "reset");
		syncAction.addActionChild(syncActionChild);
		
		
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
		
		connectionChild = new ActionChild("ApiParameter", "config.ism.ibm.com");
		connectionChild.addAttr("name", "path");
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
	
	public void addSubscribe() {
		
		Action subscribe = new Action("Subscribe", "Subscribe");
		rootAction.addAction(subscribe);
		
		ActionChild subscribeChild = ActionChild.createActionParam("connection_id", "CF1");
		subscribe.addActionChild(subscribeChild);
		
		
		subscribeChild = new ActionChild("ApiParameter", topic);
		subscribeChild.addAttr("name", "topic");
		subscribe.addActionChild(subscribeChild);
		
		subscribeChild = new ActionChild("ApiParameter", "0");
		subscribeChild.addAttr("name", "QoS");
		subscribe.addActionChild(subscribeChild);
		
		

		
	}
	
	public void addSyncComponentsReceiver() {
		
		Action message = new Action("sync_components_receiver", "SyncComponentAction");
		rootAction.addAction(message);
		
		ActionChild messageChild = ActionChild.createActionParam("component_name", "receiver");
		message.addActionChild(messageChild);
		
		messageChild = ActionChild.createActionParam("component_list", "sender;receiver");
		message.addActionChild(messageChild);
		
		messageChild = ActionChild.createActionParam("timeout", "30000");
		message.addActionChild(messageChild);
	
		
	}
	
	public void addReceiveMsg() {
		
		Action messageText = new Action("ReceiveMessage", "ReceiveMessage");
		rootAction.addAction(messageText);
		
		ActionChild messageTextChild = ActionChild.createActionParam("connection_id", "CF1");
		messageText.addActionChild(messageTextChild);
		
		messageTextChild = ActionChild.createActionParam("structure_id", "rxmsg1");
		messageText.addActionChild(messageTextChild);
		
		messageTextChild = ActionChild.createActionParam("waitTime", "10000");
		messageText.addActionChild(messageTextChild);
		
		
		
	}
	
	public void addCompareMessageData() {
		
		Action compare = new Action("CompareMessageData", "CompareMessageData");
		rootAction.addAction(compare);
		
		ActionChild compareChild = ActionChild.createActionParam("structure_id", "rxmsg1");
		compare.addActionChild(compareChild);
		
		compareChild = ActionChild.createActionParam("compareBody", "TEXT: test content 1");
		compare.addActionChild(compareChild);
		
	}
	
	public void addCloseConnection()  {
		
		Action closeConnection = new Action("CloseConnection", "CloseConnection");
		rootAction.addAction(closeConnection);
		
		ActionChild closeConnectionChild = ActionChild.createActionParam("connection_id", "CF1");
		closeConnection.addActionChild(closeConnectionChild);
		
	}
	
	
	public void buildReceiver() {
		//addSyncAction();
		addConnection();
		addSubscribe();
		addSyncComponentsReceiver();
		addReceiveMsg();
		addCompareMessageData();
		addCloseConnection();

		
		
		
	}

	public String getType() {
		return type;
	}

	public void setType(String type) {
		this.type = type;
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
		return "mqtt_rmdr_policy_validation_" + port + ".xml";
	}


	public String getDriverName() {
		return "wsDriver";
	}
	
	public String getRootElementName() {
		return "IsmWSTest";
	}
	

}
