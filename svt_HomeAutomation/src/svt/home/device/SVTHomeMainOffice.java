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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

import svt.framework.device.SVT_Device;
import svt.framework.scale.transports.SVT_Transport_PahoThread;
import svt.home.messages.SVTHome_CommandMessage;

public class SVTHomeMainOffice extends SVT_Device {
	boolean stopFlag = false;
	SVT_Transport_PahoThread myThread;
	HashMap<Long, String> deviceIdandDeviceType;
	HashMap<Long, List<Long>> custIdandDeviceId;

	public SVTHomeMainOffice(long custId, long deviceID, String deviceType) {
		super(custId, deviceID, deviceType);
		stopFlag = false;
	}

	public void stopThread() {
		stopFlag = true;
	}

	@Override
	@SuppressWarnings("unchecked")
	public boolean processCommand(String topic, String command) {
		SVTHome_CommandMessage replyEvent;
		JSONObject cmdObj = (JSONObject) JSONValue.parse(command);
		Long deviceId = (Long) cmdObj.get("deviceId");
		Long custId = (Long) cmdObj.get("custId");
		String deviceType = (String) cmdObj.get("deviceType");
		String replyTopic = (String) cmdObj.get("replyTopic");
		String replyMsg = new String();
		String cmd = (String) cmdObj.get("Command");
		ArrayList<Long> deviceList;
		JSONObject replyObj; 
		// process command
		switch (cmd.toLowerCase()) {
		case "register":
			if (custIdandDeviceId.containsKey(custId)) {
				deviceList = (ArrayList<Long>) custIdandDeviceId.get(custId);
				if (deviceList.contains(deviceId) == false) {
					deviceList.add(deviceId);
					custIdandDeviceId.put(custId, deviceList);
				}
			} else {
				deviceList = new ArrayList<Long>();
				deviceList.add(deviceId);
				custIdandDeviceId.put(custId, deviceList);
			}
			if (deviceIdandDeviceType.containsKey(deviceId) == false) {
				deviceIdandDeviceType.put(deviceId, deviceType);
			}
			replyMsg = "device " + deviceId + " is now registered";
			break;
		case "list":
			// determine what type of list is being requested.
			if (( custId != null && custId !=0) && (deviceType != null && deviceType != "")) {
				// get list of device Ids for a given customer of a specific type
				deviceList = (ArrayList<Long>) custIdandDeviceId.get(custId);
				replyObj = new JSONObject();
				JSONArray newArray = new JSONArray();
				for (Iterator<Long> iterator = deviceList.iterator(); iterator
						.hasNext();) {
					Long thisDeviceId = (Long) iterator.next();
					if (deviceIdandDeviceType.get(thisDeviceId) == deviceType) {
						newArray.add(thisDeviceId);
					}
					replyObj.put(deviceType, newArray);
				}
				replyMsg = replyObj.toJSONString();
			} else if (deviceId != null && deviceId !=0) {
				// get list of customer ids for a given deviceId
			} else if (deviceType != null && deviceType != "") {
				// get list of customer Ids for a specific deviceType
			} else if (custId != null && custId !=0) {
				// get a list of device ids and device types for a specific customer id
				deviceList = (ArrayList<Long>) custIdandDeviceId.get(custId);
				replyObj = new JSONObject();
				for (Iterator<Long> iterator = deviceList.iterator(); iterator
						.hasNext();) {
					Long thisDeviceId = (Long) iterator.next();
					replyObj.put(thisDeviceId, deviceIdandDeviceType.get(thisDeviceId));
				}
				replyMsg = replyObj.toJSONString();
				// customer
				
			}
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

	@Override
	public Object getCurrentValue() {
		System.out.println("Main Office doesn't have a value");
		return null;
	}

	@Override
	public void registerDevice() {

	}

}

// public void begin() {
// myParms.appId = "MainOffice";
// myParms.threadId = 1L;
// myThread = new SVT_Transport_PahoThread(myParms);
// myThread.topicName = "/APP/HOME/DEVICE/OFFICE/*";
// myThread.addDevice(this);
// this.setThread(myThread);
// Thread t = new Thread(myThread);
//
// while (!stopFlag) {
// try {
// t.start();
// Thread.sleep(10L);
// while (t.isAlive()) {
// t.join(60000);
// }
//
// } catch (InterruptedException e) {
// SVTLog.logex(TTYPE.SCALE,
// "thread got interrupted in MainOffice ", e);
// stopFlag = false;
// }
// }
// }

