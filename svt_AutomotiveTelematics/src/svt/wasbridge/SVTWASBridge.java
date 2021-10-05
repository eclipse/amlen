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
package svt.wasbridge;

public class SVTWASBridge {
   static Thread ism2wasthread;
   static Thread was2ismthread;

   /**
    * @param args
    * @throws InterruptedException
    */
   public static void main(String[] args) {
      ism2wasthread = new Thread(new SVTISM2WASBridge());
      was2ismthread = new Thread(new SVTWAS2ISMBridge());

      try {
         ism2wasthread.start();
         was2ismthread.start();

         while (ism2wasthread.isAlive() && was2ismthread.isAlive()) {
            Thread.sleep(2000);
         }
      } catch (Throwable e) {
         e.printStackTrace();
      }

   }

}
