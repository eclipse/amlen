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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

import svt.common.Constants;
import svt.common.SVTLog;

/**
 * UDP socket client sample
 * 
 * 
 */
public class SVTUdpClient implements Constants, Runnable {

   static String[] input = null;

   /**
    * method that starts the UDP client primary thread
    * 
    * @param args
    *           message to send
    */
   public static void main(String[] args) {
      input = args;
      new SVTUdpClient().run();
   }

   /**
    * UDP client primary thread
    * 
    */
   public void run() {

      // if no arguments provided on command line then prompt user for message to send.
      String clientSendMsg = null;
      SVTLog log = new SVTLog();
      if ((input == null) || (input.length != 2)) {
         try {
            log.print("clientSendMsg:  ");
            BufferedReader keyboardReader = new BufferedReader(new InputStreamReader(System.in));
            clientSendMsg = keyboardReader.readLine();
         } catch (Throwable e) {
            e.printStackTrace();
            System.exit(1);
         }
      } else {
         clientSendMsg = input[1];
         log.println("clientSendMsg:  " + clientSendMsg);
      }

      try {
         // send message to server
         byte[] sendData = clientSendMsg.getBytes();
         InetAddress IPAddress = InetAddress.getByName("localhost");
         DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, port);

         DatagramSocket clientSocket = new DatagramSocket();
         clientSocket.send(sendPacket);

         // get message back from server
         byte[] receiveData = new byte[1024];
         DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
         clientSocket.receive(receivePacket);
         String clientReceiveMsg = new String(receivePacket.getData());
         log.println("clientReceiveMsg:" + clientReceiveMsg);

         clientSocket.close();
      } catch (IOException e) {
         System.err.println("I/O Exception: " + host + ":" + port);
         e.printStackTrace();
         System.exit(1);
      }
   }
}
