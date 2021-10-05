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
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

/**
 * 
 * PolicyValidation is used as a wrapper to drive policy validation
 * test cases. PolicyValidation is used to start a consumer (subscriber)
 * and then send a message using a producer. This is done using two
 * PolicyValidator instances.
 * 
 *
 */
public class PolicyValidation  {

	// a monitor that is used to block and wait for messages from the consumer
	private BlockingQueue<String> consumerMonitor = new LinkedBlockingQueue<String>();

	// what kind of validation will be done
	private VALIDATION_TYPE validationType = null;

	private String producerClientId = null;
	private String consumerClientId = null;
	private String server = null;
	private String port = null;
	private String topicString = null;
	private String producerUsername = null;
	private String producerPassword = null;
	private String consumerUsername = null;
	private String consumerPassword = null;
	private String validationMsg = null;

	/**
	 * The types of validation available
	 *
	 */
	public enum VALIDATION_TYPE {
		JMS_TO_JMS,
		JMS_TO_MQTT,
		MQTT_TO_MQTT,
		MQTT_TO_JMS;
	}

	/**
	 * Create a new PolicyValidation instance of a specific type.
	 * 
	 * @param validationType  The type of validation (i.e. JMS to JMS ,etc..)
	 */
	public PolicyValidation(VALIDATION_TYPE validationType) {
		this.validationType = validationType;
	}


	/**
	 * Do the validation. Specifically this is used to validate that a certain policy
	 * scenario is indeed valid. This method will start a consumer PolicyValidator and 
	 * block until it is initialized and the star a producer PolicyValidator. The
	 * producer will then send a message and a validation will be done that the
	 * consumer received the message. 
	 * 
	 * The validation can also be used to test that a policy has enforced what should 
	 * be enforced. For example, protocol, client id, port etc... The appropriate 
	 * exception will be thrown when a consumer or producer encounters an error.
	 * 
	 * @throws ValidationException   When an error occurs
	 */
	public void validate() throws ValidationException {

		// Create the producer instance and set attributes
		PolicyValidator producer = null;
		if (validationType.equals(VALIDATION_TYPE.JMS_TO_MQTT) || validationType.equals(VALIDATION_TYPE.JMS_TO_JMS)) {
			producer = new JMSProducer(server, port, topicString);
		} else {
			producer = new MQTTProducer(server, port, topicString);
		}

		producer.setClientId(producerClientId);
		producer.setUsername(producerUsername);
		producer.setPassword(producerPassword);
		producer.setValidationMsg(validationMsg);

		// create the consumer instance and set attributes
		PolicyValidator consumer = null;
		if (validationType.equals(VALIDATION_TYPE.JMS_TO_MQTT) || validationType.equals(VALIDATION_TYPE.MQTT_TO_MQTT)) {
			consumer = new MQTTConsumer(server, port, topicString, consumerMonitor);
		} else {
			consumer = new JMSConsumer(server, port, topicString, consumerMonitor);
		}

		consumer.setClientId(consumerClientId);
		consumer.setUsername(consumerUsername);
		consumer.setPassword(consumerPassword);
		consumer.setValidationMsg(validationMsg);
		
		// start the consumer...
		consumer.startValidation();
		String consumerStatus = null;

		try {
			// block until the consumer sends a status message (or 10 seconds max)
			consumerStatus = consumerMonitor.poll(10, TimeUnit.SECONDS);
		} catch (InterruptedException e) {
			throw new ValidationException(e);
		}
		
		// make sure the consumer is initialized
		if (consumerStatus.equals("subscribed")) {
			producer.startValidation();
			if (producer.getError() != null) {
				// producer failed stop consumer
				consumer.stopExecution(true);
				consumerStatus = "producer failed";
			} else {
				try {
					// message sent - wait for consumer to validate
					consumerStatus = consumerMonitor.poll(10, TimeUnit.SECONDS);
				} catch (InterruptedException e) {
					throw new ValidationException(e);
				}

			} 

		}

		// check for consumer error
		if (consumer.getError() != null) {
			consumer.getError().printStackTrace();
			throw consumer.getError();
		}

		// check for producer error
		if (producer.getError() != null) {
			producer.getError().printStackTrace();
			throw producer.getError();
		}

	}

	public void setConsumerMonitor(BlockingQueue<String> consumerMonitor) {
		this.consumerMonitor = consumerMonitor;
	}

	public void setProducerClientId(String producerClientId) {
		this.producerClientId = producerClientId;
	}

	public void setConsumerClientId(String consumerClientId) {
		this.consumerClientId = consumerClientId;
	}

	public void setServer(String server) {
		this.server = server;
	}

	public void setPort(String port) {
		this.port = port;
	}

	public void setTopicString(String topicString) {
		this.topicString = topicString;
	}

	public void setValidationMsg(String validationMsg) {
		this.validationMsg = validationMsg;
	}

	public void setProducerUsername(String producerUsername) {
		this.producerUsername = producerUsername;
	}

	public void setProducerPassword(String producerPassword) {
		this.producerPassword = producerPassword;
	}

	public void setConsumerUsername(String consumerUsername) {
		this.consumerUsername = consumerUsername;
	}

	public void setConsumerPassword(String consumerPassword) {
		this.consumerPassword = consumerPassword;
	}


}
