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
package svt.scale.vehicle;

import java.util.Properties;
import java.util.Vector;

import javax.net.SocketFactory;

import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttSecurityException;
import org.eclipse.paho.client.mqttv3.MqttTopic;
import org.eclipse.paho.client.mqttv3.internal.security.SSLSocketFactoryFactory;

import svt.util.SVTConfig;
import svt.util.SVTConfig.BooleanEnum;
import svt.util.SVTConfig.IntegerEnum;
import svt.util.SVTConfig.LongEnum;
import svt.util.SVTConfig.StringEnum;
import svt.util.SVTLog;
import svt.util.SVTLog.TTYPE;
import svt.util.SVTStats;
import svt.util.SVTUtil;
import svt.vehicle.SVTVehicle_Paho;

//
//  This is the highest level class for launching Automotive Telematics.
//  This class starts a thread per simulated car.  Each simulated car
//  instantiates a dedicated paho client which contains 4 threads.
//
//usage:  java  svt.scale.vehicle.SVTVehicleScale
//-server   <IP Address of ismserver.(String)>                    Default:  n/a
//-server2  <IP Address of 2nd ismserver.(String)>                Default:  n/a
//-appid    <Application ID used in the topic name.(String)>      Default:  1
//-userName <user name for ldap lookup(String)>                   Default:  n/a
//-password <password for ldap lookup(String)>                    Default:  n/a
//-port     <ismserver port.(Integer)>                            Default:  n/a
//-qos      <Quality of Service level.(Integer)>                  Default:  0
//-listenerQoS <Listener Quality of Service level.(Integer)>      Default:  1
//-jms      <Number of JMS vehicles.(Integer)>                    Default:  0
//-mqtt     <Number of MQTT vehicles.(Integer)>                   Default:  0
//-paho     <Number of paho vehicles.(Integer)>                   Default:  0
//-rate     <Number of messages per minute(Integer)>              Default:  60
//-keepAlive <keepAlive in seconds(Integer)>                      Default:  36000
//-connectionTimeout <connectionTimeout in seconds(Integer)>      Default:  36000
//-disconnectRate <0 to 9999, default 50. (Integer) >             Default:  50
//-minutes  <Run for number of minutes.(Long)>                    Default:  n/a
//-hours    <Run for number of hours.(Long)>                      Default:  n/a
//-messages <Run for number of messages.(Long)>                   Default:  n/a
//-help     <Print usage statement.(Boolean)>                     Default:  false
//-verbose  <Verbose output.(Boolean)>                            Default:  true
//-vverbose <Very verbose output.(Boolean)>                       Default:  false
//-order    <Send order data in message.(Boolean)>                Default:  false
//-listener <Create listener thread.(Boolean)>                    Default:  false
//-ssl      <set true for ssl connection(Boolean)>                Default:  false
//-cleanSession <set true for cleansession(Boolean)>              Default:  true
//-genClientID <true generates id based on host,pid,tid(Boolean)> Default:  true
//-iotf     <send json messages(Boolean)>                         Default:  false
//-disconnect <enable random disconnect(Boolean)>                 Default:  false
//
//  Example:
//  java -Xss1024K -Xshareclasses -Djavax.net.ssl.trustStore=/niagara/test/svt_atelm/AT/cacerts.jks 
//  -Djavax.net.ssl.trustStorePassword=svtfortest svt.scale.vehicle.SVTVehicleScale 
//  -server "${ORG}.messaging.internetofthings.ibmcloud.com" -userName ${USER} -password ${password} 
//  -port 8883 -qos 0 -paho 500 -messages 100000 -vverbose false -listener true -iotf true -ssl true 
//  -cleanSession false -rate 600
//

public class SVTVehicle {
   static TTYPE TYPE = TTYPE.SCALE;
   Vector<Thread> list = null;
   Vector<SVTStats> statslist = null;
   static boolean stop = false;
   static int count = 0;
   static int threadcount = 0;
   public static long t = 0;
   int i = 0;
   int p = 0;
   String hostname = SVTUtil.getHostname();
//   Byte[] conLock = new Byte[1];

