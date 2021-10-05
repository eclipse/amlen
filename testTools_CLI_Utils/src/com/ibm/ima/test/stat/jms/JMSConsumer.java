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

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.ImaSubscription;
import com.ibm.ima.test.stat.Config;
import com.ibm.ima.test.stat.Config.DEST_TYPE;
import com.ibm.ima.test.stat.ExecutionMonitor;
import com.ibm.ima.test.stat.ExecutionStatus;
import com.ibm.ima.test.stat.ExecutionStatus.CLIENT_TYPE;
import com.ibm.ima.test.stat.ExecutionStatus.STATUS_TYPE;

/**
 * JMSConsumer is a JMS subscriber-based test driver. Each instance
 * of JMSConsumer is a thread and manages its own resources with
 * respect to connections and subscriptions. An instance of JMSConsumer
 * may be configured to run for a specific amount of time or consume
 * a specific number of messages.
 *
 */
public class JMSConsumer implements Runnable, MessageListener {

	// Config instance with configuration data
	private Config config = null;
	// unique client ID
	private String clientId = null;
	// verbose output to console
	private boolean verboseOutput = false;
	// 0 all is good 1 something not right
	private int exitCode = 0;
	// A monitor instance where execution status will be sent
	private ExecutionMonitor monitor = null;
	// status instance
	private ExecutionStatus exitStatus = null;
	// how many messages this instance consumed
	long msgsConsumed = 0;
	// an index to append to durable subscription names
	int threadIndex = 0;
	// the Connection instance 
	private Connection connection = null;
	// a Session instance
	private Session session = null;
	// A message consumer
	private MessageConsumer msgConsumer = null;
	// the destination that is subscribed to (Queue or Topic)
	private Destination destination = null;
	private boolean initialized = false;
	// how many times has this instance failed to initialize
	private long initFailures = 0;
	// the username used for the last connection
	private String username = null;

	/**
	 * Create a new instance of JMSConumer. The instance may subscribe to 
	 * @param config           Config instance containing all the configuration
	 *                         parameters needed for this instance of JMSConsumer
	 * @param clientId         A unique client id for this consumer
	 * @param threadIndex      An index corresponding to the number of consumer
	 *                         threads active
	 * @param monitor          The Execution monitor to send status to
	 * @param verboseOutput    Perform verbose output or not
	 */
	public JMSConsumer(Config config, String clientId, int threadIndex, ExecutionMonitor monitor, boolean verboseOutput) {

		this.config = config;
		this.clientId = clientId;
		this.monitor = monitor;
		this.verboseOutput = verboseOutput;
		this.threadIndex = threadIndex;

		exitStatus = new ExecutionStatus(clientId, CLIENT_TYPE.JMS_CONSUMER, STATUS_TYPE.EXIT_STATUS);

	}

	/**
	 * Get a new connection for this JMSConsumer instance. 
	 * 
	 * @return
	 * @throws JMSException
	 */
	private Connection getConnection() throws JMSException {

		ConnectionFactory connectionFactory = ImaJmsFactory.createConnectionFactory();

		//((ImaProperties) connectionFactory).put("ClientID", clientId);
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
		if (!config.isAnonymous())
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

		if (destType.equals(DEST_TYPE.QUEUE)) {
			destination = session.createQueue(config.getDestinationName());
		} else {
			destination = session.createTopic(config.getDestinationName());
		}

		return destination;
	}

