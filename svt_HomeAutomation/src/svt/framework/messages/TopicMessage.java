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
package svt.framework.messages;

import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

public class TopicMessage {
	private String topic;
	private MqttMessage message;

	public TopicMessage(String s, MqttMessage mqttmessage) {
		this.topic = s;
		this.message = mqttmessage;
	}

	public String getTopic() {
		return this.topic;
	}

	public MqttMessage getMessage() throws MqttException {
		return message;
	}

	// public String getTopicName() {
	// return topic;
	// }

	public String getMessageText() throws MqttException {
		return new String(message.getPayload());
	}
}
