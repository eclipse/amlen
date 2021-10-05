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
package com.ibm.ima.mqcon.rules;

import java.io.IOException;
import java.security.InvalidParameterException;
import java.util.ArrayList;
import java.util.List;

import com.ibm.ima.mqcon.basetest.MQConnectivityTest;
import com.ibm.ima.mqcon.utils.MQConnection;
import com.ibm.ima.mqcon.utils.MqttConnection;
import com.ibm.ima.mqcon.utils.Trace;
import com.ibm.mq.MQException;
import com.ibm.mq.MQGetMessageOptions;
import com.ibm.mq.MQMessage;
import com.ibm.mq.MQQueue;
import com.ibm.mq.MQTopic;
import com.ibm.mq.constants.CMQC;
import com.ibm.mq.headers.MQDataException;
import com.ibm.mq.headers.MQHeaderIterator;

/**
 * base class to cover all cases where messages are sent to or from MQ using MQ Java
 * single direction classes will inherit from this
 */
public class MqJavaConnectivityTest extends MQConnectivityTest {
	// NB storing all messages, sent and received. 
	// Approach will not work for large messages or large numbers of messages
	/** 
	 * the list of messages sent in the test
	 */	
	protected List<String> sentMessages;
	/** 
	 * the list of messages received in the test
	 */	
	protected List<String> receivedMessages;
	/** 
	 * the list of duplicate messages received in the test
	 */	
	protected List<String> duplicateMessages;
	/**
	 * MQ Java connection
	 */
	protected MQConnection mqConn;
	/**
	 * name of (first) MQ queue manager
	 */
	protected String queueManagerName = "dttest3";
	/**
	 * host name of (first) machine hosting the MQ queue manager
	 */
	protected String mqHostName = "9.20.230.234";
	/**
	 * port used by MQ client app to connect to MQ
	 */
	protected int mqPort = 1414;
	/**
	 * channel used by MQ client app to connect to MQ
	 */
	protected String mqChannel = "JAVA.CHANNEL";
	/**
	 * flag to show if the test is HA aware and can tolerate failover
	 * most tests are not designed to failover
	 */
	protected boolean haAware = false;
	/** 
	 * topic string used in the test 
	 * can be both IMS and MQ in a topic to topic test
	 */
	protected String topicString;
	/** 
	 * queue name used in the test (MQ only)
	 */
	protected String mqQueueName;
	/** 
	 * topic string used in the test (MQ only)
	 */
	protected String mqTopicString;
	/** 
	 * queue name used in the test (IMS only)
	 */
	protected String imaQueueName;

	protected MQConnection secondMqConn;
	/**
	 * name of second MQ queue manager
	 */
	protected String secondQueueManagerName;
	/**
	 * host name of machine hosting the second MQ queue manager
	 */
	protected String secondmqHostName;
	/**
	 * port used by MQ client app to connect to second MQ queue manager
	 */
	protected int secondmqPort; 
	/**
	 * channel used by MQ client app to connect to second MQ queue manager
	 */
	protected String secondmqChannel;
	/**
	 * topic used by MQ client app on second MQ queue manager
	 */
	protected MQTopic secondMqTopic;
	/** 
	 * length of time to wait for a message in ms
	 */
	protected int waitTime = 10000;
	protected MqttConnection mqttcon;
	protected int mqper = CMQC.MQPER_PERSISTENT;
	protected boolean retainedMessages = false;
	public int expectedQos = 3;
	protected MQTopic mqTopic;
	protected MQQueue mqQueue;
	/*
	 * mapping for MQ java persistence to MQTT qos
	 					subscription qos, 
	 mqpersistence \	0		1		2
	 0					0		0		0
	 1					0		1		2
	 value = expected qos
	 */
	protected int [][]expectedQosArray = {{0,0,0},{0,1,2}};

