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
package com.ibm.ism.ws.test.jsonmsg;

import com.ibm.ism.jsonmsg.JsonMessage;
import com.ibm.ism.mqtt.ImaJson;
import com.ibm.ism.ws.test.MyJsonMsg;

public class JsonMessageSend extends MyJsonMsg {
	private String	Body;
	private int		ID;
	private String	Name;
	private	boolean	Persist;
	private Object	Properties;
	private int		QoS;
	private String	Queue;
	private int		Retain;
	private String	Shared;
	private String	Topic;
	private String	Type;

	public JsonMessageSend(String action, ImaJson json, String original) {
		super(action, json, original);
		Body = json.getString("Body");
		ID = json.getInt("ID", -1);
		Name = json.getString("Name");
		Persist = json.getBoolean("Persist", false);
		QoS = json.getInt("QoS", -1);
		Queue = json.getString("Queue");
		Retain = json.getInt("Retain", -1);
		Shared = json.getString("Shared");
		Topic = json.getString("Topic");
		Type = json.getString("Type");
		// Properties?
	}

	public String getBody() {
		return Body;
	}

	public void setBody(String body) {
		Body = body;
		if (null != this.getJsonMessage()) {
			((JsonMessage)this.getJsonMessage()).setBody("");
		}
	}

	public int getID() {
		return ID;
	}

	public void setID(int iD) {
		ID = iD;
	}

	public String getName() {
		return Name;
	}

	public void setName(String name) {
		Name = name;
	}

	public boolean isPersist() {
		return Persist;
	}

	public void setPersist(boolean persist) {
		Persist = persist;
	}

	public Object getProperties() {
		return Properties;
	}

	public void setProperties(Object properties) {
		Properties = properties;
	}

	public int getQoS() {
		return QoS;
	}

	public void setQoS(int qoS) {
		QoS = qoS;
	}

	public String getQueue() {
		return Queue;
	}

	public void setQueue(String queue) {
		Queue = queue;
	}

	public int getRetain() {
		return Retain;
	}

	public void setRetain(int retain) {
		Retain = retain;
	}

	public String getShared() {
		return Shared;
	}

	public void setShared(String shared) {
		Shared = shared;
	}

	public String getTopic() {
		return Topic;
	}

	public void setTopic(String topic) {
		Topic = topic;
	}

	public String getType() {
		return Type;
	}

	public void setType(String type) {
		Type = type;
	}


}
