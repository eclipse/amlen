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

package svt.socket.udp;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

import svt.common.Constants;
import svt.common.SVTLog;

/**
 * UDP Server Sample
 * 
 * 
 */
public class SVTUdpServer implements Constants, Runnable {

   /**
    * Method that starts the UDP Server primary thread
    * 
    * @param args
    */
   public static void main(String[] args) {
      new SVTUdpServer().run();
   }

   /**
    * UDP Server primary thread that receives datagram packets and retransmits each back to the
    * client.
    * 
    */
   public void run() {
      SVTLog log = new SVTLog();
      try {
         // receive message from client
         DatagramSocket serverSocket = new DatagramSocket(port);
         byte[] receiveData = new byte[1024];
         DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
         serverSocket.receive(receivePacket);
         String receivedMsg = new String(receivePacket.getData());
         log.println("receivedMsg: " + receivedMsg);

         // send same message back to client
         byte[] sendData = receivedMsg.getBytes();
         InetAddress IPAddress = receivePacket.getAddress();
         int port = receivePacket.getPort();
         DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, port);
         serverSocket.send(sendPacket);
         serverSocket.close();
      } catch (Throwable e) {
         e.printStackTrace();
         System.exit(1);
      }
   }
}
