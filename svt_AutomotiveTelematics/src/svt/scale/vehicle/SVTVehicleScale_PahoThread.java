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

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.Random;
import java.util.Set;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import svt.util.SVTConfig.BooleanEnum;
import svt.util.SVTConfig.IntegerEnum;
import svt.util.SVTLog;
import svt.util.SVTLog.TTYPE;
import svt.util.SVTStats;
import svt.util.SVTUtil;
import svt.vehicle.SVTVehicle_Paho;
import svt.vehicle.SVTVehicle_Paho.SVTMqttClient;

public class SVTVehicleScale_PahoThread extends SVTVehicleScale implements Runnable {
   static TTYPE TYPE = TTYPE.PAHO;
   String mode = "connect_once";
   String ismserver = "";
   String ismserver2 = "";
   Integer ismport = null;
   String topicName = "";
   String appId = "";
   long minutes = 0;
   long messages = 0;
   long max = 0;
   SVTStats stats = null;
   Hashtable<MqttTopic, MqttClient> tlist;
   // Vector<Thread> list = null;
   // Vector<SVTStats> statsList = null;
   long carId = 0;
   boolean order = true;
   Integer gpsQoS = 0;
   Set<MqttTopic> failures = new HashSet<MqttTopic>();
   boolean listener = false;
   String listenerTopic = null;
   boolean ssl = false;
   String userName = null;
   String password = null;
   int keepAlive = 36000;
   int connectionTimeout = 36000;
   boolean cleanSession = true;
   String subTopicName = null;
   Integer listenerQoS = 1;
   boolean genClientID = true;
   DateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSSZ");


   SVTVehicleScale_PahoThread(String mode, String ismserver, String ismserver2, Integer ismport, String appId, Long minutes, Long hours,
         Long messages, SVTStats stats, long carId, boolean order, Integer gpsQoS, boolean listener, Integer listenerQoS, boolean ssl,
         String userName, String password, int keepAlive, int connectionTimeout, boolean cleanSession, boolean genClientID) {
      this.mode = mode;
      this.ismserver = ismserver;
      this.ismserver2 = ismserver2;
      this.ismport = ismport;
      this.appId = appId;
      if (minutes != null) {
         this.minutes = minutes;
      }
      if (hours != null) {
         this.minutes += (hours * 60);
      }
      if (messages != null) {
         this.messages = messages;
      }
      this.max = this.minutes * 60000;
      this.stats = stats;
      this.carId = carId;
      this.order = order;
      this.gpsQoS = gpsQoS;
      this.listener = listener;
      this.listenerQoS = listenerQoS;
      this.ssl = ssl;
      this.userName = userName;
      this.password = password;
      this.keepAlive = keepAlive;
      this.connectionTimeout = connectionTimeout;
      this.cleanSession = cleanSession;
      this.genClientID = genClientID;
   }

   public static String createLargeMessage(int size, int extraBits) {

      Random randomGenerator = new Random();
      StringBuilder messageBody = new StringBuilder();
      long messageSizeBytes = size * 1024 * 1024 + extraBits;
      for (int x = 0; x < messageSizeBytes; x++) {
         int randomInt = randomGenerator.nextInt(10);
         messageBody.append(randomInt);
      }
      return messageBody.toString();
   }

   public static String createSmallMessage(int size) {

      Random randomGenerator = new Random();
      StringBuilder messageBody = new StringBuilder();
      long messageSizeBytes = size * 1024;
      for (int x = 0; x < messageSizeBytes; x++) {
         int randomInt = randomGenerator.nextInt(10);
         messageBody.append(randomInt);
      }
      return messageBody.toString();
   }

