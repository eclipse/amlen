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
package svt.home.device;

import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

import svt.framework.device.SVT_Device;
import svt.home.HomeConstants;
import svt.home.messages.SVTHome_CommandMessage;

public class SVTHomeControl extends SVT_Device {
	private String currentState;

	public SVTHomeControl(long custId, long deviceID, String deviceType) {
		super(custId, deviceID, deviceType);

		// todo rethink constructor to include custId, deviceType and deviceId
		topic = HomeConstants.BaseDeviceTopic + custId + "/CONTROL/" + deviceType + "/"
				+ deviceId + "/" + "COMMAND";
		currentState = "off";
	}

	@Override
	public boolean processCommand(String topic, String command) {
		if (topic != this.topic) {
			return false;
		}
		JSONObject cmdObj = (JSONObject) JSONValue.parse(command);
		SVTHome_CommandMessage replyEvent;
		String replyTopic = (String) cmdObj.get("replyTopic");
		String replyMsg = new String();
		String cmd = (String) cmdObj.get("command");
		switch (cmd.toLowerCase()) {
		case "set":
			currentState = (String) cmdObj.get("value");
			replyMsg = "Control " + deviceId + " is now set to " + currentState;
			break;
		case "clear":
			currentState = "auto";
			replyMsg = "Control " + deviceId + " is now set to " + currentState;
			break;
		case "reset":
			currentState = "off";
			replyMsg = "Control " + deviceId + " is now set to " + currentState;
			break;
		case "down":
			switch (currentState.toLowerCase()) {
			case "low":
				currentState = "off";
				break;
			case "medium":
				currentState = "low";
				break;
			case "high":
				currentState = "medium";
				break;
			case "auto":
				currentState = "high";
				break;
			case "off":
				break;
			}
			replyMsg = "Control " + deviceId + " is now set to " + currentState;
			break;
		case "up":
			switch (currentState.toLowerCase()) {
			case "off":
				currentState = "low";
				break;
			case "low":
				currentState = "medium";
				break;
			case "medium":
				currentState = "high";
				break;
			case "high":
				currentState = "auto";
				break;
			case "auto":
				break;
			}
			replyMsg = "Control " + deviceId + " is now set to " + currentState;
			break;
		case "enable":
			status = "enabled";
			replyMsg = "Device " + deviceId + " is now enabled";
			break;
		case "disable":
			status = "disabled";
			replyMsg = "Device " + deviceId + " is now disabled";
			break;
		default:
			break;
		}
		replyEvent = new SVTHome_CommandMessage("reply", custId, deviceId,
				deviceType);
		replyEvent.message = replyMsg;
		replyEvent.requestTopic = replyTopic;
		replyEvent.replyTopic = null;
		transportThread.sendEvent(replyEvent);

		return true;
	}

	public static void main(String[] args) {

	}

	@Override
	public Object getCurrentValue() {
		String replyMsg = new String();
		replyMsg = "Device " + deviceId + " is " + status + "and has state "
				+ currentState;
		return replyMsg;
	}

}
