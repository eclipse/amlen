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

/**
 * Common constants for networking samples
 * 
 * 
 */
public interface Constants {

   /**
    * Constants for command line options
    */
   static String ACTION = "-a";
   static String COUNT = "-n";
   static String MESSAGE = "-m";
   static String SERVERURI = "-u";
   static String TOPICNAME = "-t";
   static String QOS = "-q";
   static String LOGFILE = "-o";
   static String TIMEOUT = "-x";
   static String CLIENT = "-c";
   static String DATASTORE = "-d";
   
   /**
    * Networking constants
    */
   static String host = "127.0.0.1";
   static int port = 4444;
   static String mgroup = "228.5.6.7";
   
   /**
    * Logging constants
    */
   static String SUCCESS = "Test Success!";
   static final int  MILLISECONDSPERSECOND = 1000;


}
