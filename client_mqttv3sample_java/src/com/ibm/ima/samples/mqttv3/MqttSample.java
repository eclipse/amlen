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
package com.ibm.ima.samples.mqttv3;

import java.io.FileOutputStream;
import java.io.PrintStream;
import java.sql.Timestamp;
import java.util.Random;

import com.ibm.micro.client.mqttv3.MqttClient;

/**
 * The purpose of this sample is to demonstrate a basic example of how to use MQ's native MQTTv3 classes to publish and
 * subscribe to a IBM Messaging Appliance topic. This sample consists of three classes: 
 * MqttSample.java, MqttSamplePublish.java and MqttSampleSubscribe.java
 * 
 * MqttSample contains methods for parsing and validating arguments, logging messages, and other utility type functions.
 * 
 * MqttSampleSubscribe contains methods specific to subscribing to messages published to an IMA topic.
 * 
 * MqttSamplePublish contains methods specific to publishing messages to an IMA topic.
 * 
 * When publish is specified on the command line as the action argument, a message is repeatedly published to the topic
 * a specified number of times.
 * 
 * Similarly when subscribe is specified on the command line as the action argument, the sample will subscribe to the
 * topic and wait for the specified number of messages to be received.
 * 
 * See logged messages for verification of sent and received messages.
 * 
 * NOTE: This class implements Runnable to support execution in a separate thread. However, a separate thread is not
 * required and the main function does not use one for execution.
 * 
 * Command line options:
 * 
 * -i clientID Optional parameter specifying the client id used for registering the client with the IBM Messaging Appliance
 * server.
 * 
 * -s serverURI The URI address of the IMA server. This is in the format of tcp://<ip address>:<port> This is a required
 * parameter.
 * 
 * -a action Either the String: publish or subscribe. This is a required parameter.
 * 
 * -t topicName The name of the topic on which the messages are published. The default topic name is /MQTTv3Sample
 * 
 * -m message A String representing the message to be sent. The default message is JMS Sample Message
 * 
 * -n count The number of times the specified message is to be sent. The default number of message sent is 1.
 * 
 * -q qos The Quality of Service level 0, 1, or 2 The default QOS is 0.
 * 
 * -c cleansession true or false value indicating if server side session data should be removed when client disconnects.
 * 
 * -o Log filename The log defaults to stdout.
 * 
 * -r Enable persistence and specify datastore directory The default persistence is false.
 * 
 * -u client user id for secure communications. Optional parameter.
 * 
 * -p client password for secure communications. Optional parameter.
 * 
 * -w rate at which messages are sent in units of messages/second.
 * 
 * -v Indicates verbose output
 * 
 * -h Output usage statement
 * 
 * Additionally, per MQ's MQTT documentation, valid SSL system properties are:
 * 
 * com.ibm.ssl.protocol One of: SSL, SSLv3, TLS, TLSv1, SSL_TLS. com.ibm.ssl.contextProvider Underlying JSSE provider.
 * For example "IBMJSSE2" or "SunJSSE" com.ibm.ssl.keyStore The name of the file that contains the KeyStore object that
 * you want the KeyManager to use. For example /mydir/etc/key.p12 com.ibm.ssl.keyStorePassword The password for the
 * KeyStore object that you want the KeyManager to use. The password can either be in plain-text, or may be obfuscated
 * using the static method: com.ibm.micro.security.Password.obfuscate(char[] password). This obfuscates the password
 * using a simple and insecure XOR and Base64 encoding mechanism. Note that this is only a simple scrambler to obfuscate
 * clear-text passwords. com.ibm.ssl.keyStoreType Type of key store, for example "PKCS12", "JKS", or "JCEKS".
 * com.ibm.ssl.keyStoreProvider Key store provider, for example "IBMJCE" or "IBMJCEFIPS". com.ibm.ssl.trustStore The
 * name of the file that contains the KeyStore object that you want the TrustManager to use.
 * com.ibm.ssl.trustStorePassword The password for the TrustStore object that you want the TrustManager to use. The
 * password can either be in plain-text, or may be obfuscated using the static method:
 * com.ibm.micro.security.Password.obfuscate(char[] password). This obfuscates the password using a simple and insecure
 * XOR and Base64 encoding mechanism. Note that this is only a simple scrambler to obfuscate clear-text passwords.
 * com.ibm.ssl.trustStoreType The type of KeyStore object that you want the default TrustManager to use. Same possible
 * values as "keyStoreType". com.ibm.ssl.trustStoreProvider Trust store provider, for example "IBMJCE" or "IBMJCEFIPS".
 * com.ibm.ssl.enabledCipherSuites A list of which ciphers are enabled. Values are dependent on the provider, for
 * example: SSL_RSA_WITH_AES_128_CBC_SHA;SSL_RSA_WITH_3DES_EDE_CBC_SHA. com.ibm.ssl.keyManager Sets the algorithm that
 * will be used to instantiate a KeyManagerFactory object instead of using the default algorithm available in the
 * platform. Example values: "IbmX509" or "IBMJ9X509". com.ibm.ssl.trustManager Sets the algorithm that will be used to
 * instantiate a TrustManagerFactory object instead of using the default algorithm available in the platform. Example
 * values: "PKIX" or "IBMJ9X509".
 * 
 */

