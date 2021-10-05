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
package svt.util;

import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import svt.util.SVTLog.TTYPE;


public class SVTMQTTMessage implements SVTMessage {
    TTYPE TYPE = TTYPE.MQTT;

    MqttTopic topic;
    MqttMessage mqttmessage;

    public SVTMQTTMessage(MqttTopic topic, MqttMessage message) {
        this.topic = topic;
        this.mqttmessage = message;
    }

    public String getTopicName() {
        String name = null;
        try {
            name = topic.getName();
        } catch (Throwable e) {
            name = e.getMessage();
            SVTLog.logex(TYPE, "getName", e);
        }

        return name;
    }

    public String[] getTopicTokens() {
        String[] token;

        String delims = "[/]+";
        token = getTopicName().split(delims);

        return token;
    }

    public String getMessageText() {
        String text;
        // try {
        text = new String(mqttmessage.getPayload());
        // } catch (MqttException e) {
        // text = e.getMessage();
        // SVTLog.logex(TYPE, "getPayload", e);
        // }
        return text;
    }

}
