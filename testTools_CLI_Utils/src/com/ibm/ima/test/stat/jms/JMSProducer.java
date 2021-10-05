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
package com.ibm.ima.test.stat.jms;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.test.stat.Config;
import com.ibm.ima.test.stat.ExecutionMonitor;
import com.ibm.ima.test.stat.ExecutionStatus;
import com.ibm.ima.test.stat.Config.DEST_TYPE;
import com.ibm.ima.test.stat.ExecutionStatus.CLIENT_TYPE;
import com.ibm.ima.test.stat.ExecutionStatus.STATUS_TYPE;

/**
 * JMSProducer is a JMS based test driver that may be used
 * to produce (send or publish) messages to an IMA Server.
 * The JMSProducer instance is highly configurable and supports
 * a wide range of options.
 *
 */
public class JMSProducer implements Runnable {

	private Config config = null;
	private String clientId = null;
	private boolean verboseOutput = false;
	private ExecutionMonitor monitor = null;
	private ExecutionStatus exitStatus = null;
	private int exitCode = 0;

	private Connection connection = null;
	private Session session = null;
	private MessageProducer msgProducer = null;
	private Destination destination = null;
	private boolean initialized = false;
	private long initFailures = 0;


	private long msgsProduced = 0;
	private long goodSends = 0;
	private long badSends = 0;
	long startTime = 0;
	
	private String username = null;

	public JMSProducer(Config config, String clientId, ExecutionMonitor monitor, boolean verboseOutput) {

		this.config = config;
		this.clientId = clientId;
		this.monitor = monitor;
		this.verboseOutput = verboseOutput;
		exitStatus = new ExecutionStatus(clientId, CLIENT_TYPE.JMS_PRODUCER, STATUS_TYPE.EXIT_STATUS);
	}