	private MessageConsumer getMsgConsumer(Session session, Destination dest) throws JMSException {

		MessageConsumer msgConsumer = null; //session.createConsumer(dest);
		DEST_TYPE destType = config.getDestinationType();
		if (destType.equals(DEST_TYPE.QUEUE)) {
			msgConsumer = session.createConsumer(dest);
		} else {
			// The destination is a topic
			if (config.getSubscriptionName() != null) {
				if (verboseOutput) {
					System.out.println("creating message consumer for subscription " + config.getSubscriptionName());
				}
				if (config.isShared()) {
					if (config.isCleanStore()) {
						// This is a shared non-durable subscription
						if (verboseOutput) {
							System.out.println("setting shared non-durable subscription " + config.getSubscriptionName() + threadIndex);
						}
						msgConsumer = ((ImaSubscription)session).createSharedConsumer((Topic)dest, config.getSubscriptionName() + threadIndex);
					} else {
						// This is a shared durable subscription
						if (verboseOutput) {
							System.out.println("setting shared durable subscription " + config.getSubscriptionName() + threadIndex);
						}
						msgConsumer = ((ImaSubscription)session).createSharedDurableConsumer((Topic)dest, config.getSubscriptionName() + threadIndex);
					}
				} else {
					// we need to create a durable subscription...
					if (verboseOutput) {
						System.out.println("setting durable subscription " + config.getSubscriptionName() + threadIndex);
					}
					msgConsumer = session.createDurableSubscriber((Topic)dest, config.getSubscriptionName() + threadIndex);
				}
			} else {
				// non-durable...
				msgConsumer = session.createConsumer(dest);
			}
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

	private void unsubscribe(Session session) {

		try {

			if (config.doForceUnsubscribe() ||((config.isCleanStore() && config.getDestinationType().equals(DEST_TYPE.TOPIC)))) {
				if (config.getSubscriptionName() != null) {
					if (verboseOutput) {
						System.out.println("unsubscribing from " + config.getSubscriptionName() + threadIndex);
					}
					session.unsubscribe(config.getSubscriptionName() + threadIndex);
				}
			}

		} catch (Throwable t) {
			if (verboseOutput && initialized) {
				t.printStackTrace();
			}
		}
	}


	/**
	 * Run the JMSConumer thread. Entry point for thread
	 * execution. This method will delegate to a timed run or
	 * to a specific number run (consume a specific number of
	 * messages)
	 */
	public void run()  {

		// check if there is a specific number specified
		if (config.getMsgsToConsume() <= 0) {
			// nothing specific - do timed run...
			consumeTimed();
		} else { 				
			// attempt to consume number specified
			consumeSpecificNumber();
		}

	}

	/**
	 * 
	 * @throws JMSException
	 */
	private void init() throws JMSException {

		try {

			if (session == null || connection == null) {
				if (verboseOutput) {
					System.out.println("Client <"+ clientId + "> obtaining connection...");
				}
				closeConnection(connection);
				closeSession(session);
				connection = getConnection();
				session = getSession(connection);
				destination = getDestination(session);
				msgConsumer = getMsgConsumer(session, destination);
				connection.start();
			} else {
				msgConsumer = getMsgConsumer(session, destination);
			}

			ExecutionStatus consumerInitStatus = new ExecutionStatus(clientId, CLIENT_TYPE.JMS_CONSUMER, STATUS_TYPE.INITIALIZED_STATUS);
			monitor.reportStatus(consumerInitStatus);

			System.out.println("JMS Consumer :" + clientId + " subscribed to dest: '" + config.getDestinationName() + "'.");

			initialized = true;

		} catch (JMSException jme) {
			initFailures++;
			throw jme;
		}

	}

	/**
	 * Do a timed run where JMSConsumer attempts to consume as
	 * many messages as possible in the time specified. The time
	 * is specified in the Config object provided to JMSConsumer
	 * 
	 */
	private void consumeTimed() {

		long startTime = System.currentTimeMillis();

		try {

			long currentTime = System.currentTimeMillis();
			long endTime = currentTime + (config.getTotalSeconds() * 1000);

			while ((currentTime < endTime) ) {

				try {

					if (!initialized) {	
						init();
					}

					getMessage(msgConsumer);

				} catch (JMSException jmse) {
					System.out.println(jmse.getMessage() + " current username <" + username + ">");
					if (config.isIgnoreErrors() == false) {
						throw jmse;
					} else if (verboseOutput) {
						jmse.printStackTrace();
					}

				}

			//	Thread.sleep(config.getConsumeSleepInterval());
				currentTime = System.currentTimeMillis();

			} 
			exitCode = 0;
		}  catch (Throwable t) {
			exitCode = 1;
			t.printStackTrace();
		} finally {

			long endTime = System.currentTimeMillis();
			stopConnection(connection);
			closeMsgConsumer(msgConsumer);
			unsubscribe(session);
			closeSession(session);
			closeConnection(connection);
			exitStatus.consumerExit(exitCode, startTime, endTime, msgsConsumed);
			monitor.reportStatus(exitStatus);

		}

	}


	/**
	 * Attempt to consume a specific number of messages. The number of
	 * message to consume is specified in the Config object specified when
	 * the JMSProducer is created.
	 * 
	 */
	private void consumeSpecificNumber() {

		// start time for entire execution...
		long startTime = System.currentTimeMillis();

		try {

			while(msgsConsumed < config.getMsgsToConsume()) {

				try {

					if (!initialized) {	
						// we have too many initialization failures - break
						if (initFailures >= config.getMsgsToConsume()) {
							break;
						}
						init();
					}

					getMessage(msgConsumer);


				} catch (JMSException jmse) {
					System.out.println(jmse.getMessage() + " current username <" + username + ">");
					if (config.isIgnoreErrors() == false) {
						// do not ignore errors - get out of here
						throw jmse;
					} else if (verboseOutput) {
						jmse.printStackTrace();
					}
				}

				Thread.sleep(config.getConsumeSleepInterval());

			} 

			exitCode = 0;

		}  catch (Throwable t) {

			exitCode = 1;
			t.printStackTrace();
			
		} finally {

			// clean up and disconnect...
			long endTime = System.currentTimeMillis();
			stopConnection(connection);
			closeMsgConsumer(msgConsumer);
			unsubscribe(session);
			closeSession(session);
			closeConnection(connection);
			exitStatus.consumerExit(exitCode, startTime, endTime, msgsConsumed);
			monitor.reportStatus(exitStatus);

		}

	}

	@Override
	public void onMessage(Message msg) {

		try {
			msg.acknowledge();
		} catch (JMSException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

	}

	private Message getMessage(MessageConsumer msgConsumer) throws JMSException {

		if (verboseOutput) {
			System.out.println("Consumer client : " + clientId + " waiting to receive...");
		}

		Message msg = msgConsumer.receive(config.getConsumeWaitForMsgs());

		if (msg != null) {

			if (verboseOutput) {
				System.out.println("Receiving messages from destination '" + 
						config.getDestinationName() + "' : " + ((TextMessage) msg).getText());
				System.out.println("Consumer client : " + clientId + " has consumed " + msgsConsumed + " messages.");
			}

			msgsConsumed++;

		} else {

			if (verboseOutput) {
				System.out.println("Consumer client : " + clientId + " receive timeout reached. No messge...");
			}

		}	

		return msg;
	}


	public void setVerboseOutput(boolean verboseOutput) {
		this.verboseOutput = verboseOutput;
	}

	public int getExitCode() {
		return exitCode;
	}




}