	public MqJavaConnectivityTest () {
		this.sentMessages = new ArrayList<String>();
		this.receivedMessages = new ArrayList<String>();
		this.duplicateMessages = new ArrayList<String>();
		this.imaServerHost = "9.20.123.100";		
		this.imaServerPort = 16105;
	}
	/** 
	 * adds a message to the list of messages sent in the test
	 */	
	public synchronized void addMessage (String publication) {
		sentMessages.add(publication);
	}
	/** 
	 * gets the list of messages received in the test
	 */	
	public synchronized List<String> getReceivedMessages() {
		return receivedMessages;
	}
	/** 
	 * sets the list of messages received in the test
	 */	
	public synchronized void setReceivedMessages(List<String> receivedMessages) {
		this.receivedMessages = receivedMessages;
	}
	/** 
	 * updates the list of messages received in the test
	 * if the message has already been received, the message is added to the list of 
	 * duplicate messages, otherwise it is added to the list of received messages
	 */	
	public synchronized void updateReceivedMessages(String receivedMessage) {
		if (this.receivedMessages.contains(receivedMessage)) {
			updateDuplicateMessages(receivedMessage);
		} else {
			this.receivedMessages.add(receivedMessage);
		}
	}
	/** 
	 * gets the list of duplicate messages received in the test
	 */	
	public synchronized List<String> getDuplicateMessages() {
		return duplicateMessages;
	}
	/** 
	 * sets the list of duplicate messages received in the test
	 */	
	public synchronized void setDuplicateMessages(List<String> duplicateMessages) {
		this.duplicateMessages = duplicateMessages;
	}
	/** 
	 * adds a message to the list of duplicate messages sent in the test
	 */	
	public synchronized void updateDuplicateMessages(String receivedMessage) {
		this.duplicateMessages.add(receivedMessage);
	}
	/** 
	 * checks if a message has already been logged in receivedMessages
	 */	
	public synchronized boolean isDuplicate(String messagePayload) {
		if (receivedMessages.contains(messagePayload)){
			return true;
		}
		return false;
	}
	/** 
	 * checks if a message has already been logged in receivedMessages
	 */	
	public synchronized boolean checkMessagesOK() {
		boolean success = true;
		int unsentMessages = (getNumberOfMessages()  * getNumberOfPublishers())- sentMessages.size();
		if  (unsentMessages != 0) {
			System.out.println("MESSAGES UNSENT - TEST FAILED " + unsentMessages);
			success = false;
		}
		for (String payload : sentMessages) {
			if (getReceivedMessages().contains(payload)) {
				// everything OK
			} else {
				// message lost
				System.out.println("MESSAGE EXPECTED: " + payload + ". RECEIVED: " +getReceivedMessages());
				success = false;
			}
		}
		for (String receivedPayload : getReceivedMessages()) {
			if (sentMessages.contains(receivedPayload)) {
				// everything OK
			} else {
				// message lost
				System.out.println("TEST FAILED - EXTRA MESSAGE RECEIVED: " + receivedPayload);
				success = false;
			}
		}
		if (duplicateMessages.isEmpty()) {
			// no dupes - OK
		} else {
			System.out.println("TEST FAILED - DUPLICATE MESSAGES RECEIVED: " + duplicateMessages.size());
			success = false;
			if (Trace.traceOn()) {
				for (String dupe : duplicateMessages) {
					Trace.trace(dupe);
				}
			}		
		}
		return success;
	}

	protected void createMQConnection() {
		try {
			this.mqConn = new MQConnection(queueManagerName, mqHostName, mqPort, mqChannel);
		} catch (MQException e) {
			System.out.println("FAILED");
			e.printStackTrace();		
		}
	}

	protected String returnMqMessagePayloadAsString(MQMessage message) throws IOException, MQDataException {
		MQHeaderIterator it = new MQHeaderIterator (message);
		it.skipHeaders ();		
		String returnText = it.getBodyAsText();
		return returnText;
	}

	protected void checkJmsPersistence(int jmsPer) {
		if ((mqper +1) == jmsPer) {
			// everything OK - nothing to do
		} else {
			System.out.println("delivery mode " + jmsPer + " should be " + (mqper +1));
			setSuccess(false);
		}
	}

	protected MQGetMessageOptions getMessageWaitOptions() {
		MQGetMessageOptions messageOptions = new MQGetMessageOptions();
		messageOptions.options = CMQC.MQGMO_WAIT;
		messageOptions.waitInterval =waitTime;
		return messageOptions;
	}