   public void run() {
      String pClientId = null;
      Random random = null;
      if (BooleanEnum.disconnect.value) {
         random = new Random(System.currentTimeMillis());
      }

      if (BooleanEnum.iotf.value == true) {
         pClientId = userName;
         userName = "use-token-auth";
      } else if (genClientID) {
         pClientId = SVTUtil.MyClientId(TYPE);
      } else {
         pClientId = userName;
      }
      this.topicName = SVTUtil.MyTopicId(appId, pClientId);
      String message = "";
      // SVTLog.log(TYPE, "thread created");

      SVTVehicle_Paho vehicle = new SVTVehicle_Paho(ismserver, ismserver2, ismport, carId, gpsQoS, ssl, userName, password, keepAlive,
            connectionTimeout, cleanSession);
      // if (this.listener == true)
      // vehicle.startListener();

      try {
         this.stats.reinit();

         SVTLog.log3(TYPE, pClientId + " to send messages on topic " + topicName + " Connection mode is " + mode);

         if ("connect_each".equals(mode)) {
            while (true) {
               this.stats.beforeMessage();
               if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                     || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                  break;
               }
               int x = (int) (Math.random() * 1000.0);
               int y = (int) (Math.random() * 1000.0);
               vehicle.sendMessage(topicName, pClientId, "GPS " + x + "," + y, this.stats.increment());
               if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                     || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                  break;
               }
               this.stats.afterMessageRateControl();
            }
         } else if ("connect_once".equals(mode)) {
            SVTMqttClient svtclient = vehicle.getSVTClient(pClientId);
            MqttTopic topic = svtclient.client.getTopic(topicName);
            String messagetext = null;
            Long startTimeStamp = null;
            Long prevTimeStamp = null;

            if (listener == true) {
               if (BooleanEnum.iotf.value == false) {
                  listenerTopic = "/CAR/" + pClientId + "/APP/" + appId + "/#";
               } else {
                  listenerTopic = "iot-2/cmd/+/fmt/+";
               }
               svtclient.client.subscribe(listenerTopic);
            }

            while (true) {
               this.stats.beforeMessage();
               if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                     || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                  break;
               }
               int x = (int) (Math.random() * 1000.0);
               int y = (int) (Math.random() * 1000.0);

               if (BooleanEnum.iotf.value) {
                  Long theTimeStamp = System.currentTimeMillis();
                  if (this.stats.getCount() == 0) {
                     startTimeStamp = theTimeStamp;
                     prevTimeStamp = theTimeStamp;
                  }
                  while (theTimeStamp == prevTimeStamp) {
                     theTimeStamp = System.currentTimeMillis();
                  }
                  prevTimeStamp = theTimeStamp;


                  String payload = "GPS " + x + "," + y;
                  messagetext = "{\"ts\":" + theTimeStamp + ",\"d\":{\"Time\":\"" + df.format(new Date()) + "\", \"Pub\":\"" + pClientId
                        + "\" , \"Topic\":\"" + topicName + "\" , \"QoS\":" + gpsQoS + " , \"Index\":" + this.stats.getCount()
                        + " , \"payload\":\"" + payload + "\"  }}";
               } else {
                  messagetext = "GPS " + x + "," + y;
               }

               vehicle.sendMessage(topic, messagetext, gpsQoS, pClientId);
               this.stats.increment();

               if ((listener) && (svtclient.stacksize() > 0))
                  vehicle.sendReplyMessage(svtclient);

               if ((BooleanEnum.disconnect.value) && (random.nextInt(9999) < IntegerEnum.disconnectRate.value)) {
                  vehicle.reconnect(svtclient);
               }
               
               if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                     || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                  break;
               }
               this.stats.afterMessageRateControl();
            }

            if (listener == true) {
               svtclient.client.unsubscribe(listenerTopic);
            }

            try {
               SVTLog.log3(TYPE, pClientId + ".disconnect(20000)");
               svtclient.client.disconnect(20000);
               SVTLog.log3(TYPE, pClientId + " disconnected!");
            } catch (MqttException e) {
               SVTLog.logex(TYPE, pClientId + ": Exception caught from " + pClientId + ".disconnect(20000)", e);
            }
            svtclient.client.close();

         } else if ("order_message".equals(mode)) {
            MqttClient client = vehicle.getClient(pClientId);
            MqttTopic topic = client.getTopic(topicName);
            while (true) {
               this.stats.beforeMessage();
               if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                     || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                  break;
               }

               /*
                * Below is where message is published. The format of message could require specific data that is agreed by subscriber. For
                * scenario cars publishing with certain statistics, below is format:
                * 
                * text = "ORDER:" + clientID + ":" + config.qos + ":" + iCount + ":" + iCount + ":" + config.payload;
                * 
                * vehicle.sendMessage(topic, "GPS " + x + "," + y);
                * 
                * TODO: decide if we need to vary size of a string message public static String createLargeMessage(int size, int extraBits)
                */
               message = createLargeMessage(4096, 16);
               vehicle.sendMessage(topic, "ORDER:" + pClientId + ":" + gpsQoS + ":" + this.stats.getCount() + ":" + this.stats.getCount()
                     + ":" + message, gpsQoS, pClientId);

               this.stats.increment();
               if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                     || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                  break;
               }
               this.stats.afterMessageRateControl();
            }
            try {
               client.disconnect();
            } catch (MqttException e) {
               SVTLog.logex(TYPE, pClientId + ": Exception caught from client.disconnect()", e);
            }
         } else if ("batch".equals(mode)) {

            SVTMqttClient svtclient = vehicle.getSVTClient(pClientId);
            MqttTopic topic = svtclient.client.getTopic(topicName);
            boolean sent = false;
            while (true) {
               this.stats.beforeMessage();
               if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                     || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                  break;
               }
               sent = false;
               int x = (int) (Math.random() * 1000.0);
               int y = (int) (Math.random() * 1000.0);
               while (sent == false) {
                  if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                        || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                     break;
                  }
                  try {
                     if (topic == null) {
                        // client = vehicle.getClient(pClientId);
                        topic = svtclient.client.getTopic(topicName);
                     }
                     vehicle.sendMessage(topic, "GPS " + x + "," + y, gpsQoS, pClientId);
                     sent = true;
                  } catch (MqttException e) {
                     topic = null;
                     client = null;
                  }
               }
               this.stats.increment();
               if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                     || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                  break;
               }
               this.stats.afterMessageRateControl();
            }
            try {
               svtclient.client.disconnect();
            } catch (MqttException e) {
               SVTLog.logex(TYPE, pClientId + ": Exception caught from client.disconnect()", e);
            }
         } else if ("batch_with_order".equals(mode)) {
            MqttClient myclient = null;
            MqttTopic mytopic = null;
            int tlistsize = 50;
            tlist = new Hashtable<MqttTopic, MqttClient>();
            this.stats.setTopicCount(tlistsize);
            for (i = 0; i < tlistsize; i++) {
               myclient = vehicle.getClient(pClientId + "_" + i);
               mytopic = myclient.getTopic(topicName + "_" + i);
               tlist.put(mytopic, myclient);
            }

            int x = (int) (Math.random() * 1000.0);
            int y = (int) (Math.random() * 1000.0);
            int i = 0;
            MqttMessage textMessage = null;
            if (this.order == false) {
               String text = "GPS " + x + "," + y;
               textMessage = new MqttMessage(text.getBytes());
               textMessage.setQos(gpsQoS);
            }

            Set<MqttTopic> topicSet = tlist.keySet();

            while (true) {
               if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max))
                     || ((this.messages > 0) && (this.stats.getCount() >= this.messages))) {
                  break;
               }
               this.stats.beforeMessage();
               for (MqttTopic topic : topicSet) {
                  if (this.order == true) {
                     String text = "ORDER:" + tlist.get(topic).getClientId() + ":" + i + ":" + "GPS " + x + "," + y;
                     textMessage = new MqttMessage(text.getBytes());
                     textMessage.setQos(this.gpsQoS);
                  }
                  try {
                     topic.publish(textMessage);
                     this.stats.increment();
                  } catch (MqttException e) {
                     failures.add(topic);
                  }
               }

               synchronized (failures) {
                  failures.notifyAll();
               }
               this.stats.afterMessageRateControl();
               i++;
            }
            this.stats.setTopicCount(0);
            tlist.clear();
         }

      } catch (Throwable e) {
         SVTLog.logex(TYPE, "sendMessage", e);
         SVTUtil.stop(true);
      }

      // if (this.listener == true)
      // vehicle.stopListener();

      int t = SVTUtil.threadcount(-1);

      String text = "mqttclient ," + pClientId + ", finished__" + message + ". Ran for ," + this.stats.getRunTimeSec() + ", sec. Sent ,"
            + this.stats.getCount() + ", messages. Rate: ," + this.stats.getRatePerMin() + ", msgs/min ," + this.stats.getRatePerSec()
            + ", msgs/sec " + t + " clients remain.";

      SVTLog.log3(TYPE, text);
      return;
   }

   class failuresThread implements Runnable {
      MqttClient myclient = null;
      Set<MqttTopic> topics = new HashSet<MqttTopic>();
      Set<MqttTopic> connected = new HashSet<MqttTopic>();
      SVTStats stats = null;

      failuresThread(SVTStats stats) {
         this.stats = stats;
      }

      public void run() {
         long failedCount = 0;
         while (this.stats.getDeltaRunTimeMS() < max) {
            failedCount = 0;
            synchronized (failures) {
               try {
                  failures.wait();
                  topics.addAll(failures);
                  failures.clear();
               } catch (InterruptedException e) {
               }
            }
            connected.clear();
            for (MqttTopic topic : topics) {
               myclient = tlist.get(topic);
               if (myclient.isConnected() == true) {
                  connected.add(topic);
               } else {
                  for (int j = 0; j < 3; j++) {
                     try {
                        myclient.connect();
                        connected.add(topic);
                     } catch (Throwable e) {
                     }
                     if (myclient.isConnected() == false) {
                        try {
                           Thread.sleep(j * 500);
                        } catch (Throwable e) {
                        }
                     } else
                        break;
                  }
                  if (myclient.isConnected() == false) {
                     failedCount++;
                  }
               }
            }
            if (failedCount > 0) {
               this.stats.setFailedCount(failedCount);
               this.stats.setTopicCount(tlist.size() - failedCount);
            }

            for (MqttTopic topic : connected) {
               topics.remove(topic);
            }
         }
         return;
      }
   }
}