   public static void main(String[] args) {

      SVTConfig.parse(args);

      SVTConfig.printSettings();

      if (SVTConfig.help() == false) {
         SVTVehicle stress = new SVTVehicle();
         stress.begin();
      }

      System.exit(0);
   }

   public void begin() {

      list = new Vector<Thread>();
      statslist = new Vector<SVTStats>();
      SVTStats stats = null;

      String userPrefix = SVTUtil.getPrefix(StringEnum.userName.value);
      long userIndex = SVTUtil.getIndex(StringEnum.userName.value);
      int userIndexDigits = SVTUtil.getDigits(StringEnum.userName.value);
      String name = null;
      Properties sslClientProps = null;
      SocketFactory socketFactory = null;

      if (BooleanEnum.ssl.value) {
         sslClientProps = getSSLClientProps();
         if (BooleanEnum.cacheSocketFactory.value) {
            socketFactory = getSSLSocketFactory(sslClientProps);
         }
      }
      
      while (p < IntegerEnum.paho.value) {
         stats = new SVTStats(1000 * 60 / IntegerEnum.rate.value, 1);
         statslist.add(stats);
         name = SVTUtil.formatName(userPrefix, userIndex, userIndexDigits);
         Thread t = new SVTVehicleEngine(stats, p + 1, name, sslClientProps, socketFactory);
         if (name != null) {
            t.setName(name);
         } else {
            t.setName("Unauth User");
         }
         list.add(t);
         userIndex++;
         p++;
      }

      long starttime = System.currentTimeMillis();
      try {
         for (Thread t : list) {
            SVTUtil.threadcount(1);
            t.start();
            if (LongEnum.connectionThrottle.value > 0L) {
               try {
                  Thread.sleep(LongEnum.connectionThrottle.value);
               } catch (InterruptedException e) {
               }
            }
         }

         long lasttime = System.currentTimeMillis();

         while (activethreads(list)) {
            for (Thread t : list) {
               while (t.isAlive()) {
                  t.join(20000);
                  if (StringEnum.statServer.value != null) {
                     lasttime = printStats(list, lasttime, hostname);
                  }
               }
            }
         }
      } catch (Throwable e) {
         e.printStackTrace();
         System.exit(1);
      }

      long count = printFinalStats(starttime);
      if ((count > 0) && (SVTUtil.stop(null) == false)) {
         SVTLog.log3(TYPE, "SVTVehicleScale Success!!!");
      } else {
         SVTLog.log3(TYPE, "SVTVehicleScale Failed!!!");
      }
   }

   boolean activethreads(Vector<Thread> list) {
      boolean active = false;
      int i = 0;
      while ((active == false) && (i < list.size())) {
         active = list.elementAt(i).isAlive();
         i++;
      }
      return active;
   }

   SVTVehicle_Paho client = null;
   MqttTopic topic = null;

   long printStats(Vector<Thread> list, long lasttime, String hostname) {
      long time = System.currentTimeMillis();
      String clientId = SVTUtil.getHostname() + "_" + SVTUtil.MyPid();
      double delta_ms = time - lasttime;
      double delta_sec = delta_ms / 1000.0;
      long cars = 0;
      long count = 0;
      long failedCount = 0;
      for (SVTStats s : statslist) {
         count += s.getICount();
         cars += s.getTopicCount();
         failedCount += s.getFailedCount();
      }

      double rate_sec = 0;
      if (delta_sec > 0)
         rate_sec = count / delta_sec;

      try {
         if (topic == null) {
            client = new SVTVehicle_Paho(StringEnum.statServer.value, StringEnum.statServer.value, IntegerEnum.statPort.value, 0, 1, false,
                  StringEnum.statUserName.value, StringEnum.statPassword.value, IntegerEnum.keepAlive.value,
                  IntegerEnum.connectionTimeout.value, true);
            topic = client.getTopic(clientId, "/svt/" + clientId);
         }

         String text = clientId + ":" + cars + ":" + count + ":" + delta_ms + ":" + rate_sec + ":" + failedCount;
         MqttMessage msg = new MqttMessage(text.getBytes());
         msg.setQos(1);
         topic.publish(msg);
      } catch (Throwable e) {
         SVTLog.logex(TYPE, "exception while publishing stats", e);
         topic = null;
      }

      return time;
   }

