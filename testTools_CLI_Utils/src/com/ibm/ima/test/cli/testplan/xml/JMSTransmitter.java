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

public class JMSTransmitter implements IPolicyValidator {

	private final static String actionString = "rmdt";
	private final static String type = "CompositeAction";
	private final static String driverName = "jmsDriver";
	private final static String rootElementName = "LlmJmsTest";

	private String clientId = null;
	private String user = null;
	private String password = null;
	private String port = null;
	private String topic = null;

	private Action rootAction = null;

	private boolean built = false;


	public JMSTransmitter(String topic, int port) {

		this.rootAction = new Action(actionString, type);

		this.topic = topic;
		this.port = Integer.toString(port);

	}

	public void addConnFactory() {

		// Connection Factory...
		Action connectionFactory = new Action("CreateTx_cf1", "CreateConnectionFactory");
		rootAction.addAction(connectionFactory);

		ActionChild connectionFactoryChild = ActionChild.createActionParam("structure_id", "tx_cf1");
		connectionFactory.addActionChild(connectionFactoryChild);

	}

	public void addIsmProps() {

		//rootAction.addComment("Hey this is comment 1");
		Action fillIsmProps = new Action("SetPropsTx_cf1", "FillIsmProps");
		rootAction.addAction(fillIsmProps);

		ActionChild fillIsmPropsChild = ActionChild.createActionParam("structure_id", "tx_cf1");
		fillIsmProps.addActionChild(fillIsmPropsChild);

		fillIsmPropsChild = ActionChild.createActionParam("validateAll", "true");
		fillIsmProps.addActionChild(fillIsmPropsChild);

		fillIsmPropsChild = ActionChild.createActionParam("validateNoWarn", "false");
		fillIsmProps.addActionChild(fillIsmPropsChild);

		if (clientId != null) {
			fillIsmPropsChild = ActionChild.createISMProperty("ClientID", clientId , "STRING");
			fillIsmProps.addActionChild(fillIsmPropsChild);
		}

		fillIsmPropsChild = new ActionChild("include", "../common/JMS_server.xml");
		fillIsmProps.addActionChild(fillIsmPropsChild);

		fillIsmPropsChild = ActionChild.createISMProperty("Port", port , "STRING");
		fillIsmProps.addActionChild(fillIsmPropsChild);

	}

	public void addConnection() {

		// Connection Factory...
		Action connection = new Action("CreateConnectionTx_cf1", "CreateConnection");
		rootAction.addAction(connection);

		ActionChild connectionChild = ActionChild.createActionParam("structure_id", "connection_tx_cf1");
		connection.addActionChild(connectionChild);

		if (user != null) {
			
			connectionChild = new ActionChild("ApiParameter", user);
			connectionChild.addAttr("name", "user");

			connection.addActionChild(connectionChild);

			connectionChild = new ActionChild("ApiParameter", password);
			connectionChild.addAttr("name", "passwd");
			connection.addActionChild(connectionChild);

		}
		
		connectionChild = ActionChild.createActionParam("factory_id", "tx_cf1");
		connection.addActionChild(connectionChild);

	}

	public void addSession() {

		// Connection Factory...
		Action session = new Action("CreateSession1Tx_conn_cf1", "CreateSession");
		rootAction.addAction(session);

		ActionChild sessionChild = ActionChild.createActionParam("conn_id", "connection_tx_cf1");
		session.addActionChild(sessionChild);

		sessionChild = ActionChild.createActionParam("structure_id", "session1_tx_cf1");
		session.addActionChild(sessionChild);

	}

	public void addDestination() {

		// Connection Factory...
		Action dest = new Action("CreateTx_dest1", "CreateDestination");
		rootAction.addAction(dest);

		ActionChild destChild = ActionChild.createActionParam("structure_id", "tx_dest1");
		dest.addActionChild(destChild);

		destChild = ActionChild.createActionParam("type", "topic");
		dest.addActionChild(destChild);

		destChild = new ActionChild("ApiParameter", topic);
		destChild.addAttr("name", "name");
		dest.addActionChild(destChild);	

	}

