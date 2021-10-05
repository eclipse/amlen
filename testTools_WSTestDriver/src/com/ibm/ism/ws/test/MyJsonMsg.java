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

import com.ibm.ism.jsonmsg.JsonMessage;
import com.ibm.ism.mqtt.ImaJson;
import com.ibm.ism.ws.test.jsonmsg.*;

public class MyJsonMsg {
	private String	original;
	private String	action;
	private ImaJson	json;
	private JsonMessage jsonMessage;
	
	enum MessageType {
		Connect, Connected, Send, Ack, Subscribe, CloseSubscription, DestroySubscription,
		Ping, Pong, Close
	}

	public MyJsonMsg(String action, ImaJson json, String original) {
		this.action = action;
		this.json = json;
		this.original = original;
	}
	
	public MyJsonMsg(JsonMessage msg) {
		this.action = "Send";
		jsonMessage = msg;
		json = null;
		original = null;
	}
	
	public MyJsonMsg(String topic, String payload) {
		jsonMessage = new JsonMessage(topic, payload);
	}


	public String getOriginal() {
		return original;
	}


	public void setOriginal(String original) {
		this.original = original;
	}


	public String getAction() {
		return action;
	}


	public void setAction(String action) {
		this.action = action;
	}


	public ImaJson getJson() {
		return json;
	}


	public void setJson(ImaJson json) {
		this.json = json;
	}


	public JsonMessage getJsonMessage() {
		return jsonMessage;
	}


	public void setJsonMessage(JsonMessage jsonMessage) {
		this.jsonMessage = jsonMessage;
	}


	static MyJsonMsg parseJsonMsg(String original) throws IsmTestException{
		ImaJson json = new ImaJson();
		json.parse(original);
		String action = json.getString("Action");
		if (null == action) {
			throw new IsmTestException("ISMTEST"+Constants.MYJSONMSG
					, "JsonMessage Action is empty, msg is '"+original+"'");
		} 
		MessageType actionType = Enum.valueOf(MessageType.class,
				action);
		switch (actionType) {
		case Connect:
			return new JsonMessageConnect(action, json, original);
		case Connected:
			return new JsonMessageConnected(action, json, original);
		case Send:
			return new JsonMessageSend(action, json, original);
		case Ack:
			return new JsonMessageAck(action, json, original);
		case Subscribe:
			return new JsonMessageSubscribe(action, json, original);
		case CloseSubscription:
			return new JsonMessageCloseSubscription(action, json, original);
		case DestroySubscription:
			return new JsonMessageDestroySubscription(action, json, original);
		case Ping:
			return new JsonMessagePing(action, json, original);
		case Pong:
			return new JsonMessagePong(action, json, original);
		case Close:
			return new JsonMessageClose(action, json, original);
		default:
			throw new IsmTestException("ISMTEST"+Constants.MYJSONMSG+1
					, "Unknown Action '"+action+"' for JsonMessage");

		}
	}
}