   long printFinalStats(long starttime) {
      double delta_ms = System.currentTimeMillis() - starttime;
      double delta_sec = delta_ms / 1000.0;
      double delta_min = delta_sec / 60.0;
      long count = 0;
      long cars = 0;
      for (SVTStats s : statslist) {
         count += s.getCount();
         cars += s.getTopicCount();
      }

      double rate_min = 0;
      if (delta_min > 0)
         rate_min = count / delta_min;
      double rate_sec = 0;
      if (delta_sec > 0)
         rate_sec = count / delta_sec;

      SVTLog.log3(TYPE, String.format("%s%10.2f%s%10.2f%s%10.2f%s", SVTUtil.MyPid() + ": " + cars + " clients sent " + count
            + " messages in ", delta_min, " minutes.  The rate was ", rate_min, " msgs/min, ", rate_sec, " msgs/sec"));

      return count;
   }
   
   private Properties getSSLClientProps() {
      Properties sslClientProps = new Properties();
      // sslClientProps.setProperty("com.ibm.ssl.protocol", "SSLv3");
      // sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "SSL_RSA_WITH_AES_128_CBC_SHA");
      sslClientProps.setProperty("com.ibm.ssl.protocol", "TLSv1.2");
      sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "TLS_RSA_WITH_AES_128_GCM_SHA256");

      String keyStore = System.getProperty("com.ibm.ssl.keyStore");
      if (keyStore == null)
         keyStore = System.getProperty("javax.net.ssl.keyStore");

      String keyStorePassword = System.getProperty("com.ibm.ssl.keyStorePassword");
      if (keyStorePassword == null)
         keyStorePassword = System.getProperty("javax.net.ssl.keyStorePassword");

      String trustStore = System.getProperty("com.ibm.ssl.trustStore");
      if (trustStore == null)
         trustStore = System.getProperty("javax.net.ssl.trustStore");

      String trustStorePassword = System.getProperty("com.ibm.ssl.trustStorePassword");
      if (trustStorePassword == null)
         trustStorePassword = System.getProperty("javax.net.ssl.trustStorePassword");

      if (keyStore != null)
         sslClientProps.setProperty("com.ibm.ssl.keyStore", keyStore);
      if (keyStorePassword != null)
         sslClientProps.setProperty("com.ibm.ssl.keyStorePassword", keyStorePassword);
      if (trustStore != null)
         sslClientProps.setProperty("com.ibm.ssl.trustStore", trustStore);
      if (trustStorePassword != null)
         sslClientProps.setProperty("com.ibm.ssl.trustStorePassword", trustStorePassword);

      return sslClientProps;
   }
   
   // NOTE:  SSLSocketFactoryFactory is an internal paho class
   // this method is an attempt to optimize loading of the truststore.
   // though I have not noticed a real improvement.
   private SocketFactory getSSLSocketFactory(Properties sslClientProps) {
      SSLSocketFactoryFactory socketFactoryFactory = new SSLSocketFactoryFactory();
      socketFactoryFactory.initialize(sslClientProps, null);
      SocketFactory socketFactory = null;
      try {
         socketFactory = socketFactoryFactory.createSocketFactory(null);
      } catch (MqttSecurityException e) {
         // TODO Auto-generated catch block
         e.printStackTrace();
      }
      return socketFactory;
   }
}
