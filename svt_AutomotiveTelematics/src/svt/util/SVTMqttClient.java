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

import java.util.ArrayList;

import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;

public class SVTMqttClient {
   public MqttClient client;
   public MqttConnectOptions options;
   public MqttCallback callback;
   ArrayList<SVTTopicMessage> stack = null;

   public SVTMqttClient(MqttClient client, MqttConnectOptions options, MqttCallback callback, ArrayList<SVTTopicMessage> stack) {
      this.client = client;
      this.options = options;
      this.callback = callback;
      this.stack = stack;
   }

   public int stacksize() {
      int size = 0;
      synchronized (stack) {
         size = stack.size();
      }
      return size;
   }

   public SVTTopicMessage pop() {
      SVTTopicMessage entry = null;
      synchronized (stack) {
         if (stack.size() > 0)
            entry = stack.remove(0);
      }
      return entry;
   }

};
