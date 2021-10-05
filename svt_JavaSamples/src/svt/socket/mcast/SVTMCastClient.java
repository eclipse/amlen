/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

package svt.socket.mcast;

import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.MulticastSocket;

import svt.common.Constants;
import svt.common.SVTLog;

/**
 * The Class SVTMCastClient.
 * 
 */
public class SVTMCastClient implements Constants, Runnable {

   static String[] input = null;

   /**
    * The main method.
    * 
    * @param args
    *           the arguments
    */
   public static void main(String[] args) {
      input = args;
      new SVTMCastClient().run();
   }

   /**
    * multicast client Primary thread that sends a message and receives a reply.
    * 
    */
   public void run() {
      SVTLog log = new SVTLog();
      try {
         String msg = "Hello";
         InetAddress group = InetAddress.getByName(mgroup);
         MulticastSocket s = new MulticastSocket(port);

         // join multicast group
         s.joinGroup(group);

         // send a message to the server
         DatagramPacket hi = new DatagramPacket(msg.getBytes(), msg.length(), group, port);
         log.println("send message: " + hi);
         s.send(hi);

         // receive the reply message
         byte[] buf = new byte[1024];
         DatagramPacket recv = new DatagramPacket(buf, buf.length);
         s.receive(recv);
         String resp = recv.getData().toString();
         log.println("reply message: " + resp);

         // leave multicast group
         s.leaveGroup(group);

      } catch (Throwable e) {
         log.printException("Exception caught in multicast client", e);
      }
   }

}
