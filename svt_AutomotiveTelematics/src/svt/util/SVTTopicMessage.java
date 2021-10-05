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

import java.util.Hashtable;

import org.eclipse.paho.client.mqttv3.MqttMessage;

public class SVTTopicMessage {
   private String topic;
   private MqttMessage message;
   private Hashtable<String, Object> properties = null;

   public SVTTopicMessage(String topic, MqttMessage message) {
      this.topic = topic;
      this.message = message;
   }

   public Object setProperty(String name, Long value) {
      if (properties == null)
       properties = new Hashtable<String, Object>();
      return properties.put(name, value);
   }

   public Object setProperty(String name, String value) {
      if (properties == null)
         properties = new Hashtable<String, Object>();
      return properties.put(name, value);
   }

   public Object getProperty(String name) {
      if (properties == null)
         properties = new Hashtable<String, Object>();
      return properties.get(name);
   }

   public String getTopic() {
      return this.topic;
   }

   public MqttMessage getMessage() {
      return message;
   }

   public String getMessageText() {
      return new String(message.getPayload());
   }

   public void setMessageText(byte[] payload) {
      message.setPayload(payload);
   }
}
