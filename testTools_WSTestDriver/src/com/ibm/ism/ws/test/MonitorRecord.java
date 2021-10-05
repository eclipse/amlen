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

import java.text.ParseException;
import java.util.Date;


public abstract class MonitorRecord {
	enum ObjectType {
		Server,
        Endpoint,
        Connection,
        Topic,
        Queue,
        DestinationMappingRule,
        Memory,
        Store
	}

	public ObjectType	type;
	public Date			timeStamp;
	public String		nodeName;
	public String		version;

	MonitorRecord(ImaJson json) throws IsmTestException {
		setMessageType(json.getString("ObjectType"));
		String tStamp = json.getString("TimeStamp");
		setVersion(json.getString("Version"));
		try {
			timeStamp = TestUtils.parse8601(tStamp);
		} catch (ParseException pe) {
			throw new IsmTestException("ISMTEST"+(Constants.MONITORRECORD+1),
				"Parse Exception thrown trying to parse timeStamp '"+timeStamp+"'", pe);
		}
	}
	
	public String getVersion() {
		return version;
	}

	public void setVersion(String version) {
		this.version = version;
	}
	
	public ObjectType getMessageType() {
		return type;
	}

	public static String getMessageTypeString(ObjectType type) {
		switch (type) {
		case Server: return "Server";
		case Endpoint: return "Endpoint";
		case Connection: return "Connection";
		case Topic: return "Topic";
		case Queue: return "Queue";
		case DestinationMappingRule: return "DestinationMappingRule";
		case Memory: return "Memory";
		case Store: return "Store";
		default: return "Unknown";
		}
	}

	public static int getMessageTypeInt(ObjectType type) {
		switch (type) {
		case Server: return 0;
		case Endpoint: return 1;
		case Connection: return 2;
		case Topic: return 3;
		case Queue: return 4;
		case DestinationMappingRule: return 5;
		case Memory: return 6;
		case Store: return 7;
		default: return -1;
		}
	}
	
	public int getMessageTypeInt() {
		return getMessageTypeInt(type);
	}

	public Date getTimeStamp() {
		return timeStamp;
	}
	
	public String getNodeName() {
		return nodeName;
	}
	
	public void setMessageType(String messageType) {
		this.type = ObjectType.valueOf(messageType);
	}
	
	public void setMessageType(ObjectType type) {
		this.type = type;
	}
	
	public String getMessageTypeString() {
		return getMessageTypeString(this.type);
	}
	
	public static ObjectType getMessageType(String messageType) {
		return ObjectType.valueOf(messageType);
	}
	
	public void setNodeName(String nodeName) {
		this.nodeName = nodeName;
	}
	
	public boolean validateValues(ObjectType type, String[] values) throws IsmTestException {
		for (int i=0; i<values.length; i++) {
			String[] parts = values[i].split(",");
			if (3 != parts.length) 
				throw new IsmTestException("ISMTEST"+(Constants.MONITORRECORD+2),
						"Value check should have 3 comma separated sections; was '"+values[i]+"'");
			validateValue(parts[0], parts[1], parts[2]);
		}
		return false;
	}
	
	public boolean validateValue(String name, String op, String value) throws IsmTestException {
		throw new IsmTestException("ISMTEST"+(Constants.MONITORRECORD+3),
						"validateValue is not supported for record type "+this.getMessageTypeString());	}
}
