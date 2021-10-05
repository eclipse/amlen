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
package com.ibm.ism.test.mqtt;

import com.ibm.ism.mqtt.IsmMqttListener;
import com.ibm.ism.mqtt.IsmMqttMessage;

public class MqttTestListener implements IsmMqttListener {
	
	private int numMessagesRcvd = 0;

	public void onDisconnect(int rc, Throwable e) {
		System.out.println("Disconnected from ISM Server. rc=" + rc);
		if (e != null) { 
			e.printStackTrace();
		}
		
	}

	public void onMessage(IsmMqttMessage msg) {
		//System.out.println(msg.getTopic() + ": " + msg.getPayload());
		numMessagesRcvd++;
	}
	
	public int getNumReceived() {
		return numMessagesRcvd;
	}

}
