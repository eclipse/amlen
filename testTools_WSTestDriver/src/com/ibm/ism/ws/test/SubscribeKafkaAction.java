/*
 * Copyright (c) 2019-2021 Contributors to the Eclipse Foundation
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


package com.ibm.ism.ws.test;

/*
 * SubscribeKafkaAction is the action class to subscribe to a topic from Kafka brokers.
 * Following is an XML example
 * 
 *      <Action name="SubscribeEventStreams" type="SubscribeKafka" >
            <ActionParameter name="connectionID">eventstreams</ActionParameter>
            <ActionParameter name="topic">test-topic-1</ActionParameter>
        </Action>

 */
import java.util.List;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class SubscribeKafkaAction extends ApiCallAction {
	
	private final String _topic;
	private final String _connectionID;

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public SubscribeKafkaAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		
		super(config, dataRepository, trWriter);
		_connectionID=_actionParams.getProperty("connectionID");
		
		_topic = _actionParams.getProperty("topic");
		

	}

	protected boolean invokeApi() throws IsmTestException {

		connectKafka = (MyConnectionKafka) _dataRepository.getVar(_connectionID);
		connectKafka.subscribe(_topic);
		return true;
	}

}
