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

import java.util.Random;

import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

import svt.framework.device.SVT_Device;
import svt.home.messages.SVTHome_CommandMessage;

public class SVTHomeSensor extends SVT_Device {
	public SVTHomeSensor(long custId, long deviceID, String deviceType) {
		super(custId, deviceID, deviceType);
	}

	private int currentValue = 0;
	private java.util.Random randGen = new Random();
	private boolean init = true;
	long sensorId = 0;

	@Override
	public boolean processCommand(String topic, String commandString) {
		SVTHome_CommandMessage replyEvent;
		JSONObject cmdObj; 
		String replyTopic;
		String replyMsg = new String();
		String cmd;
		// process command
		Integer newVal;
		int setTo = 0;
		cmdObj= (JSONObject) JSONValue.parse(commandString);
		replyTopic = (String) cmdObj.get("replyTopic");
		cmd = (String) cmdObj.get("command");
		if (topic != this.topic) {
			return false;
		}
		switch (cmd.toLowerCase()) {
		case "set":
			newVal = (Integer) cmdObj.get("value");
			setTo = newVal.intValue();
			setCurrentValue(setTo);
			replyMsg = "Command " + cmd +" set value = "+setTo;
			break;
		case "clear":
			setTo = 0;
			setCurrentValue(setTo);
			replyMsg = "Command " + cmd +" set value = "+setTo;
			break;
		case "reset":
			setTo = randGen.nextInt(100);
			setCurrentValue(setTo);
			replyMsg = "Command " + cmd +" set value = "+setTo;
			break;
		case "down":
			newVal = (Integer) cmdObj.get("value");
			setTo = ((Integer) getCurrentValue()).intValue()
					- newVal.intValue();
			setCurrentValue(setTo);
			replyMsg = "Command " + cmd +" set value = "+setTo;
			break;
		case "up":
			newVal = (Integer) cmdObj.get("value");
			setTo = ((Integer) getCurrentValue()).intValue()
					+ newVal.intValue();
			setCurrentValue(setTo);
			replyMsg = "Command " + cmd +" set value = "+setTo;
			break;
		case "enable":
			status = "enabled";
			replyMsg = "Device "+deviceId+" is now enabled";
			break;
		case "disable":
			status = "disabled";
			replyMsg = "Device "+deviceId+" is now disabled";
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

	public Object getCurrentValue() {
		if (init) {
			setCurrentValue(randGen.nextInt(100));
			init = false;
		} else {
			if (randGen.nextBoolean()) {
				setCurrentValue(currentValue + randGen.nextInt(3));
			} else {
				setCurrentValue(currentValue - randGen.nextInt(3));
			}
		}

		return currentValue;
	}


	private void setCurrentValue(int currentValue) {
		this.currentValue = currentValue;
		System.out.println("Device ID " + deviceId+" current sensor reading is: "
				+ currentValue);
	}

	public static void main(String[] args) {
		SVTHomeSensor mySensor = new SVTHomeSensor(1L, 1L, "testSensor");
		for (int i = 0; i < 100; i++) {
			System.out.println("current sensor reading is: "
					+ mySensor.getCurrentValue());
		}
	}

}