	private Connection getConnection() throws JMSException {

		ConnectionFactory connectionFactory = ImaJmsFactory.createConnectionFactory();

		if (config.getQos() == 0) {
			((ImaProperties)connectionFactory).put("DisableACK", true);
		}
		if (Config.isSsl()) {
			((ImaProperties) connectionFactory).put("Protocol", "tcps");
			((ImaProperties) connectionFactory).put("SecurityProtocols", Config.getSslProtocol().getText());
			String cipherSuite = Config.getCipher();
			if (cipherSuite != null && cipherSuite.length() > 0) {
				((ImaProperties) connectionFactory).put("CipherSuites", Config.getCipher());
			}
		}
		((ImaProperties) connectionFactory).put("Server", config.getServer());
		((ImaProperties) connectionFactory).put("Port", config.getPort());

		username = config.getUsername();
		Connection connection = null;

		if (username != null && username.length() > 0) {
			connection = connectionFactory.createConnection(username, config.getPassword());
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
		DEST_TYPE destType = config.getDestinationType();

		if (!config.getDestDisableACK()) {
			if (destType.equals(DEST_TYPE.QUEUE)) {
				destination = session.createQueue(config.getDestinationName());
			} else {
				destination = session.createTopic(config.getDestinationName());
			}
		} else {
			if (destType.equals(DEST_TYPE.QUEUE)) {
				destination = ImaJmsFactory.createQueue(config.getDestinationName());
			} else {
				destination = ImaJmsFactory.createTopic(config.getDestinationName());
			}
			((ImaProperties)destination).put("DisableACK",true);
		}

		return destination;
	}

	private MessageProducer getMsgProducer(Session session, Destination destination) throws JMSException {

		MessageProducer msgProducer = session.createProducer(destination);
		if ((config.getQos()== 0) || config.getDestDisableACK())
			msgProducer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
		if (verboseOutput) {
			System.out.println("Producer delivery mode is "+(msgProducer.getDeliveryMode() == DeliveryMode.NON_PERSISTENT ? "NON persistent.":"PERSISTENT."));
		}
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

	public void run()  {


		if (config.getMsgsToSend() <= 0) {

			produceTimed();

		} else {
			produceSpecificNumber();

		} 

	}
	
	/**
	 * Attempt to connect and establish a session with the IMA Server.
	 * This is done using the configuration data provided in the 
	 * Config instance provided when instantiating the JMSProducer.
	 * 
	 * @throws JMSException  If anything goes wrong...
	 */
	private void init() throws JMSException {

		try {

			// if the Connection or Session are null try to connect from beginning
			if (session == null || connection == null) {
				closeConnection(connection);
				connection = getConnection();
				session = getSession(connection);
				destination = getDestination(session);
			}

			msgProducer = getMsgProducer(session, destination);

			// The JMSProducer is initialized and ready to send messages...
			ExecutionStatus consumerInitStatus = new ExecutionStatus(clientId, CLIENT_TYPE.JMS_CONSUMER, STATUS_TYPE.INITIALIZED_STATUS);
			monitor.reportStatus(consumerInitStatus);

			System.out.println("JMS Producer '" +  clientId + "' ready to send to destination: '" + config.getDestinationName() + "'.");

			initialized = true;

		} catch (JMSException jme) {
			initFailures++;
			throw jme;
		} 

	}

	/**
	 * Send or publish messages for a specific amount of time. The
	 * amount of time that should be spent sending messages is defined
	 * in the Config instance that was provide when JMSProducer was
	 * instantiated. 
	 * 
	 */
	private void produceTimed() {

		// for total time of execution....
		startTime = System.currentTimeMillis();

		try {

			// figure out how long the producer should run...
			long currentTime = System.currentTimeMillis();
			long consumeEndTime = currentTime + (config.getTotalSeconds() * 1000);

			// get a payload...
			// TODO - get new payload every 100 messages?
			String payload = config.getPayload();
			
			// print out the size as a sanity check...
			if (verboseOutput) {
				int size = getMessageSizeInBytes(payload);
				System.out.println("Producer '" +  clientId + "' payload size is " + size +" bytes" );
			}

			while (currentTime < consumeEndTime) {

				try {

					if (!initialized) {
						init();
					}
					msgsProduced++;
					TextMessage tmsg = session.createTextMessage(payload);
					msgProducer.send(tmsg);
					goodSends++;

				} catch (JMSException jmse) {
					System.out.println(jmse.getMessage() + " current username <" + username + ">");
					
					if (initialized) {
						badSends++;
					}

					if (config.isIgnoreErrors() == false) {
						throw jmse;
					} else if (verboseOutput) {
						jmse.printStackTrace();
					}
				}

				// if requested to sleep...
				Thread.sleep(config.getSendInterval());
				
				// update the current time...
				currentTime = System.currentTimeMillis();

			}
			exitCode = 0;
		} catch (Throwable t) {

			exitCode = 1;
			t.printStackTrace();

		} finally {

			// tidy up and get the time execution ended...
			long endTime = System.currentTimeMillis();
			closeMsgProducer(msgProducer);
			closeSession(session);
			closeConnection(connection);
			// send a status report to the execution monitor
			exitStatus.producerExit(exitCode, startTime, endTime, msgsProduced, goodSends, badSends);
			monitor.reportStatus(exitStatus);

		}

	}

	private void produceSpecificNumber() {


		startTime = System.currentTimeMillis();

		try {
			
			String payload = config.getPayload();
			int size = getMessageSizeInBytes(payload);
			if (verboseOutput) {
				System.out.println("Producer '" +  clientId + "' payload size is " + size +" bytes" );
			}

			for (int i =0; i < config.getMsgsToSend(); i++ ) {

				try {

					if (!initialized) {
						init();
					}
					msgsProduced++;
					TextMessage tmsg = session.createTextMessage(payload);
					msgProducer.send(tmsg);
					goodSends++;

				} catch (JMSException jmse) {
					System.out.println(jmse.getMessage() + " current username <" + username + ">");
					if (initialized) {
						badSends++;
					}

					if (config.isIgnoreErrors() == false) {
						throw jmse;
					} else if (verboseOutput) {
						jmse.printStackTrace();
					}
				}

				Thread.sleep(config.getSendInterval());


			}
			exitCode = 0;
		} catch (Throwable t) {

			exitCode = 1;
			t.printStackTrace();

		} finally {

			long endTime = System.currentTimeMillis();
			closeMsgProducer(msgProducer);
			closeSession(session);
			closeConnection(connection);
			exitStatus.producerExit(exitCode, startTime, endTime, msgsProduced, goodSends, badSends);
			monitor.reportStatus(exitStatus);

		}


	}

	private int getMessageSizeInBytes(String message) throws IOException {

		if (message == null) { return 0;}
		ByteArrayOutputStream baos = new ByteArrayOutputStream();
		ObjectOutputStream oos = new ObjectOutputStream(baos);
		oos.writeObject(message);
		oos.close();
		return baos.size() - 7;
	}


	public void setVerboseOutput(boolean verboseOutput) {
		this.verboseOutput = verboseOutput;
	}

}
