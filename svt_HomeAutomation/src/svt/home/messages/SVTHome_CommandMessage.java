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
package svt.home.messages;

import org.json.simple.JSONObject;

import svt.framework.messages.SVT_BaseEvent;
import svt.home.HomeConstants;

public class SVTHome_CommandMessage extends SVT_BaseEvent {
	long custId;
	long deviceId;
	String deviceType;
	String command;
	JSONObject msg;

	@SuppressWarnings("unchecked")
	public SVTHome_CommandMessage(String command, long custId, long deviceId,
			String deviceType) {
		this.custId = custId;
		this.deviceId = deviceId;
		this.deviceType = deviceType;
		this.requestTopic = HomeConstants.OfficeTopic;
		this.replyTopic = HomeConstants.BaseDeviceTopic + custId + "/" + deviceType + "/"
				+ deviceId+"/"+HomeConstants.EventTopicPostfix;
		msg = new JSONObject();
		msg.put("command", command);
		msg.put("custId", custId);
		msg.put("deviceId", deviceId);
		msg.put("deviceType", deviceType);
		msg.put("requestTopic", this.requestTopic);
		msg.put("replyTopic", this.replyTopic);
	}

	public String getMessageData() {
		message = msg.toJSONString();
		return message;
	}

	public long getCustId() {
		return custId;
	}

	public long getDeviceId() {
		return deviceId;
	}

	public String getDeviceType() {
		return deviceType;
	}

	public String getCommand() {
		return command;
	}

	@SuppressWarnings("unchecked")
	public void addData(String parameter, String value) {
		msg.put(parameter, value);
	}

}
