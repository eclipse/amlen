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


public class JMSReceiver implements IPolicyValidator {
	
	
	private String type = "CompositeAction";
	private String clientId = null;
	private String user = null;
	private String password = null;
	private String topic = null;
	private String port = null;
	private Action rootAction = null;
	private boolean built = false;
	
	
	
	public JMSReceiver(String topic, int port) {
		
		this.rootAction = new Action("rmdr", this.type);
		this.topic = topic;
		this.port = Integer.toString(port);
		
	}
	
	
	public void addSyncAction() {
		
		// Reset sync action
		Action syncAction = new Action("syncReset", "SyncAction");
		rootAction.addAction(syncAction);
		
		ActionChild syncActionChild = ActionChild.createActionParam("request", "reset");
		syncAction.addActionChild(syncActionChild);
		
		// Initialize sync driver
		Action syncdriverAction = new Action("syncInit1", "SyncAction");
		rootAction.addAction(syncdriverAction);
		
		ActionChild syncdriverActionChild1 = ActionChild.createActionParam("request", "init");
		syncdriverAction.addActionChild(syncdriverActionChild1);
		
		ActionChild syncdriverActionChild2 = ActionChild.createActionParam("condition", "syncPoint");
		syncdriverAction.addActionChild(syncdriverActionChild2);
		
	}
	
	
	public void addConnFactory() {
		
		// Connection Factory...
		Action connectionFactory = new Action("CreateRx_cf1", "CreateConnectionFactory");
		rootAction.addAction(connectionFactory);
		
		ActionChild connectionFactoryChild = ActionChild.createActionParam("structure_id", "rx_cf1");
		connectionFactory.addActionChild(connectionFactoryChild);
		
		
	}
	
	public void addIsmProps() {
	
		//rootAction.addComment("Hey this is comment 1");
		Action fillIsmProps = new Action("SetPropsRx_cf1", "FillIsmProps");
		rootAction.addAction(fillIsmProps);
		
	
		
		ActionChild fillIsmPropsChild = ActionChild.createActionParam("structure_id", "rx_cf1");
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
		Action connection = new Action("CreateConnectionRx_cf1", "CreateConnection");
		rootAction.addAction(connection);
		
		ActionChild connectionChild = ActionChild.createActionParam("structure_id", "connection_rx_cf1");
		connection.addActionChild(connectionChild);
		
		if (user != null) {
		connectionChild = new ActionChild("ApiParameter", user);
		connectionChild.addAttr("name", "user");
		connection.addActionChild(connectionChild);
		
		connectionChild = new ActionChild("ApiParameter", password);
		connectionChild.addAttr("name", "passwd");
		connection.addActionChild(connectionChild);
		
		}
		connectionChild = ActionChild.createActionParam("factory_id", "rx_cf1");
		connection.addActionChild(connectionChild);
		
		
		
	}
	
	public void addSession1() {
		
		// Connection Factory...
		Action session = new Action("CreateSession1Rx_conn_cf1", "CreateSession");
		rootAction.addAction(session);
		
		ActionChild sessionChild = ActionChild.createActionParam("conn_id", "connection_rx_cf1");
		session.addActionChild(sessionChild);
		
		sessionChild = ActionChild.createActionParam("structure_id", "session1_rx_cf1");
		session.addActionChild(sessionChild);
		
		
		
	}
	
	public void addSession2() {
		
		// Connection Factory...
		Action session = new Action("CreateSession2Rx_conn_cf1", "CreateSession");
		rootAction.addAction(session);
		
		ActionChild sessionChild = ActionChild.createActionParam("conn_id", "connection_rx_cf1");
		session.addActionChild(sessionChild);
		
		sessionChild = ActionChild.createActionParam("structure_id", "session2_rx_cf1");
		session.addActionChild(sessionChild);
		
		
		
	}
	
	public void addDestination() {
		
		// Connection Factory...
		Action dest = new Action("CreateRx_dest1", "CreateDestination");
		rootAction.addAction(dest);
		
		ActionChild destChild = ActionChild.createActionParam("structure_id", "rx_dest1");
		dest.addActionChild(destChild);
		
		
		destChild = ActionChild.createActionParam("type", "topic");
		dest.addActionChild(destChild);
		
		destChild = new ActionChild("ApiParameter", topic);
		destChild.addAttr("name", "name");
		dest.addActionChild(destChild);	
		
	}
	
	public void addConsumer1() {
		

		
		// Connection Factory...
		Action producer = new Action("CreateConsumer1Rx_dest1", "CreateConsumer");
		rootAction.addAction(producer);
		
		ActionChild producerChild = ActionChild.createActionParam("structure_id", "consumer1_dest1");
		producer.addActionChild(producerChild);
		
		producerChild = ActionChild.createActionParam("dest_id", "rx_dest1");
		producer.addActionChild(producerChild);
		
		producerChild = ActionChild.createActionParam("session_id", "session1_rx_cf1");
		producer.addActionChild(producerChild);
		
		
		
	}
	
public void addConsumer2() {
		

		
		// Connection Factory...
		Action producer = new Action("CreateConsumer2Rx_dest1", "CreateConsumer");
		rootAction.addAction(producer);
		
		ActionChild producerChild = ActionChild.createActionParam("structure_id", "consumer2_dest1");
		producer.addActionChild(producerChild);
		
		producerChild = ActionChild.createActionParam("dest_id", "rx_dest1");
		producer.addActionChild(producerChild);
		
		producerChild = ActionChild.createActionParam("session_id", "session2_rx_cf1");
		producer.addActionChild(producerChild);
		
		
		
	}
	
