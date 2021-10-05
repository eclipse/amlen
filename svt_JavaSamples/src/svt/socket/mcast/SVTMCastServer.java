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

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.MulticastSocket;

import svt.common.Constants;
import svt.common.SVTLog;

/**
 * Multicast server class that receives a message from a client and resends the same message back to
 * the client.
 * 
 * 
 */
public class SVTMCastServer implements Constants, Runnable {

   /**
    * Method that starts multicast server thread
    * 
    * @param args
    */
   public static void main(String[] args) {
      new SVTMCastServer().run();
   }

   /**
    * Primary method for the multicast server
    * 
    */
   public void run() {
      SVTLog log = new SVTLog();
      try {
         byte[] buf = new byte[1024];
         InetAddress group = InetAddress.getByName(mgroup);
         MulticastSocket s = new MulticastSocket(port);

         // Join the mcast group
         s.joinGroup(group);

         // receive message from client
         DatagramPacket recv = new DatagramPacket(buf, buf.length);
         s.receive(recv);
         String msg = recv.getData().toString();
         log.println("server received:  " + msg);

         // send message back to client
         DatagramPacket resp = new DatagramPacket(msg.getBytes(), msg.length(), group, port);
         log.println("server:  s.send(resp)");
         s.send(resp);

         // Leave the mcast group
         s.leaveGroup(group);
      } catch (IOException e) {
         log.printException("Exception caught in multicast server.", e);
      }
   }
}
