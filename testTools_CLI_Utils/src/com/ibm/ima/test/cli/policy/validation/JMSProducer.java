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
package com.ibm.ima.test.cli.policy.validation;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

/**
 * JMSProducer is a JMS based test driver that may be used
 * to produce (send or publish) messages to an IMA Server.
 * The JMSProducer instance is highly configurable and supports
 * a wide range of options.
 *
 */
public class JMSProducer implements PolicyValidator {

	private String clientId = null;
	private String server = null;
	private String port = null;
	private String destinationName = null;
	private String username = null;
	private String password = null;
	private String msgToSend = null;
	private ValidationException error = null;
	boolean queueConsumer = false;

	public JMSProducer(String server, String port, String destinationName) {

		this.server = server;
		this.port = port;
		this.destinationName = destinationName;


	}

	private Connection getConnection() throws JMSException {

		ConnectionFactory connectionFactory = ImaJmsFactory.createConnectionFactory();


		((ImaProperties) connectionFactory).put("Server", server);
		((ImaProperties) connectionFactory).put("Port", port);


		Connection connection = null;

		if (username != null && username.length() > 0) {
			connection = connectionFactory.createConnection(username, password);
		} else {
			connection = connectionFactory.createConnection();
		}
		connection.setClientID(clientId);

		return connection;

	}

	private Session getSession(Connection connection) throws JMSException {

		Session session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
		return session;

	}

	private Destination getDestination(Session session) throws JMSException {

		Destination destination = null;


		if (queueConsumer) {
			destination = session.createQueue(destinationName);
		} else {
			destination = session.createTopic(destinationName);
		}

		return destination;
	}

	private MessageProducer getMsgProducer(Session session, Destination destination) throws JMSException {

		MessageProducer msgProducer = session.createProducer(destination);
		return msgProducer;

	}

	private void closeConnection(Connection connection) {

		try {
			connection.close();
		} catch (Throwable t) {
			// do not care about exception here...
		}

	}

	private void closeSession(Session session) {

		try {
			session.close();
		} catch (Throwable t) {
			// do not care about exception here...
		}

	}

	private void closeMsgProducer(MessageProducer msgProducer) {

		try {
			msgProducer.close();
		} catch (Throwable t) {
			// do not care about exception here...
		}

	}

	public void startValidation()  {


		Connection connection = null;
		Session session = null;
		Destination destination = null;
		MessageProducer msgProducer = null;


		try {

			closeConnection(connection);
			connection = getConnection();
			session = getSession(connection);
			destination = getDestination(session);

			msgProducer = getMsgProducer(session, destination);

			TextMessage tmsg = session.createTextMessage(msgToSend);
			msgProducer.send(tmsg);

		} catch (JMSException jmse) {
			error = getValidationException(jmse);
		} catch (Throwable t) {
			error = new ValidationException(t);
		} finally {

			closeMsgProducer(msgProducer);
			closeSession(session);
			closeConnection(connection);

		}

	}
	
	private ValidationException getValidationException(JMSException jmse) {

		ValidationException ve = null;
		String errorCode = jmse.getErrorCode();
		
		if (errorCode != null) {
			if (errorCode.equals("CWLNC0022")) {
				ve = new ValidationConnException(jmse);
			} else if (errorCode.equals("CWLNC0207")) {
				ve = new ValidationAuthException(jmse);
			}
		}
		
		if (ve == null) {
			ve = new ValidationException(jmse);
		}
		
		return ve;

	}

	@Override
	public void setServer(String server) {
		this.server = server;

	}

	@Override
	public void setPort(String port) {
		this.port = port;

	}

	@Override
	public void setDestinationName(String destinationName) {
		this.destinationName = destinationName;

	}

	@Override
	public void setClientId(String clientId) {
		this.clientId = clientId;

	}

	@Override
	public void setUsername(String username) {
		this.username = username;

	}

	@Override
	public void setPassword(String password) {
		this.password = password;

	}

	@Override
	public void setValidationMsg(String msg) {
		this.msgToSend = msg;

	}

	@Override
	public ValidationException getError() {
		return error;
	}

	@Override
	public void stopExecution(boolean stopExecution) {
		// no-op

	}


}