	public void addListener() {
		
		Action connection = new Action("CreateMsgListener_consumer1", "CreateMessageListener");
		rootAction.addAction(connection);
		
		ActionChild connectionChild = ActionChild.createActionParam("structure_id", "msg_listener_consumer1");
		connection.addActionChild(connectionChild);
	
	}
	
	public void addSetListener() {
		
		Action connection = new Action("SetMsgListener_consumer1", "SetMessageListener");
		rootAction.addAction(connection);
		
		ActionChild connectionChild = ActionChild.createActionParam("listener_id", "msg_listener_consumer1");
		connection.addActionChild(connectionChild);
		
		connectionChild = ActionChild.createActionParam("consumer_id", "consumer1_dest1");
		connection.addActionChild(connectionChild);
	
	}
	
	
	public void addStartConnection() {
		Action syncAction = new Action("StartConnectionRx_cf1", "StartConnection");
		rootAction.addAction(syncAction);
		
		ActionChild syncChild = ActionChild.createActionParam("conn_id", "connection_rx_cf1");
		syncAction.addActionChild(syncChild);
	
		
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
	
	
	public void addReceiveMsg1() {
		
		Action messageText = new Action("RcvTxtMessage_consumer1", "ReceiveMessage");
		rootAction.addAction(messageText);
		
		ActionChild messageTextChild = ActionChild.createActionParam("listener_id", "msg_listener_consumer1");
		messageText.addActionChild(messageTextChild);
		
		messageTextChild = ActionChild.createActionParam("structure_id", "rx_txt_msg1");
		messageText.addActionChild(messageTextChild);
		
		messageTextChild = new ActionChild("ApiParameter", "60000");
		messageTextChild.addAttr("name", "timeout");
		messageText.addActionChild(messageTextChild);
		
		
	}
	
	public void addGetTxtMsgConsumer1()  {
		
		Action sendMessage = new Action("GetMsgText_consumer1", "GetMessageText");
		rootAction.addAction(sendMessage);
		
		ActionChild sendMessageChild = ActionChild.createActionParam("message_id", "rx_txt_msg1");
		sendMessage.addActionChild(sendMessageChild);
		
		sendMessageChild = ActionChild.createActionParam("verifyValue", "TEXT: test content 1");
		sendMessage.addActionChild(sendMessageChild);
		
	}
	
public void addReceiveMsg2() {
		
		Action messageText = new Action("RcvTxtMessage_consumer2", "ReceiveMessage");
		rootAction.addAction(messageText);
		
		ActionChild messageTextChild = ActionChild.createActionParam("consumer_id", "consumer2_dest1");
		messageText.addActionChild(messageTextChild);
		
		messageTextChild = ActionChild.createActionParam("structure_id", "rx_txt_msg2");
		messageText.addActionChild(messageTextChild);
		
		messageTextChild = new ActionChild("ApiParameter", "60000");
		messageTextChild.addAttr("name", "timeout");
		messageText.addActionChild(messageTextChild);
		
		
	}
	
	public void addGetTxtMsgConsumer2()  {
		
		Action sendMessage = new Action("GetMsgText_consumer2", "GetMessageText");
		rootAction.addAction(sendMessage);
		
		ActionChild sendMessageChild = ActionChild.createActionParam("message_id", "rx_txt_msg2");
		sendMessage.addActionChild(sendMessageChild);
		
		sendMessageChild = ActionChild.createActionParam("verifyValue", "TEXT: test content 1");
		sendMessage.addActionChild(sendMessageChild);
		
	}
	
	
	
	public void addCloseConnection()  {
		
		Action closeConnection = new Action("CloseConnectionRx_cf1", "CloseConnection");
		rootAction.addAction(closeConnection);
		
		ActionChild closeConnectionChild = ActionChild.createActionParam("conn_id", "connection_rx_cf1");
		closeConnection.addActionChild(closeConnectionChild);
		
	}
	
	
	public void buildReceiver() {
		
		//addSyncAction();
		addConnFactory();
		addIsmProps();
		addConnection();
		addSession1();
		addSession2();
		addDestination();
		addConsumer1();
		addConsumer2();
		addListener();
		addSetListener();
		addStartConnection();
		addSyncComponentsReceiver();
		addReceiveMsg1();
		addGetTxtMsgConsumer1();
		addReceiveMsg2();
		addGetTxtMsgConsumer2();
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
		return "jms_rmdr_policy_validation_" + port + ".xml";
	}


	public String getDriverName() {
		return "jmsDriver";
	}
	
	public String getRootElementName() {
		return "LlmJmsTest";
	}
	

}
