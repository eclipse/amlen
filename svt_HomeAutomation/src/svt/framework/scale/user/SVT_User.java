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
package svt.framework.scale.user;

import svt.mqtt.mq.MqttSample;

public class SVT_User implements Runnable {
   static Thread ism2mqthread;
   static Thread mq2ismthread;
   static int i = 0;

   /**
    * @param args
    * @throws InterruptedException
    */
   public static void main(String[] args) {
      for (i = 1; i < 100; i++) {
         new Thread(new SVT_User()).start();
         try {
            Thread.sleep(1);
         } catch (Throwable e) {
            e.printStackTrace();
         }
      }
   }

   public void run() {
      String[] args = { "-v", "-s", "tcp://9.3.179.59:16102", "-a", "publish", "-t", "BRIDGE2ISM", "-n", "1000000000", "-w", "1000" };
      MqttSample sample = new MqttSample(args);
      sample.run();
   }

}
