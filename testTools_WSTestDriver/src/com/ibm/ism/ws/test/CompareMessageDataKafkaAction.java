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

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Iterator;
import java.util.StringTokenizer;

import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.common.header.Header;
import org.apache.kafka.common.header.Headers;
import org.apache.kafka.common.header.internals.RecordHeaders;
import com.ibm.ism.mqtt.ImaJson;
/*
 * CompareMessageDataKafkaAction is the action class to compare the expected message information and the actual message
 * 
 * Following is an XML example to use
 * 
 *      <Action name="CheckMessageEventStreams" type="CompareMessageDataKafka">
            <ActionParameter name="connectionID">eventstreams</ActionParameter>
            <ActionParameter name="compareTopic">test-topic-1</ActionParameter>
            <ActionParameter name="compareValue">Go .....</ActionParameter>
            <ActionParameter name="compareKey">MyKey</ActionParameter>
            <ActionParameter name="comparePartition">0</ActionParameter>
            <ActionParameter name="compareHeaders">{"a":"b","a":"c","e":"0 - 9"}</ActionParameter>
        </Action>
 * 
 * 
 * 
 */


public class CompareMessageDataKafkaAction extends ApiCallAction {
	
	private final String _connectionID;
	private final String _msgID;
	
	private final String _compareTopic;
	private final String _compareValue;
	private final String _compareKey;
	private final String _comparePartition;
	private final String _compareHeaders;
	private final ImaJson _compareHeadersJson;
	//private Headers expectedHeaders = null;

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CompareMessageDataKafkaAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_connectionID=_actionParams.getProperty("connectionID");
		_msgID = _connectionID + "MSG";
		_compareTopic = _actionParams.getProperty("compareTopic");
		_compareValue = _actionParams.getProperty("compareValue");
		
		_compareKey = _actionParams.getProperty("compareKey");
		_comparePartition = _actionParams.getProperty("comparePartition");
		
		_compareHeaders = _actionParams.getProperty("compareHeaders");
		
		if (null != _compareHeaders) {
			ImaJson jo = new ImaJson();
			int ne = jo.parse(_compareHeaders);
			if (ne > 0) {
				_compareHeadersJson = jo;
			}
			else {
				_compareHeadersJson = null;
			}
		}
		else {
			_compareHeadersJson = null;
		}



	}

	protected boolean invokeApi() throws IsmTestException {
		
		ConsumerRecord<String, String> msg = (ConsumerRecord<String, String>) _dataRepository.getVar(_msgID);
		if (null != _compareTopic) {
			if (!_compareTopic.equals(msg.topic())) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
						, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
						, "Compare failure, "
						+ " msg:topic=" + msg.topic()
						+ " msg:partition=" + msg.partition()
						+ " msg:key=" + msg.key()
						+ " msg:string=" + msg.value());
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+11),
						"Received message on topic "+msg.topic()+", " + " expected "+_compareTopic);
			}
		}
		
		if (null != _compareValue) {
			if (!_compareValue.equals(msg.value())) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
						, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
						, "Compare failure, "
						+ " msg:topic=" + msg.topic()
						+ " msg:partition=" + msg.partition()
						+ " msg:key=" + msg.key()
						+ " msg:string=" + msg.value());
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+11),
						"Received message on value "+msg.value()+", " + " expected "+_compareValue);
			}
		}

		if (null != _compareHeadersJson) {
			Headers headers = msg.headers();
			if (null == headers) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
						, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
						, "Compare failure, "
						+ " msg:headers=" + msg.headers()
						+ " msg:topic=" + msg.topic()
						+ " msg:partition=" + msg.partition()
						+ " msg:key=" + msg.key()
						+ " msg:string=" + msg.value());
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+11),
						"Received message on headers "+msg.headers()+", " + " expected " + _compareHeaders);
			}
			else {
				for (int i = 0; i < _compareHeadersJson.getEntryCount(); i++) {
					if (null != _compareHeadersJson.getEntries()[i].name && null != _compareHeadersJson.getEntries()[i].value) {
						boolean failure = true;						
						Iterator<Header> iterator = headers.iterator();
						while (iterator.hasNext()) {
							Header next = iterator.next();
							if (next.key().equals(_compareHeadersJson.getEntries()[i].name)) {
								byte[] bytes = next.value();
								if (null != bytes) {
									String value = new String(bytes);
									if (value.equals(_compareHeadersJson.getEntries()[i].value)) {
										failure = false;
										break;
									}
								}
								continue;
							}
						}
						if (failure) {
							_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
									, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
									, "Compare failure, "
									+ " msg:headers=" + msg.headers()
									+ " msg:topic=" + msg.topic()
									+ " msg:partition=" + msg.partition()
									+ " msg:key=" + msg.key()
									+ " msg:string=" + msg.value());
							throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+11),
									"Received message on headers "+msg.headers()+", " + " expected " + _compareHeaders);

						}
					}
				}				
			}
		}
		
		if (null != _compareKey) {
			if ( (null == msg.key() && !_compareKey.isEmpty()) || null != msg.key() && !_compareKey.equals(msg.key())) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
						, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
						, "Compare failure, "
						+ " msg:topic=" + msg.topic()
						+ " msg:partition=" + msg.partition()
						+ " msg:key=" + msg.key()
						+ " msg:string=" + msg.value());
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+11),
						"Received message on key "+msg.key()+", " + " expected " + (_compareKey.isEmpty() ? "null(empty string in XML)" : _compareKey));
			}
		}
		
		if (null != _comparePartition) {
			if (Integer.parseInt(_comparePartition) != msg.partition()) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
						, "ISMTEST"+(Constants.COMPAREMESSAGEDATA+13)
						, "Compare failure, "
						+ " msg:topic=" + msg.topic()
						+ " msg:partition=" + msg.partition()
						+ " msg:key=" + msg.key()
						+ " msg:string=" + msg.value());
				throw new IsmTestException("ISMTEST"+(Constants.COMPAREMESSAGEDATA+11),
						"Received message on partition "+msg.partition()+", " + " expected "+_comparePartition);
			}
		}

		return true;
	}


}
