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
package com.ibm.ism.ws.test;

import java.util.HashMap;
import java.util.Map;

public class MyMonitorRecord {
	private MonitorRecord monitor = null;
	@SuppressWarnings("unused")
	private static ServerRecord		previousServerRecord;
	private static ServerRecord		currentServerRecord;
	@SuppressWarnings("unused")
	private static MemoryRecord		previousMemoryRecord;
	private static MemoryRecord		currentMemoryRecord;
	@SuppressWarnings("unused")
	private static StoreRecord		previousStoreRecord;
	private static StoreRecord		currentStoreRecord;
	private static Map<String, EndpointRecord>	previousEndpointRecords = new HashMap<String, EndpointRecord>();
	private static Map<String, EndpointRecord>	currentEndpointRecords = new HashMap<String, EndpointRecord>();
	private static Map<String, TopicRecord>		previousTopicRecords = new HashMap<String, TopicRecord>();
	private static Map<String, TopicRecord>		currentTopicRecords = new HashMap<String, TopicRecord>();

	MyMonitorRecord(MyMessage msg) throws IsmTestException {
		
		ImaJson json = new ImaJson();
		int entries = json.parse(msg.getPayload());
		if (entries < 0) {
			throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD+1),
				"json.parse() returned "+entries+" for '"+msg.getPayload()+"'");
		}
		if (0 < json.left) {
			json.skipWhite();
			if (0 < json.left) {
				throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD+2),
						"json includes extraneous characters '"+msg.getPayload()+"'");
			}
		}
		String messageType = json.getString("ObjectType");
		switch (MonitorRecord.ObjectType.valueOf(messageType)) {
		case Server:
			monitor = new ServerRecord(json);
			previousServerRecord = currentServerRecord;
			currentServerRecord = (ServerRecord)monitor;
			break;
		case Memory:
			monitor = new MemoryRecord(json);
			previousMemoryRecord = currentMemoryRecord;
			currentMemoryRecord = (MemoryRecord)monitor;
			break;
		case Store:
			monitor = new StoreRecord(json);
			previousStoreRecord = currentStoreRecord;
			currentStoreRecord = (StoreRecord)monitor;
			break;
		case Endpoint:
			EndpointRecord currentEndpoint = new EndpointRecord(json);
			monitor = currentEndpoint;
			EndpointRecord previousEndpoint = currentEndpointRecords.get(currentEndpoint.name);
			if (null != previousEndpoint) {
				validateEndpointUpdate(previousEndpoint, currentEndpoint);
				previousEndpointRecords.put(currentEndpoint.name, previousEndpoint);
			}
			currentEndpointRecords.put(currentEndpoint.name, currentEndpoint);
			break;
		case Topic:
			TopicRecord currentTopic = new TopicRecord(json);
			monitor = currentTopic;
			TopicRecord previousTopic = currentTopicRecords.get(currentTopic.topicString);
			if (null != previousTopic) {
				validateTopicUpdate(previousTopic, currentTopic);
				previousTopicRecords.put(currentTopic.topicString, previousTopic);
			}
			currentTopicRecords.put(currentTopic.topicString, currentTopic);
			break;
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD+3),
					"Unknown monitor message type '"+messageType+"'");
		}
		if (!validateTopic(msg.getTopic(), messageType)) {
			throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD+4),
					"Invalid topic '"+msg.getTopic()+"' for monitor record type "+messageType);
		}

	}
	
	MonitorRecord getMonitorRecord() {
		return monitor;
	}
	
	boolean validateTopic(String topic, String type) throws IsmTestException {
		String[] levels = topic.split("/");
		if (levels.length != 4 && levels.length != 3) {
			return false;
		}
		if (!"$SYS".equals(levels[0])) {
			return false;
		}
		if (!"ResourceStatistics".equals(levels[1]) && !"Server".equals(levels[1])) {
			return false;
		}
		if (!type.equals(levels[levels.length-1])) {
			return false;
		}
		return true;
	}

	void validateEndpointUpdate(EndpointRecord previous, 
			EndpointRecord current) throws IsmTestException {
		if (current.resetTime == previous.resetTime) {
			if (current.bytesRead < previous.bytesRead)
				throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD)
						,"New EndpointRecord has lower byteRead: "+current.bytesRead
						+" than previous record: "+previous.bytesRead);
			if (current.bytesWrite < previous.bytesWrite)
				throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD)
						,"New EndpointRecord has lower bytesWrite: "+current.bytesWrite
						+" than previous record: "+previous.bytesWrite);
			if (current.msgRead < previous.msgRead)
				throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD)
						,"New EndpointRecord has lower msgRead: "+current.msgRead
						+" than previous record: "+previous.msgRead);
			if (current.msgWrite < previous.msgWrite)
				throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD)
						,"New EndpointRecord has lower msgWrite: "+current.msgWrite
						+" than previous record: "+previous.msgWrite);
		}
	}

	void validateTopicUpdate(TopicRecord previous, 
			TopicRecord current) throws IsmTestException {
		if (current.resetTime == previous.resetTime) {
			if (current.failedPublishes < previous.failedPublishes)
				throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD)
						,"New TopicRecord has lower failedPublishes: "
						+current.failedPublishes+" than previous record: "
						+previous.failedPublishes);
			if (current.publishedMsgs < previous.publishedMsgs)
				throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD)
						,"New TopicRecord has lower publishedMsgs: "
						+current.publishedMsgs+" than previous record: "
						+previous.publishedMsgs);
			if (current.rejectedMsgs < previous.rejectedMsgs)
				throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD)
						,"New TopicRecord has lower rejectedMsgs: "
						+current.rejectedMsgs+" than previous record: "
						+previous.rejectedMsgs);
		}
	}

	void validateServerUpdate(ServerRecord previous, 
			ServerRecord current) throws IsmTestException {
		if (current.bytesRead < previous.bytesRead)
			throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD)
					,"New ServerRecord has lower byteRead: "+current.bytesRead
					+" than previous record: "+previous.bytesRead);
		if (current.bytesWrite < previous.bytesWrite)
			throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD)
					,"New ServerRecord has lower bytesWrite: "+current.bytesWrite
					+" than previous record: "+previous.bytesWrite);
		if (current.msgRead < previous.msgRead)
			throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD)
					,"New ServerRecord has lower msgRead: "+current.msgRead
					+" than previous record: "+previous.msgRead);
		if (current.msgWrite < previous.msgWrite)
			throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD)
					,"New ServerRecord has lower msgWrite: "+current.msgWrite
					+" than previous record: "+previous.msgWrite);
		if (current.badConnCount < previous.badConnCount)
			throw new IsmTestException("ISMTEST"+(Constants.MYMONITORRECORD)
					,"New ServerRecord has lower badConnCount: "+current.badConnCount
					+" than previous record: "+previous.badConnCount);
	}
}
