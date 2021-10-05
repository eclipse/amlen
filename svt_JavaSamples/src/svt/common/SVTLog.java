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

package svt.common;

import java.io.FileOutputStream;
import java.io.PrintStream;

public class SVTLog {
   PrintStream out;

   public SVTLog() {
      this.out = System.out;
   }

   public SVTLog(PrintStream stream) {
      this.out = stream;
   }

   public SVTLog(String filename) {
      try {
         FileOutputStream fos = new FileOutputStream(filename);
         this.out = new PrintStream(fos);
      } catch (Throwable e) {
         this.out = System.out;
      }
   }

   /**
    * Common method for logging output
    * 
    * @param message
    *           message to be logged
    */
   public void print(String message) {
      System.out.print(message);
   }

   /**
    * Common method for logging output
    * 
    * @param message
    *           message to be logged
    */
   public void println(String message) {
      System.out.println(message);
   }

   /**
    * Common method to log an exception.
    * 
    * @param message
    *           Message to logged along with the exception
    * @param e
    *           The exception thrown
    */
   public void printException(String message, Throwable e) {
      println(message + e.getMessage());
      e.printStackTrace();
   }

}