	public void addProducer() {

		// Connection Factory...
		Action producer = new Action("CreateProducer1Tx_dest1", "CreateProducer");
		rootAction.addAction(producer);

		ActionChild producerChild = ActionChild.createActionParam("structure_id", "producer1_dest1");
		producer.addActionChild(producerChild);

		producerChild = ActionChild.createActionParam("dest_id", "tx_dest1");
		producer.addActionChild(producerChild);

		producerChild = ActionChild.createActionParam("session_id", "session1_tx_cf1");
		producer.addActionChild(producerChild);

	}

	public void addStartConnection() {

		Action connection = new Action("StartConnectionTx_cf1", "StartConnection");
		rootAction.addAction(connection);

		ActionChild connectionChild = ActionChild.createActionParam("conn_id", "connection_tx_cf1");
		connection.addActionChild(connectionChild);

	}


	public void addSyncAction() {
		
		Action syncAction = new Action("sync_components_sender", "SyncComponentAction");
		rootAction.addAction(syncAction);

		ActionChild syncChild = ActionChild.createActionParam("component_name", "sender");
		syncAction.addActionChild(syncChild);

		syncChild = ActionChild.createActionParam("component_list", "sender;receiver");
		syncAction.addActionChild(syncChild);

		syncChild = ActionChild.createActionParam("timeout", "60000");
		syncAction.addActionChild(syncChild);

	}

	public void addCreateMessage() {

		Action message = new Action("CreateTxtMessage_prod_dest1", "CreateMessage");
		rootAction.addAction(message);

		ActionChild messageChild = ActionChild.createActionParam("structure_id", "tx_txt_msg1");
		message.addActionChild(messageChild);

		messageChild = ActionChild.createActionParam("session_id", "session1_tx_cf1");
		message.addActionChild(messageChild);

		messageChild = new ActionChild("ApiParameter", "TEXT");
		messageChild.addAttr("name", "msgType");
		message.addActionChild(messageChild);

	}


	public void addSetMessageText() {

		Action messageText = new Action("SetTxtMsg_prod_dest1", "SetMessageText");
		rootAction.addAction(messageText);

		ActionChild messageTextChild = ActionChild.createActionParam("message_id", "tx_txt_msg1");
		messageText.addActionChild(messageTextChild);

		messageTextChild = new ActionChild("ApiParameter", "TEXT: test content 1");
		messageTextChild.addAttr("name", "value");
		messageText.addActionChild(messageTextChild);

	}

	public void addSendMessage()  {

		Action sendMessage = new Action("SendTxtMessage_prod_dest1", "SendMessage");
		rootAction.addAction(sendMessage);

		ActionChild sendMessageChild = ActionChild.createActionParam("producer_id", "producer1_dest1");
		sendMessage.addActionChild(sendMessageChild);

		sendMessageChild = ActionChild.createActionParam("message_id", "tx_txt_msg1");
		sendMessage.addActionChild(sendMessageChild);

	}

	public void addStopConnection()  {

		Action stopConnection = new Action("StopConnection2Tx_cf1", "StopConnection");
		rootAction.addAction(stopConnection);

		ActionChild stopConnectionChild = ActionChild.createActionParam("conn_id", "connection_tx_cf1");
		stopConnection.addActionChild(stopConnectionChild);

	}


	public void addCloseConnection()  {

		Action closeConnection = new Action("CloseConnectionTx_cf1", "CloseConnection");
		rootAction.addAction(closeConnection);

		ActionChild closeConnectionChild = ActionChild.createActionParam("conn_id", "connection_tx_cf1");
		closeConnection.addActionChild(closeConnectionChild);

	}


	public void buildTransmitter() {

		addConnFactory();
		addIsmProps();
		addConnection();
		addSession();
		addDestination();
		addProducer();
		addStartConnection();
		addSyncAction();
		addCreateMessage();
		addSetMessageText();
		addSendMessage();
		addStopConnection();
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
			buildTransmitter();
			built = true;
		}
		return rootAction;
	}


	public String getXmlName() {
		return "jms_rmdt_policy_validation_" + port + ".xml";
	}

	public String getDriverName() {
		return driverName;
	}

	public String getRootElementName() {
		return rootElementName;
	}

}