public class MqttSample implements Runnable {

   final static int AT_MOST_ONCE = 0;
   final static int AT_LEAST_ONCE = 1;
   final static int EXACTLY_ONCE = 2;

   /**
    * The main method executes the client's run method.
    * 
    * @param args
    *           Commandline arguments. See usage statement.
    */
   public static void main(String[] args) {

      /*
       * Instantiate and run the client.
       */
      new MqttSample(args).run();

   }

   public String action = null;
   public String clientId = null;
   public String userName = null;
   public String password = null;
   public int throttleWaitMSec = 0;
   public int count = 1;
   public String dataStoreDir = null;
   public String payload = "IMA WebSocket Sample Message";
   public boolean persistence = false;
   public int qos = AT_MOST_ONCE;
   public boolean cleanSession = true;
   public String serverURI = null;
   public String topicName = "/MQTTv3Sample";
   public boolean ssl = false;
   public boolean verbose = false;

   /**
    * Instantiates a new MQTT client.
    * 
    * 
    */
   public MqttSample(String[] args) {
      parseArgs(args);
   }

   /**
    * Parses the command line arguments passed into main().
    * 
    * @param args
    *           the command line arguments. See usage statement.
    */
   private void parseArgs(String[] args) {
      boolean showUsage = false;
      String comment = null;

      for (int i = 0; i < args.length; i++) {
         if ("-s".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            serverURI = args[i];
            if (serverURI.startsWith("tcp://")) {
               ssl = false;
            } else if (serverURI.startsWith("ssl://")) {
               ssl = true;
            } else {
               showUsage = true;
               comment = "Invalid Parameter:  " + args[i] + ".  serverURI must begin with either tcp:// or ssl://.";
               break;
            }
         } else if ("-a".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            action = args[i];
         } else if ("-i".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            clientId = args[i];
            if (clientId.length() > 23) {
               showUsage = true;
               comment = "Invalid Parameter:  " + args[i] + ".  The clientId cannot exceed 23 characters.";
               break;
            }
         } else if ("-m".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            payload = args[i];
         } else if ("-t".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            topicName = args[i];
         } else if ("-n".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            count = Integer.parseInt(args[i]);
         } else if ("-o".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            setfile(args[i]);
         } else if ("-r".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            dataStoreDir = args[i];
            persistence = true;
         } else if ("-v".equalsIgnoreCase(args[i])) {
            verbose = true;
         } else if ("-h".equalsIgnoreCase(args[i])) {
            showUsage = true;
            break;
         } else if ("-q".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            if ("0".equals(args[i])) {
               qos = AT_MOST_ONCE;
            } else if ("1".equals(args[i])) {
               qos = AT_LEAST_ONCE;
            } else if ("2".equals(args[i])) {
               qos = EXACTLY_ONCE;
            } else {
               showUsage = true;
               comment = "Invalid Parameter:  " + args[i];
               break;
            }
         } else if ("-c".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            if ("true".equalsIgnoreCase(args[i])) {
               cleanSession = true;
            } else if ("false".equalsIgnoreCase(args[i])) {
               cleanSession = false;
            } else {
               showUsage = true;
               comment = "Invalid Parameter:  " + args[i];
               break;
            }
         } else if ("-u".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            userName = args[i];
         } else if ("-p".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            password = args[i];
         } else if ("-w".equalsIgnoreCase(args[i]) && (i + 1 < args.length)) {
            i++;
            throttleWaitMSec = Integer.parseInt(args[i]);
         } else {
            showUsage = true;
            comment = "Invalid Parameter:  " + args[i];
            break;
         }
      }

      if (showUsage == false) {
         if (dataStoreDir == null)
            dataStoreDir = System.getProperty("java.io.tmpdir");

         if (clientId == null) {
            clientId = MqttClient.generateClientId();
            if (clientId.length() > 23) {
               // If the generated clientId is too long, then create one based on action and a
               // random number.
               clientId = String.format("%s_%.0f", action, new Random(99999));
            }
         }

         if (serverURI == null) {
            comment = "Missing required parameter: -s <serverURI>";
            showUsage = true;
         } else if (action == null) {
            comment = "Missing required parameter: -a publish|subscribe";
            showUsage = true;
         } else if (!("publish".equals(action)) && !("subscribe".equals(action))) {
            comment = "Invalid parameter: -a " + action;
            showUsage = true;
         } else if ((password != null) && (userName == null)) {
            comment = "Missing parameter: -u <userId>";
            showUsage = true;
         } else if ((userName != null) && (password == null)) {
            comment = "Missing parameter: -p <password>";
            showUsage = true;
         }
      }

      if (showUsage) {
         usage(args, comment);
         System.exit(1);
      }
   }

