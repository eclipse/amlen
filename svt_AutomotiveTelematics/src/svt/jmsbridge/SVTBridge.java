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
package svt.jmsbridge;

public class SVTBridge {
   static Thread ism2mqthread;
   static Thread mq2ismthread;

   /**
    * @param args
    * @throws InterruptedException
    */
   public static void main(String[] args) {
      ism2mqthread = new Thread(new SVTISM2MQBridge());
      mq2ismthread = new Thread(new SVTMQ2ISMBridge());

      try {
         ism2mqthread.start();
         mq2ismthread.start();

         while (ism2mqthread.isAlive() && mq2ismthread.isAlive()) {
            Thread.sleep(2000);
         }
      } catch (Throwable e) {
         e.printStackTrace();
      }

   }

}
