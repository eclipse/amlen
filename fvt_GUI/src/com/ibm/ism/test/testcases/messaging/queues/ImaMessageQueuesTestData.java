package com.ibm.ism.test.testcases.messaging.queues;


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
import com.ibm.ism.test.common.IsmTestData;

public class ImaMessageQueuesTestData extends IsmTestData {

	public static final String KEY_MESSAGE_QUEUE_NAME = "MESSAGE_QUEUE_NAME";
	public static final String KEY_MESSAGE_QUEUE_DESC = "MESSAGE_QUEUE_DESC";
	public static final String KEY_MESSAGE_QUEUE_MAX_MESSAGES = "MESSAGE_QUEUE_MAX_MESSAGES";
	public static final String KEY_MESSAGE_QUEUE_ALLOW_SEND = "MESSAGE_QUEUE_ALLOW_SEND";
	public static final String KEY_MESSAGE_QUEUE_CONCURRENT_CONSUMERS = "MESSAGE_QUEUE_CONCURRENT_CONSUMERS";
	
	public ImaMessageQueuesTestData(String[] args) {
		super(args);
	}
	
	public String getMessageQueueName() {
		return getProperty(KEY_MESSAGE_QUEUE_NAME);
	}
	
	public void setMessageQueueName(String name) {
		setProperty(KEY_MESSAGE_QUEUE_NAME, name);
	}
	

	public String getNewMessageQueueDescription() {
		return getProperty(KEY_MESSAGE_QUEUE_DESC);
	}	

	public String getNewMessageQueueMaxMessages() {
		return getProperty(KEY_MESSAGE_QUEUE_MAX_MESSAGES);
	}
	
	public boolean isNewMessageQueueAllowSend() {
		boolean allowSend = true;
		if (getProperty(KEY_MESSAGE_QUEUE_ALLOW_SEND) != null) {
			if (getProperty(KEY_MESSAGE_QUEUE_ALLOW_SEND).equalsIgnoreCase("false")) {
				allowSend = false;
			}
		}
		return allowSend;
	}
	
	public boolean isNewMessageQueueAllowConcurentConsumers() {
		boolean allowSend = true;
		if (getProperty(KEY_MESSAGE_QUEUE_CONCURRENT_CONSUMERS) != null) {
			if (getProperty(KEY_MESSAGE_QUEUE_CONCURRENT_CONSUMERS).equalsIgnoreCase("false")) {
				allowSend = false;
			}
		}
		return allowSend;
	}
	
}