   /**
    * Primary MQTT client method that either publishes or subscribes messages on a topic
    */
   public void run() {

      try {
         if ("publish".equalsIgnoreCase(action)) {
            new MqttSamplePublish().doPublish(this);
         } else { // If action is not publish then it must be subscribe
            new MqttSampleSubscribe().doSubscribe(this);
         }
      } catch (Throwable e) {
         println("MqttException caught in MQTT sample:  " + e.getMessage());
         e.printStackTrace();
      }

      /*
       * close program handles
       */
         close();

      return;
   }

   /**
    * Output the usage statement to standard out.
    * 
    * @param args
    *           The command line arguments passed into main().
    * @param comment
    *           An optional comment to be output before the usage statement
    */
   private void usage(String[] args, String comment) {

      if (comment != null) {
         System.err.println(comment);
         System.err.println();
      }

      System.err
               .println("usage:  "
                        + "-s <Server URI> -a publish|subscribe [-i <clientId>][-t <topic name>][-m <payload>][-n <num messages>][-q 0|1|2][-o <logfile>][-c true|false][-r <temp datastore dir>][-u <userId>][-p <password>][-w <rate>][-v][-h]");
      System.err.println();
      System.err
               .println(" -i clientId  The client identity used for connection to the IMA server.  The maximum length is 23 characters.  This is an optional parameter.");
      System.err
               .println(" -s serverURI The URI address of the IMA server. This is in the format of tcp://<ipaddress>:<port>. This is a required parameter.");
      System.err.println(" -a action Either the String: publish or subscribe. This is a required parameter.");
      System.err.println(" -t topicName The name of the topic on which the messages are published. The default topic name is /MQTTv3Sample.");
      System.err.println(" -m message A String representing the message to be sent. Default value is \"IMA WebSocket Sample Message\"");
      System.err.println(" -n count The number of times the specified message is to be sent. The default number of message sent is 1.");
      System.err.println(" -q qos The Quality of Service level 0, 1, or 2 The default QOS is 0.");
      System.err
               .println(" -c cleanSession  true or false value indicating if session data is to be removed on client disconnect. Defaults value is true");
      System.err.println(" -o Log filename The log defaults to stdout.");
      System.err.println(" -r enable persistence and specify datastore directory. The default persistence is false.");
      System.err.println(" -u userid for secure communications. This is an optional parameter.");
      System.err.println(" -p password for secure communications.  This is an optional parameter.");
      System.err.println(" -w rate at which messages are sent in units of messages/second.");
      System.err.println(" -v Indicates verbose output");
      System.err.println(" -h output usage statement.");

   }
   
   private PrintStream stream = System.out;

   void setfile(String filename) {
      try {
         FileOutputStream fos = new FileOutputStream(filename);
         synchronized (stream) {
            stream = new PrintStream(fos);
         }
         if (verbose)
            println("Log sent to " + filename);

      } catch (Throwable e) {
         println("Log entries sent to System.out instead of " + filename);
      }
   }

   void println(String string) {
      synchronized (stream) {
         stream.println(new Timestamp(new java.util.Date().getTime()) + " " + string);
      }
   }

   void close() {
      synchronized (stream) {
         if (stream != System.out)
            stream.close();
      }
   }

}
