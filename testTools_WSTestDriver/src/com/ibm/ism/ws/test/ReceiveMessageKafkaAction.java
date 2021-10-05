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


import java.util.HashMap;
import java.util.Map;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;
import org.apache.kafka.clients.consumer.ConsumerRecord;

/*
 * ReceiveMessageKafkaAction is the action class for receive a Kafka message
 * The execution is blocked when waiting for an incoming message
 * Following is an XML example
 *      <Action name="ReceiveMessageEventStreams" type="ReceiveMessageKafka" >
            <ActionParameter name="connectionID">eventstreams</ActionParameter>
            <ActionParameter name="poll_duration">10000</ActionParameter>
        </Action>
 */
public class ReceiveMessageKafkaAction extends ApiCallAction {
	
	private final String _connectionID;
	private final String _msgID;
	
	//protected final int _poll_duration = 10000;
	protected final int _poll_duration;


	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public ReceiveMessageKafkaAction(Element config, DataRepository dataRepository
			, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_connectionID=_actionParams.getProperty("connectionID");
		_msgID = _connectionID + "MSG";
		_poll_duration =  Integer.parseInt(_actionParams.getProperty("poll_duration", "2000"));  // poll duration with default to 2000

	}

	protected boolean invokeApi() throws IsmTestException {

		connectKafka = (MyConnectionKafka) _dataRepository.getVar(_connectionID);
		ConsumerRecord<String, String> msg = connectKafka.receiveMessage(_poll_duration);
		if (null == msg) {
			throw new IsmTestException("ISMTEST"+(Constants.RECEIVEMESSAGE+9)
					, "No Eventstreams message was received in poll duration of " + _poll_duration);
		}
		_dataRepository.storeVar(_msgID, msg);

		return true;
	}

}
