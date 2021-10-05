/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

import java.util.concurrent.BlockingQueue;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.TextMessage;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

/**
 * JMSConsumer is a simple JMS based PolicyValidtor implementation 
 * for a JMS consumer/subscriber. This PolicyValidator may be
 * used to send a validation message to a topic or queue using
 * the JMS protocol.
 *
 */
public class JMSConsumer implements Runnable, PolicyValidator {

	private String clientId = null;
	private String server = null;
	private String port = null;
	private String destinationName = null;
	private String username = null;
	private String password = null;
	private String expectedMsg = null;
	private long timeout = 5000;
	private BlockingQueue<String> taskMonitor = null;
	private ValidationException error = null;
	boolean stopExecution = false;
	boolean queueConsumer = false;


	/**
	 * Create a JMSConsumer instance
	 * 
	 * @param server    The server host/address to connect to
	 * @param port      The port on the server to connect to
	 * @param destinationString  The destination (topic or queue) to connect to
	 * @param taskMonitor  A monitor that should be notified of failure or success
	 */
	public JMSConsumer(String server, String port, String destinationString, BlockingQueue<String> taskMonitor) {

		this.server = server;
		this.port = port;
		this.destinationName = destinationString;
		this.taskMonitor = taskMonitor;

	}

	/**
	 * Get a new connection for this JMSConsumer instance. 
	 * 
	 * @return  A valid Connection instance
	 * @throws JMSException  if something went wrong
	 */
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
			destination = session.createQueue(this.destinationName);
		} else {
			destination = session.createTopic(this.destinationName);
		}

		return destination;
	}

	private MessageConsumer getMsgConsumer(Session session, Destination dest) throws JMSException {

		MessageConsumer msgConsumer = null; //session.createConsumer(dest);

		if (queueConsumer) {
			msgConsumer = session.createConsumer(dest);
		} else {

			// non-durable...
			msgConsumer = session.createConsumer(dest);

		}
		return msgConsumer;

	}

	private void closeConnection(Connection connection) {

		try {
			//connection.stop();
			connection.close();
		} catch (Throwable t) {
			// do not care about exception here...
		}

	}

	private void stopConnection(Connection connection) {

		try {
			connection.stop();
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

	private void closeMsgConsumer(MessageConsumer msgConsumer) {

		try {
			msgConsumer.close();
		} catch (Throwable t) {
			// do not care about exception here...
		}

	}



	public void startValidation() {
		Thread t = new Thread(this);
		t.start();
	}

	/**
	 * Run the JMSConumer thread and validate that the correct 
	 * message is consumed.
	 * 
	 */
	public void run()  {


		Connection connection = null;
		Session session = null;
		Destination destination = null;
		MessageConsumer msgConsumer = null;

		try {

			connection = getConnection();
			session = getSession(connection);
			destination = getDestination(session);
			msgConsumer = getMsgConsumer(session, destination);
			connection.start();

			// connect and subscribe worked - 
			taskMonitor.add("subscribed");
			TextMessage msg = (TextMessage) msgConsumer.receive(timeout);

			if (msg != null && msg.getText().equals(expectedMsg)) {
				taskMonitor.add("received");
			} else {
				throw new Exception("Message expected [" + expectedMsg + "] message received [" + msg + "]");

			}

		} catch (JMSException jmse) {
			error = getValidationException(jmse);
			taskMonitor.add("failed");
		}  catch (Throwable t) {
			error = new ValidationException(t);
			taskMonitor.add("failed");
		} finally {

			stopConnection(connection);
			closeMsgConsumer(msgConsumer);
			closeSession(session);
			closeConnection(connection);

		}
	}


	/**
	 * initOnly can be used by test cases that only wish to 
	 * validate that the connection/subscribe worked.
	 * 
	 * @throws ValidationException
	 */
	public void initOnly() throws ValidationException {


		Connection connection = null;
		Session session = null;
		Destination destination = null;
		MessageConsumer msgConsumer = null;

		try {
			
			connection = getConnection();
			session = getSession(connection);
			destination = getDestination(session);
			msgConsumer = getMsgConsumer(session, destination);
			connection.start();
			
		} catch (JMSException jmse) {
			ValidationException ve = getValidationException(jmse);
			throw ve;
		} finally {
			stopConnection(connection);
			closeMsgConsumer(msgConsumer);
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


	public void setClientId(String clientId) {
		this.clientId = clientId;
	}

	public void setServer(String server) {
		this.server = server;
	}

	public void setPort(String port) {
		this.port = port;
	}

	public void setDestinationName(String destinationName) {
		this.destinationName = destinationName;
	}

	public void setUsername(String username) {
		this.username = username;
	}

	public void setPassword(String password) {
		this.password = password;
	}

	public void setTimeout(long timeout) {
		this.timeout = timeout;
	}


	public void stopExecution(boolean stopExecution) {
		this.stopExecution = stopExecution;
	}

	public ValidationException getError() {
		return error;
	}

	public void setValidationMsg(String msg) {
		this.expectedMsg = msg;

	}


}