	public void parseCommandLineArguments(String[] inputArgs) throws NumberFormatException {
		if (Trace.traceOn()){
			StringBuilder sb =new StringBuilder();
			for (String arg: inputArgs) {
				sb.append (arg + " ");
			}
			System.out.println("args are " + sb);
		}
		int x = 0;
		try {
			// the following piece of hideousness needs to be changed to use
			// switch / case when only java 7 will be used
			for (x=0;x < inputArgs.length; x++) {
				if (!inputArgs[x].startsWith("-")) {
					// oops - not a correct argument name
					throw new InvalidParameterException("invalid argument: " + inputArgs[x]);
				}				
				if (inputArgs[x].equalsIgnoreCase("-mqPort")) {
					x++;
					mqPort = Integer.valueOf(inputArgs[x]);
				} else if (inputArgs[x].equalsIgnoreCase("-mqHostName")) {
					x++;
					mqHostName = inputArgs[x];
				} else if (inputArgs[x].equalsIgnoreCase("-queueManagerName")) {
					x++;
					queueManagerName = inputArgs[x];
				} else if (inputArgs[x].equalsIgnoreCase("-mqChannel")) {
					x++;
					mqChannel = inputArgs[x];
				} else if (inputArgs[x].equalsIgnoreCase("-mqQueueName")) {
					x++;
					mqQueueName = inputArgs[x];
				} else if (inputArgs[x].equalsIgnoreCase("-numberOfMessages")) {
					x++;
					numberOfMessages = Integer.valueOf(inputArgs[x]);
				} else if (inputArgs[x].equalsIgnoreCase("-imaServerHost")) {
					x++;
					imaServerHost = inputArgs[x];
				} else if (inputArgs[x].equalsIgnoreCase("-imaServerPort")) {
					x++;
					imaServerPort = Integer.valueOf(inputArgs[x]);
				} else if (inputArgs[x].equalsIgnoreCase("-waitTime")) {
					x++;
					waitTime  = Integer.valueOf(inputArgs[x]);
				} else if (inputArgs[x].equalsIgnoreCase("-topicString")) {
					x++;
					topicString = inputArgs[x];
				} else if (inputArgs[x].equalsIgnoreCase("-mqTopicString")) {
					x++;
					mqTopicString = inputArgs[x];
				} else if (inputArgs[x].equalsIgnoreCase("-messagesExpected")) {
					x++;
					if (inputArgs[x].equalsIgnoreCase("true") || inputArgs[x].equalsIgnoreCase("false")) {
						messagesExpected = Boolean.valueOf(inputArgs[x]);
					} else {
						throw new InvalidParameterException("not a boolean: " + inputArgs[x]);
					}
				} else if (inputArgs[x].equalsIgnoreCase("-numberOfPublishers")) {
					x++;
					numberOfPublishers  = Integer.valueOf(inputArgs[x]);
				} else if (inputArgs[x].equalsIgnoreCase("-numberOfSubscribers")) {
					x++;
					numberOfSubscribers  = Integer.valueOf(inputArgs[x]);
				} else if (inputArgs[x].equalsIgnoreCase("-haAware")) {
					x++;
					if (inputArgs[x].equalsIgnoreCase("true") || inputArgs[x].equalsIgnoreCase("false")) {
						haAware  = Boolean.valueOf(inputArgs[x]);
					} else {
						throw new InvalidParameterException("not a boolean: " + inputArgs[x]);
					}
				} else if (inputArgs[x].equalsIgnoreCase("-imaQueueName")) {
					x++;
					imaQueueName = inputArgs[x];
				} else if (inputArgs[x].equalsIgnoreCase("-secondQueueManagerName")) {
					x++;
					secondQueueManagerName = inputArgs[x];
				} else if (inputArgs[x].equalsIgnoreCase("-secondmqHostName")) {
					x++;
					secondmqHostName = inputArgs[x];
				} else if (inputArgs[x].equalsIgnoreCase("-secondmqChannel")) {
					x++;
					secondmqChannel = inputArgs[x];
				} else if (inputArgs[x].equalsIgnoreCase("-secondmqPort")) {
					x++;
					secondmqPort = Integer.valueOf(inputArgs[x]);
				} else {
					// parameter will be handled by specific test - skip over the value
					x++;
				}
			}
		} catch (NumberFormatException nfe) {
			System.out.println("TEST FAILED - NumberFormatException for argument" 
					+ inputArgs[x-1] + " with value " + inputArgs[x]);
			throw nfe;
		}		
	}

	public boolean isRetainedMessages() {
		return retainedMessages;
	}

	public void setRetainedMessages(boolean retainedMessages) {
		this.retainedMessages = retainedMessages;
	}

	public String getSecondQueueManagerName() {
		return secondQueueManagerName;
	}

	public void setSecondQueueManagerName(String secondQueueManagerName) {
		this.secondQueueManagerName = secondQueueManagerName;
	}

	public String getSecondmqHostName() {
		return secondmqHostName;
	}

	public void setSecondmqHostName(String secondmqHostName) {
		this.secondmqHostName = secondmqHostName;
	}

	public int getSecondmqPort() {
		return secondmqPort;
	}

	public void setSecondmqPort(int secondmqPort) {
		this.secondmqPort = secondmqPort;
	}

	public String getSecondmqChannel() {
		return secondmqChannel;
	}

	public void setSecondmqChannel(String secondmqChannel) {
		this.secondmqChannel = secondmqChannel;
	}

	public MQTopic getSecondMqTopic() {
		return secondMqTopic;
	}

	public void setSecondMqTopic(MQTopic secondMqTopic) {
		this.secondMqTopic = secondMqTopic;
	}
	public MqttConnection getMqttcon() {
		return mqttcon;
	}
	public void setMqttcon(MqttConnection mqttcon) {
		this.mqttcon = mqttcon;
	}
}
