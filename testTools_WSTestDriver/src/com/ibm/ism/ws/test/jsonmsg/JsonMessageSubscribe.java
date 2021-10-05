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

import com.ibm.ism.mqtt.ImaJson;
import com.ibm.ism.ws.test.MyJsonMsg;

public class JsonMessageSubscribe extends MyJsonMsg {
	private String	ID;
	private String	Name;
	private String	Queue;
	private String	Topic;
	private String	Shared;
	private int		QoS;
	private String	Type;	

	public JsonMessageSubscribe(String action, ImaJson json, String original) {
		super(action, json, original);
		ID		= json.getString("ID");
		Name	= json.getString("Name");
		Queue	= json.getString("Queue");
		Topic	= json.getString("Topic");
		Shared	= json.getString("Shared");
		QoS		= json.getInt("QoS", -1);
		Type	= json.getString("Type");
	}

	public String getID() {
		return ID;
	}

	public void setID(String iD) {
		ID = iD;
	}

	public String getName() {
		return Name;
	}

	public void setName(String name) {
		Name = name;
	}

	public String getQueue() {
		return Queue;
	}

	public void setQueue(String queue) {
		Queue = queue;
	}

	public String getTopic() {
		return Topic;
	}

	public void setTopic(String topic) {
		Topic = topic;
	}

	public String getShared() {
		return Shared;
	}

	public void setShared(String shared) {
		Shared = shared;
	}

	public int getQoS() {
		return QoS;
	}

	public void setQoS(int qoS) {
		QoS = qoS;
	}

	public String getType() {
		return Type;
	}

	public void setType(String type) {
		Type = type;
	}


}
