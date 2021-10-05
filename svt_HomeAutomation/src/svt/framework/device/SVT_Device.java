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
package svt.framework.device;

import svt.framework.scale.transports.SVT_Transport_Thread;
import svt.home.HomeConstants;
import svt.home.messages.SVTHome_CommandMessage;

public abstract class SVT_Device {
	protected SVT_Transport_Thread transportThread;
	protected long custId; 
	protected long deviceId; 
	protected String deviceType;
	protected String topic;
	protected String status; 
	
	
	public SVT_Device(long custId, long deviceId, String deviceType) {
		super();
		this.custId = custId;
		this.deviceId = deviceId;
		this.deviceType = deviceType;
		this.status = "enabled";
		topic = HomeConstants.BaseDeviceTopic + custId + "/" + deviceType + "/"
				+ deviceId+"/"+HomeConstants.EventTopicPostfix ;
	}

	public abstract boolean processCommand(String topic, String command);

	public abstract Object getCurrentValue();
	
	public void registerDevice() {
		// create a message that acts as a registration with the central hub.
		SVTHome_CommandMessage regEvnt = new SVTHome_CommandMessage("register",
				custId, deviceId, deviceType);
		regEvnt.requestTopic=HomeConstants.OfficeTopic;
		regEvnt.replyTopic="";
		if (!(transportThread.sendEvent(regEvnt))) {
			// could not register device, did not get response
		}
	}

	public void setThread(SVT_Transport_Thread thisThread) {
		transportThread=thisThread;
	}
	public String getTopic() {
		return topic;
	}
}
