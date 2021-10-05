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

package svt.socket.tcp;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.UnknownHostException;

import svt.common.Constants;
import svt.common.SVTLog;

/**
 * Client that demonstrates tcp sockets
 * 
 * 
 */
public class SVTTcpClient implements Constants, Runnable {

   static String[] input = null;

   /**
    * method that starts tcp client's primary thread
    * 
    * @param args
    */
   public static void main(String[] args) {
      input = args;
      new SVTTcpClient().run();
   }

   /**
    * tcp client's primary thread
    * 
    * 
    */
   public void run() {

      SVTLog log = new SVTLog();
      
      String clientSendMsg = null;
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

      Socket socket = null;
      PrintWriter socketWriter = null;
      BufferedReader socketReader = null;

      try {
         socket = new Socket(host, port);
         socketWriter = new PrintWriter(socket.getOutputStream(), true);
         socketReader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
      } catch (UnknownHostException e) {
         System.err.println("Unknown Host Exception: " + host + ":" + port);
         e.printStackTrace();
         System.exit(1);
      } catch (IOException e) {
         System.err.println("I/O Exception: " + host + ":" + port);
         e.printStackTrace();
         System.exit(1);
      }

      socketWriter.println(clientSendMsg);

      String serverReplyMsg = null;
      try {
         serverReplyMsg = socketReader.readLine();
         log.println("serverReplyMsg: " + serverReplyMsg);
      } catch (Throwable e) {
         e.printStackTrace();
         System.exit(1);
      }

      try {
         socketWriter.close();
         socketReader.close();
         socket.close();
      } catch (Throwable e) {
         e.printStackTrace();
         System.exit(1);
      }
   }
}
