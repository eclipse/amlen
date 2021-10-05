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
import java.net.ServerSocket;
import java.net.Socket;

import svt.common.Constants;
import svt.common.SVTLog;

/**
 * TCP Server sample application that receives a single message from the client and retransmits the
 * message back to the client.
 * 
 * 
 */
public class SVTTcpServer implements Constants, Runnable {

   /**
    * Method that starts the TCP Server primary thread
    * 
    * @param args
    */
   public static void main(String[] args) {
      new SVTTcpServer().run();
   }

   /**
    * TCP Server primary thread
    */
   public void run() {
      ServerSocket serverSocket = null;
      SVTLog log = new SVTLog();
      
      try {
         serverSocket = new ServerSocket(port);
      } catch (IOException e) {
         System.err.println("ServerSocket(" + port + ") I/O Exception: " + e.getMessage());
         System.exit(1);
      }

      Socket socket = null;
      try {
         socket = serverSocket.accept();
      } catch (IOException e) {
         System.err.println("ServerSocket.accept() I/O Exception:  " + e.getMessage());
         System.exit(1);
      }

      PrintWriter socketWriter;
      try {
         socketWriter = new PrintWriter(socket.getOutputStream(), true);
         BufferedReader socketReader = new BufferedReader(new InputStreamReader(socket.getInputStream()));

         String receivedMsg = null;

         // receive message from client
         receivedMsg = socketReader.readLine();
         log.println("receivedMsg:  " + receivedMsg);

         // send message back to client
         socketWriter.println(receivedMsg);

         socketReader.close();
         socketWriter.close();
      } catch (IOException e) {
         e.printStackTrace();
         System.exit(1);
      }

      try {
         socket.close();
         serverSocket.close();
      } catch (IOException e) {
         e.printStackTrace();
         System.exit(1);
      }
   }

}
