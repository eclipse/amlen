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

package svt.scale.backend;

import java.util.Collections;
import java.util.Hashtable;
import java.util.LinkedList;
import java.util.List;
import java.util.Properties;
import java.util.Random;
import java.util.Vector;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttAsyncClient;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.persist.MqttDefaultFilePersistence;

import svt.util.SVTTopicMessage;

/**
 * Class to encapsulate the ISM topic subscribe logic.
 * 
 * This class contains the doSubscribe method for subscribing to ISM topic messages. It also defines the necessary MQTT callbacks for
 * asynchronous subscriptions.
 * 
 */

public class ATBackendEngine implements MqttCallback {
   // Byte[] lock = new Byte[1];
   // Byte[] cLock = new Byte[1];

   ATBackend config = null;
   // String clientId = null;
   public List<SVTTopicMessage> resendMessageList = Collections.synchronizedList(new LinkedList<SVTTopicMessage>());
   // public List<SVTTopicMessage> orderCheckMessageList = Collections.synchronizedList(new LinkedList<SVTTopicMessage>());
//   public ArrayList<SVTTopicMessage> orderCheckMessageList = new ArrayList<SVTTopicMessage>();
//   public ArrayList<SVTTopicMessage> orderMessageList = new ArrayList<SVTTopicMessage>();

     public Vector<SVTTopicMessage> orderCheckMessageList = new Vector<SVTTopicMessage>();
//   Integer list = 0;

   AtomicLong receivedCount = new AtomicLong(0L);
   // private Lock cmutex = new ReentrantLock();
   // public Lock subscriberMutex = new ReentrantLock();
   MqttAsyncClient client = null;
   MqttConnectOptions options = null;
   Long batch = 100000L;
   Long main = 100000L;
   // long startTime = 0;
   MqttDefaultFilePersistence dataStore = null;
   IMqttToken conToken = null;
   IMqttToken subToken = null;
   Long qosCount[] = { 0L, 0L, 0L };
   boolean notifyCalled = false;



   /**
    * Callback invoked when the MQTT connection is lost
    * 
    * @param e
    *           exception returned from broken connection
    * 
    */

   public void connectionLost(Throwable e) {
      config.log.println("Lost connection to " + config.serverURI + ".  " + e.getMessage());
      if (shouldReconnect(null, null)) {

         long remainingTime = config.time.remainingRunTimeMillis();
         // if more than 1000ms runtime remain then sleep 500ms to prevent overwhelming the server
         if (remainingTime > 1000) {
            sleep(500);
            // test if reconnect flag is still true after sleeping
            if (shouldReconnect(null, null)) {
               subscriber_connect(true);
            }
         } else if (remainingTime > 500) {
            // if less than 1000ms runtime remain (and more than 500ms) then sleep 200ms
            sleep(200);
            // test if reconnect flag is still true after sleeping
            if (shouldReconnect(null, null)) {
               subscriber_connect(true);
            }
         }
      }

   }

   public void sleep(long s) {
      try {
         Thread.sleep(s);
      } catch (InterruptedException e2) {
         System.exit(1);
      }
   }

   public synchronized void subscriber_connect(boolean verbose) {
      if (config.debug)
         config.log.println("entering subscriber_connect");

      int currentConnectionRetry = 0;
      while ((client.isConnected() == false) && shouldReconnect(null, null) && !subscriberIsDone(null, null)) {
         try {
            if (config.connectionRetry != -1) {
               currentConnectionRetry++;
            }
            conToken = null;
            if (verbose)
               config.log.printTm("Subscriber " + this.client.getClientId() + " connecting ...");
            conToken = client.connect(options);
            if (verbose)
               config.log.print("waiting...");
            while ((conToken != null) && (conToken.isComplete() == false)) {
               conToken.waitForCompletion();
            }
            if (verbose)
               config.log.println("\n Established connection to " + config.serverURI);
         } catch (Exception ex) {
            String exMsg = ex.getMessage();
            String m = "failed connection to " + this.client.getServerURI() + ":  " + exMsg;
            if (ex.getCause() != null) {
               m = m + ", caused by " + ex.getCause().getMessage();
            }
            m = m + "\n";
            config.log.print(m);
            if (currentConnectionRetry >= config.connectionRetry && config.connectionRetry != -1) {
               shouldReconnect(false, exMsg + " - Aborting connection attempts after " + currentConnectionRetry + " failures. ");
               // config.log.println(exMsg);
            }
            sleep(500);
         }

      }
      if (config.debug)
         config.log.println("exiting subscriber_connect");
   }

   public void subscriber_disconnect(boolean verbose) {
      while (client.isConnected() == true) {
         try {
            conToken = null;
            if (verbose)
               config.log.printTm("Subscriber disconnecting ...");
            conToken = client.disconnect();
            if (verbose)
               config.log.print("waiting...");
            while ((conToken != null) && (conToken.isComplete() == false)) {
               conToken.waitForCompletion();
            }
            if (verbose)
               config.log.println("\n Disconnected from " + config.serverURI);

         } catch (Exception ex) {
            config.log.println("failed.  " + ex.getMessage() + "\n");
            sleep(500);
         }
      }
   }

   /**
    * Synchronized method for indicating all messages received.
    * 
    * @param setDone
    *           optional Boolean parameter indicating all messages received.
    * 
    * @return Boolean flag
    */
   private Boolean doneFlag = false;

   synchronized public boolean subscriberIsDone(Boolean done, String message) {
      if ((done != null) && (done == true)) {
         if (config.debug)
            config.log.println(message);
         doneFlag = done;
      }
      return doneFlag;
   }

   private Boolean reconnect = true;

   synchronized public boolean shouldReconnect(Boolean done, String message) {
      if (done != null) {
         if (config.debug)
            config.log.println(message);
         reconnect = done;
      }
      return reconnect;
   }

   private Boolean resendDoneFlag = false;

   synchronized public boolean isResendDone(Boolean done, String message) {
      if ((done != null) && (done == true)) {
         if (config.debug)
            config.log.println(message);
         resendDoneFlag = done;
      }
      return resendDoneFlag;
   }

   synchronized private long qosCount(int i, Integer n) {
      if (n != null) {
         qosCount[i] += n;
      }
      return this.qosCount[i];
   }

   boolean useWait = false;
   AtomicLong startWaitTime = new AtomicLong(0L);
   AtomicInteger maxWait = new AtomicInteger(3);

   // Byte[] waitMutex = new Byte[1];

   /**
    * subscribe to messages published on the topic name
    * 
    * @param config
    *           MqttSample instance containing configuration settings
    * 
    * @throws MqttException
    */
   public void run(String clientId, ATBackend config) throws MqttException {

      this.config = config;
      maxWait.set(config.initialWait);
      if ((config.initialWait > 0) || (config.wait > 0)) {
         useWait = true;
      }

      // this.clientId = new String(clientId);

      if (config.persistence)
         dataStore = new MqttDefaultFilePersistence(config.dataStoreDir);

      options = new MqttConnectOptions();
      if ((config.serverURI.indexOf(' ') != -1) || (config.serverURI.indexOf(',') != -1) || (config.serverURI.indexOf('|') != -1)
            || (config.serverURI.indexOf('!') != -1) || (config.serverURI.indexOf('+') != -1)) {
         String[] list = config.serverURI.split("[ ,|!+]");

         for (int l = 0; l < list.length; l++) {
            config.log.println("list[" + l + "] \"" + list[l] + "\"");
         }

         client = new MqttAsyncClient(list[0], clientId, dataStore);
         options.setServerURIs(list);
      } else {
         client = new MqttAsyncClient(config.serverURI, clientId, dataStore);
      }

      if ("3.1.1".equals(config.mqttVersion)) {
         options.setMqttVersion(org.eclipse.paho.client.mqttv3.MqttConnectOptions.MQTT_VERSION_3_1_1);
      } else if ("3.1".equals(config.mqttVersion)) {
         options.setMqttVersion(org.eclipse.paho.client.mqttv3.MqttConnectOptions.MQTT_VERSION_3_1);
      }
      client.setCallback(this);

      // set CleanSession true to automatically remove server data associated
      // with the client id at disconnect. In this sample the clientid is based on a random number
      // and not typically reused, therefore the release of client's server side data on
      // disconnect is appropriate.
      if (config.oauthToken != null) {
         options.setUserName("IMA_OAUTH_ACCESS_TOKEN");
         options.setPassword(config.oauthToken.toCharArray());
      } else if (config.password != null) {
         options.setUserName(config.userName);
         options.setPassword(config.password.toCharArray());
      }

      if (config.ssl == true) {
         Properties sslClientProps = new Properties();

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

         options.setSSLProperties(sslClientProps);
      }

      // KeepAliveInterval is set to 0 to keep the connection from timing out
      options.setKeepAliveInterval(config.keepAliveInterval);
      options.setConnectionTimeout(config.connectionTimeout);
      if (config.maxInflight > 0)
         options.setMaxInflight(config.maxInflight);

      // CleanSessionState indicates if you want to skip Rick's Swizzle on
      // connections for HA Failover reconnects.
      if (!config.cleanSessionState) {
         // Rick's Swizzle for HA Connections

         if (config.cleanSession) {
            options.setCleanSession(true);
            subscriber_connect(true);
            subscriber_disconnect(true);
         }

         options.setCleanSession(false);
         subscriber_connect(true);
      } else {
         options.setCleanSession(config.cleanSession);
         subscriber_connect(true);
      }

      if (client.isConnected()) {
         // Subscribe to the topic. messageArrived() callback invoked when
         // message received.
         // startTime = System.currentTimeMillis();
         // if (config.monitor == false)
         // //???????????????????????????????????????????????????? ------------ RKA
         subToken = client.subscribe(config.topicName, config.qos);
         while ((subToken != null) && (subToken.isComplete() == false)) {
            subToken.waitForCompletion();
         }

         config.log.println("Client '" + client.getClientId() + "' subscribed to topic: '" + config.topicName + "' on "
               + client.getServerURI() + " with QOS " + config.qos + ".");

         if ((config.count == 0) && (config.time.configuredRunTimeMillis() == 0)) {
            subscriberIsDone(true, "doSubscribe is setting isDone(true) because config.count == 0 and config.minutes==0");
            if (config.unsubscribe) {
               // Disconnect the client
               subToken = client.unsubscribe(config.topicName);
               while ((subToken != null) && (subToken.isComplete() == false)) {
                  subToken.waitForCompletion();
               }
            }

            subscriber_disconnect(true);

         } else {
            Thread batchMessage = new Thread(new batchMessage());
            batchMessage.start();

            Thread orderMessage = null;
            Thread resendMessage = null;
            Thread sprayMessage[] = null;
            if (config.order == true) {
               if (config.gatherer) {
                  orderMessage = new Thread(new globalOrderMessage(this));
               } else {
                  orderMessage = new Thread(new orderMessage());
               }
               orderMessage.start();
            } else if ((config.publisherURI != null) || (config.app != null) || (config.car != null)) {
               if (config.spray) {
                  sprayMessage = new Thread[config.sprayThreads];
                  for (int i=0;i<config.sprayThreads;i++) {
                    sprayMessage[i] = new Thread(new spray(config));
                  }
                  for (int i=0;i<config.sprayThreads;i++) {
                     sprayMessage[i].start();
                   }
               } else {
                  resendMessage = new Thread(new resendMessage(config));
                  resendMessage.start();
               }}
            long lReceivedCount = 0L;
            while (!subscriberIsDone(null, null)) {
               lReceivedCount = receivedCount.get();
               int size = 0;
               // synchronized (list) {
               // size = orderCheckMessageList.size();
               // }
               // if (size > 500000) {
               // shouldReconnect(false, "suspending reconnect because receivedMessageList.size is " + size);
               // subscriber_disconnect(false);
               // while (size > 10000) {
               // sleep(50);
               // }
               // synchronized (list) {
               // size = orderCheckMessageList.size();
               // }
               //
               // shouldReconnect(true, "Enabling reconnect because receivedMessageList.size is " + size);
               // subscriber_connect(false);
               // }
               // synchronized (list) {
               size = orderCheckMessageList.size();
               // }

               if (config.time.runTimeExpired()) {
                  config.log.println("Received " + receivedCount.get() + " messages in " + config.time.actualRunTimeMillis() + " ms");
                  subscriberIsDone(true, "doSubscribe is setting isDone(true) because runtime has expired");
                  synchronized (batch) {
                     batch.notify();
                  }
               } else if (((config.defaultcount == false) && (lReceivedCount == config.count))
                     || ((config.defaultcount == true) && (config.time.configuredRunTimeSeconds() == 0))) {
                  config.log.println("Received " + receivedCount.get() + " messages.");
                  synchronized (batch) {
                     batch.notify();
                  }
                  subscriberIsDone(true, "messageArrived is setting isDone(true) because config.count has been reached");
               } else if (((config.order == true) || (config.publisherURI != null)) && (maxWait.get() > 0) && (size == 0)) {
                  if (startWaitTime.get() == 0)
                     startWaitTime.set(System.currentTimeMillis());
                  else if ((System.currentTimeMillis() - startWaitTime.get()) >= (maxWait.get() * 60000l)) {
                     subscriberIsDone(true, "subscriberIsDone set true since no messages received in " + maxWait.get() + " minutes");
                     synchronized (batch) {
                        batch.notify();
                     }
                  }
               }

               if (config.throttleWaitMSec > 0) {
                  double elapsed = (double) config.time.actualRunTimeMillis();
                  double projected = ((double) lReceivedCount / (double) config.throttleWaitMSec) * 1000.0d;
                  double sleepInterval = projected - elapsed;

                  if (sleepInterval > 400) {
                     // going too fast
                     config.log.println("need to slow down ... Received " + receivedCount.get() + " messages in " + elapsed
                           + " milliseconds, sleepInterval is " + (long) (sleepInterval - 200));
                     shouldReconnect(false, "suspending reconnect because throttle");
                     subscriber_disconnect(false);
                     sleep((long) sleepInterval - 100);
                  } else if (sleepInterval < -400) {
                     // going too slow
                     config.log.println("need more messages... Received " + receivedCount.get() + " messages in " + elapsed
                           + " milliseconds, sleepInterval is " + (long) sleepInterval);
                     shouldReconnect(true, "Enabling reconnect because throttle");
                     subscriber_connect(false);
                     sleep(config.throttleWaitMSec / 20);
                     if (sleepInterval > -1000) {
                        shouldReconnect(false, "suspending reconnect because throttle");
                        subscriber_disconnect(false);
                     }
                  }
               } else {
                  synchronized (main) {
                     try {
                        main.wait(100);
                     } catch (InterruptedException e) {
                     }
                  }
               }

            }

            // if (config.resendURI != null) {
            // while (isResendDone(null, null)) {
            // sleep(500);
            // }
            // }

            if (config.unsubscribe) {
               // Disconnect the client
               subToken = client.unsubscribe(config.topicName);
               while ((subToken != null) && (subToken.isComplete() == false)) {
                  subToken.waitForCompletion();
               }
            }

            subscriber_disconnect(true);

            if (config.order) {
               try {
                  orderMessage.join();
               } catch (InterruptedException e) {
                  e.printStackTrace();
               }
            }
            if ((config.order == false) && (config.publisherURI != null)) {
               try {
                  resendMessage.join();
               } catch (InterruptedException e) {
                  e.printStackTrace();
               }
            }
         }
      }

      if (!config.cleanSessionState) {
         if (config.cleanSession) {
            options.setCleanSession(true);
            subscriber_connect(true);
            subscriber_disconnect(true);
         }
      }

      client.close();

      if (config.debug) {
         config.log.println("Subscriber disconnected.");
         config.log.println("doSubscribe return.");
      }
      return;
   }

   /**
    * Callback invoked when a message arrives.
    * 
    * @param topic
    *           the topic on which the message was received.
    * @param receivedMessage
    *           the received message.
    * @throws Exception
    *            the exception
    */
   // public void messageArrived(MqttTopic topic, MqttMessage receivedMessage) throws Exception {
   @Override
   public void messageArrived(String topic, MqttMessage receivedMessage) throws Exception {
      // increment count of received messages
      long lreceivedcount = receivedCount.incrementAndGet();
      if (useWait) {
         startWaitTime.set(0);
      }
      if (config.order == true) {
         qosCount(receivedMessage.getQos(), 1);
      }

      if (config.veryverbose) {
         config.log.println("Received message " + lreceivedcount + " on topic '" + topic + "' and Qos " + receivedMessage.getQos() + " "
               + new String(receivedMessage.getPayload()) + ". Dup flag is " + receivedMessage.isDuplicate() + ", isRetained is "
               + receivedMessage.isRetained());
      }

      if (lreceivedcount == 1) {
         config.log.println("Received first message on topic '" + topic + "'");
         byte[] msgBytes = receivedMessage.getPayload();
         config.log.println("MSG:" + new String(msgBytes));
         if (useWait) {
            maxWait.set(config.wait);
         }
      }

      if ((config.order == true) && (topic.indexOf("gps") != -1)) {
//         synchronized (list) {
            orderCheckMessageList.add(new SVTTopicMessage(topic, receivedMessage));
//         }
         // if (config.veryverbose)
         // config.log.println(new String(receivedMessage.getPayload()) + " added to end of orderCheckMessageList");
      }
      // if it is not a gps message or a delta message
      
      if (config.publisherURI != null) {
         if ((config.app != null) || (config.car != null)) {
            if ((config.iotf == true) && (topic.indexOf("gps") == -1) && (topic.indexOf("delta") == -1)) {
               resendMessageList.add(new SVTTopicMessage(topic, receivedMessage));
               if (config.veryverbose)
                  config.log.println(new String(receivedMessage.getPayload()) + " added to end of receivedMessageList");
            }
            // if the message is either to or from the USER then it will contain USER in the topic
            if ((config.iotf == false) && (topic.indexOf("USER") != -1)) {
               resendMessageList.add(new SVTTopicMessage(topic, receivedMessage));
               if (config.veryverbose)
                  config.log.println(new String(receivedMessage.getPayload()) + " added to end of receivedMessageList");
            }
         }
      }
      
      if ((config.verbose == true) && (lreceivedcount % batch == 0)) {
         synchronized (batch) {
            notifyCalled = true;
            batch.notify();
         }
      }

   }

   class batchMessage implements Runnable {
      long nowTime = 0;
      long prevTime = 0;
      long nowCount = 0;
      long prevCount = 0;
      long rate = 0;
      long delta = 0;
      boolean first = true;

      public void run() {
         while (subscriberIsDone(null, null) == false) {
            synchronized (batch) {
               try {
                  while (first || ((delta = (System.currentTimeMillis() - prevTime)) < 20000)) {
                     batch.wait(20000 - delta);
                     first = false;
                     if (notifyCalled)
                        break;
                  }
               } catch (InterruptedException e) {
               }
               notifyCalled = false;
            }
            nowTime = System.currentTimeMillis();
            nowCount = receivedCount.get();

            if ((prevTime > 0) && (subscriberIsDone(null, null) == false) && ((nowTime - prevTime) > 0)) {
               rate = ((nowCount - prevCount) * 1000L + 500L) / (nowTime - prevTime);
               config.log.println(" Received last " + (nowCount - prevCount) + " msgs at the rate " + rate + " msg/sec");
            }
            prevTime = nowTime;
            prevCount = nowCount;
         }
      }
   }

   class orderMessage implements Runnable {
      Hashtable<String, Long> publishersIndexTable = new Hashtable<String, Long>();
      Long previous = 0L;
      Long incoming = 0L;
//      LinkedList<SVTTopicMessage> orderMessageList = new LinkedList<SVTTopicMessage>();
      LinkedList<String> failureList = new LinkedList<String>();;
      String failure = null;
      SVTTopicMessage svttopicmessage = null;
      String[] words = null;
      boolean sleep = false;
      boolean pass = true;
      boolean receiverDone = false;
      int incomingqos = 0;
      boolean empty = false;
      int qos = 0;
      String pubId = null;
      boolean process = false;
      String payload = null;
      int s, s3;
      long count = 0;
      long lastcount = 0;
      long fails = 0;
      long lastfails = 0;
      String comment = null;
      long size = 0;

      public void run() {
         if (config.verbose) {
            config.log.println("orderMessage.run() entry");
         }

         // subscriberMutex.lock();
         receiverDone = subscriberIsDone(null, null);
         empty = orderCheckMessageList.isEmpty();
         // subscriberMutex.unlock();

         while (!receiverDone || !empty) {
            // subscriberMutex.lock();
            sleep = true;
//            size = orderCheckMessageList.size();
//            if (size > 0) {
//               sleep = false;
//               // if (config.veryverbose) {
//               // config.log.println("orderMessage verifying " + orderCheckMessageList.size() + " messages.");
//               // }
//               ArrayList<SVTTopicMessage> tmp = orderCheckMessageList;
////                  synchronized (list) {
//                     orderCheckMessageList = orderMessageList;
////                  }
//                     orderMessageList = tmp;
//               
////               for (int i = 0; i < orderMessageList.size(); i++) {
////                  orderMessageList.add(orderMessageList.remove(0));
////               }
//            }
            // subscriberMutex.unlock();

            while (orderCheckMessageList.isEmpty() == false) {
               sleep = false;
               process = false;
               svttopicmessage = orderCheckMessageList.remove(0);
               count++;
               if (config.iotf) {
                  // messagetext = "{\"ts\":" + theTimeStamp + ",\"d\":{\"cTime\":" + theTimeStamp + ", \"sTime\":" + startTimeStamp
                  // + ", \"Pub\":\"" + pClientId + "\" , \"Topic\":\"" + topicName + "\" , \"QoS\":" + gpsQoS + " , \"Retain\":"
                  // + "false" + " , \"Index\":" + this.stats.getCount() + " , \"payload\":\"" + payload + "\"  }}";

                  // 0 1  2               3 4  5    6 7                            8  9   0 11                    12  13    4 15                     16  17  18    19    20    21      2 23          24
                  // {"ts":1433787712036,"d":{"Time":"2015-06-08 13:21:52.036-0500", "Pub":"d:q34fxi:car:c0000016" , "Topic":"iot-2/evt/gps/fmt/json" , "QoS":0 , "Index":0 , "payload":"GPS 620,412"  }}
                  
                  
                  payload = svttopicmessage.getMessageText();
                  words = svttopicmessage.getMessageText().split("[ \":]");

//                  int i = 0;
//                  for (String word:words) {
//                     config.log.println("words["+i+"]:"+word);
//                     i++;
//                  }
                  
//                  s = payload.indexOf("\"Pub\":");
//                  s3 = payload.indexOf(",", s);
//                  pubId = payload.substring(s + 6, s3);
//
//                  previous = publishersIndexTable.get(pubId);
//
//                  s = payload.indexOf("\"QoS\":");
//                  s3 = payload.indexOf(",", s);
//                  incomingqos = Integer.parseInt(payload.substring(s + 6, s3).trim());
//                  qos = Math.min(config.qos, incomingqos);
//
//                  s = payload.indexOf("\"Index\":");
//                  s3 = payload.indexOf(",", s);
//                  incoming = Long.parseLong(payload.substring(s + 8, s3).trim());

                  if ("Pub".equals(words[16])) {
//                     pubId = words[19] + ":" + words[20] + ":" + words[21] + ":" + words[22];
                     pubId = words[22];
                     previous = publishersIndexTable.get(pubId);
                     incomingqos = Integer.parseInt(words[35]);
                     qos = Math.min(config.qos, incomingqos);
                     incoming = Long.parseLong(words[40]);

                     // config.log.println("pubId:  "+pubId+" incomingqos:  "+incomingqos+" incoming  "+incoming);
                     // System.exit(0);

                     process = true;
                  }
               } else {
                  words = svttopicmessage.getMessageText().split(":");
                  if ("ORDER".equals(words[0])) {
                     pubId = words[1];
                     previous = publishersIndexTable.get(pubId);
                     incomingqos = Integer.parseInt(words[2]);
                     qos = Math.min(config.qos, incomingqos);
                     incoming = Long.parseLong(words[3]);
                     process = true;
                  }
               }
               if (process) {
                  if ((qos == 0) && ((previous == null) || (incoming == (previous + 1)))) {
                     publishersIndexTable.put(pubId, incoming);
                  } else if ((qos == 0) && (incoming > previous)) {
                     if (!config.verifyPercentage) {
                        config.log.printErr("WARNING:  for " + pubId + " expected index " + (previous + 1) + " but received " + incoming
                              + "\n");
                     } else {
                        if (comment == null) {
                           comment = pubId;
                        } else {
                           comment = comment + " " + pubId;
                        }
                     }
                     fails++;
                     publishersIndexTable.put(pubId, incoming);
                  } else if ((qos == 1) && ((previous == null) && (incoming == 0))) {
                     publishersIndexTable.put(pubId, incoming);
                  } else if ((qos == 1) && ((previous != null) && ((incoming == previous) || (incoming == (previous + 1))))) {
                     publishersIndexTable.put(pubId, incoming);
                  } else if ((qos == 2) && ((previous == null) && (incoming == 0))) {
                     publishersIndexTable.put(pubId, incoming);
                  } else if ((qos == 2) && ((previous != null) && (incoming == (previous + 1)))) {
                     publishersIndexTable.put(pubId, incoming);
                  } else {
                     publishersIndexTable.put(pubId, incoming);
                     failure = "Order Error from " + pubId + " Qos (sent:" + incomingqos + " recv:" + config.qos + " comb:" + qos + "), "
                           + " index " + incoming + " is incorrect.  Previous index was " + previous;
                     failureList.addLast(failure);
                     config.log.printErr(failure);
                     pass = false;
                     // System.exit(1);
                     // fails++;
                  }
               }
            }
            if (sleep == true) {
               try {
                  Thread.sleep(500);
               } catch (InterruptedException e) {
                  e.printStackTrace();
               }
            }
            // subscriberMutex.lock();
            receiverDone = subscriberIsDone(null, null);
            empty = orderCheckMessageList.isEmpty();
            // subscriberMutex.unlock();

            if ((config.verifyPercentage) && (fails > lastfails)) {
               config.log.println(comment);
               config.log.printTm("" + (fails - lastfails) + " failures in " + (count - lastcount) + " messages (" + fails + "/" + count
                     + " = " + ((fails * 100d) / (count * 1d)) + "%)\n");
               lastfails = fails;
               lastcount = count;
               comment = null;
            }

         }

         if ((pass == true) && (config.qos0Min <= qosCount(0, null)) && (config.qos0Max >= qosCount(0, null))
               && (config.qos1Min <= qosCount(1, null)) && (config.qos2 == qosCount(2, null))) {
            config.log.println("Message Order Pass");
         } else {
            config.log.println("\n");
            config.log.println("Configured Qos0 Min: " + config.qos0Min);
            config.log.println("Configured Qos0 Max: " + config.qos0Max);
            config.log.println("Configured Qos1 Min: " + config.qos1Min);
            config.log.println("Configured qos2: " + config.qos2);
            config.log.println("Qos0 messages received: " + qosCount(0, null));
            config.log.println("Qos1 messages received: " + qosCount(1, null));
            config.log.println("Qos2 messages received: " + qosCount(2, null));
            config.log.println("\n");
            config.log.println("Failure Report");
            config.log.println("--------------");
            config.log.println("-  Pass: " + pass);
            config.log.println("\n");
            config.log.println("-  Number of senders: " + publishersIndexTable.size());
            if (publishersIndexTable.size() > 0) {
               config.log.println("-  List of senders:");
               for (String sender : publishersIndexTable.keySet()) {
                  config.log.println("-  " + sender);
               }
            }
            config.log.println("\n");
            config.log.println("-  Number of failures:" + failureList.size());
            if (failureList.size() > 0) {
               config.log.println("-  List of failures:");
               while (failureList.isEmpty() == false) {
                  config.log.println("-  " + failureList.removeFirst());
               }
            }
            config.log.println("--------------\n");
         }

         if (config.verbose) {
            config.log.println("orderMessage.run() exit");
         }
         return;
      }
   }

   class resendMessage implements Runnable {
      LinkedList<SVTTopicMessage> resendList = new LinkedList<SVTTopicMessage>();
      String entrytext = null;
      String outgoing_topic = null;
      SVTTopicMessage entry = null;
      String[] words = null;
      boolean sleep = false;
      ATBackend config = null;
      int qos = 2;

      // String clientId = null;

      resendMessage(ATBackend config) {
         this.config = new ATBackend(config);
         if (config.subscriberID != null)
            this.config.clientID = config.subscriberID;
         if (config.subscriberURI != null)
            this.config.serverURI = config.subscriberURI;
         if (config.subscriberTopic != null)
            this.config.topicName = config.subscriberTopic;

      }

      public void run() {
         SVTTopicMessage tm = null;
         // MqttSamplePublish resendPublisher = new MqttSamplePublish(config);
         // try {
         // resendPublisher.doConnect();
         // long startTime = System.currentTimeMillis();
         MqttMessage resendMessage = null;

         // subscriberMutex.lock();
         boolean empty = resendMessageList.isEmpty();
         boolean subscriberDone = subscriberIsDone(null, null);
         // subscriberMutex.unlock();

         while ((subscriberDone == false) || (empty == false)) {
            // if (config.debug) {
            // config.log.println("subscriberDone:  " + subscriberDone);
            // config.log.println("receivedMessageList.isEmpty:  " + empty);
            // }
            // subscriberMutex.lock();
            while (resendMessageList.isEmpty() == false) {
               // synchronized (lock) {
               tm = resendMessageList.remove(0);
               // }
               if (config.veryverbose) {
                  config.log.println("Moving payload:  " + tm.getMessageText() + " to resend list.");
               }
               resendList.addLast(tm);
            }
            // subscriberMutex.unlock();

            sleep = resendList.isEmpty();
            while (resendList.isEmpty() == false) {
               entry = resendList.removeFirst();

               if (config.iotf) {
                  config.log.println("topic: " + entry.getTopic());
                  entrytext = entry.getMessageText();
                  String[] token = entry.getTopic().split("[/]+");
                  // iot-2/type/${type}/id/${id}/evt/${event}/fmt/${fmt}
                  String type = token[2];
                  String id = token[4];
                  String cmd = null;
                  String destType = null;
                  String destID = null;
                  if (token[6].contains(":")) {
                     String[] evt = token[6].split("[:]+");
                     cmd = evt[0];
                     destType = evt[1];
                     destID = evt[2];
                     outgoing_topic = "iot-2/type/" + destType + "/id/" + destID + "/cmd/" + cmd + ":" + type + ":" + id + "/fmt/json";
                  } else {
                     cmd = token[6];

                     int s = entrytext.indexOf("From");
                     int s2 = entrytext.indexOf(":", s);
                     int s3 = entrytext.indexOf(",", s);
                     String from = entrytext.substring(s2 + 1, s3);
                     String[] req = from.split("[/]+");
                     type = req[1];
                     id = req[3];

                     s = entrytext.indexOf("To");
                     s2 = entrytext.indexOf(":", s);
                     s3 = entrytext.indexOf(",", s);
                     String to = entrytext.substring(s2 + 1, s3);

                     outgoing_topic = "iot-2" + to + "/cmd/" + cmd + ":" + type + ":" + id + "/fmt/json";

                  }

                  qos = 1;
               } else {
                  entrytext = entry.getMessageText();
                  String[] token = entry.getTopic().split("[/]+");
                  outgoing_topic = "/" + token[5] + "/" + token[6] + "/" + token[1] + "/" + token[2] + "/" + token[3] + "/" + token[4];
                  qos = 1;
               }
               config.log.println("publish topic: " + outgoing_topic);

               resendMessage = new MqttMessage();
               resendMessage.setPayload(entrytext.getBytes());
               resendMessage.setQos(qos);
               resendMessage.setRetained(false);

               // try {
               send(outgoing_topic, resendMessage);
               // resendPublisher.send(resendMessage, outgoing_topic,
               // config.time);
               // } catch (MqttPersistenceException e) {
               // e.printStackTrace();
               // } catch (MqttException e) {
               // e.printStackTrace();
               // }

            }
            if (sleep == true) {
               try {
                  Thread.sleep(2000);
               } catch (InterruptedException e) {
                  e.printStackTrace();
               }
            }

            // subscriberMutex.lock();
            empty = resendMessageList.isEmpty();
            subscriberDone = subscriberIsDone(null, null);
            if (empty && subscriberDone) {
               int sleepCount = 0;
               while ((empty) && (sleepCount < 20)) {
                  sleepCount++;
                  sleep(2000);
                  empty = resendMessageList.isEmpty();
               }
            }
            // subscriberMutex.unlock();

         }

         while (subscriberIsDone(null, null) == false) {
            try {
               Thread.sleep(500);
            } catch (InterruptedException e) {
               e.printStackTrace();
            }
         }

         isResendDone(true,
               "ResendMessage.run() is setting isResendDone(true) because receiverQueue is empy, isDone is true, and resendList is empty (???)");

         if (config.verbose) {
            config.log.println("resendMessage.run() calling resendPublisher.publisher_disconnect()");
         }
         // resendPublisher.publisher_disconnect();

         if (config.verbose) {
            config.log.println("resendMessage.run() calling return.");
         }
         return;
      }
   }

   Byte[] pubLock = new Byte[1];

   void send(String topicName, MqttMessage message) {
      boolean sent = false;
      while (sent == false) {
         try {
            if (config.veryverbose) {
               config.log.println(" publishing " + new String(message.getPayload()) + " to topic " + topicName + ", isRetained set to "
                     + message.isRetained());
            }
            IMqttDeliveryToken token = null;
            synchronized (pubLock) {
            }
            token = client.publish(topicName, message);

            if (token == null) {
               System.err.println("Publish returned null token");
               System.exit(1);
            } else if (config.veryverbose) {
               token.setUserContext(message);
            }
            sent = true;
         } catch (MqttException ex) {
            config.log.println(" Failed to publish " + new String(message.getPayload()) + " due to MqttException " + ex.getMessage()
                  + ", reason code " + ex.getReasonCode());
            if (32201 == ex.getReasonCode() /* no new message IDs available */) {
               sleep(500);
            } else if (32202 == ex.getReasonCode() /* too many publishers */) {
            } else {
               sleep(500);
            }
         } catch (Exception ex) {
            config.log.println(" Failed to publish " + new String(message.getPayload()) + " due to Exception" + ex.getMessage());
            sleep(500);
         }
      }
      if (config.verbose)
         config.log.println("send topic: " + topicName + "  Message: " + new String(message.getPayload()) + "\n");

   }

   @Override
   public void deliveryComplete(IMqttDeliveryToken arg0) {
   }

   class spray implements Runnable {
//      LinkedList<SVTTopicMessage> resendList = new LinkedList<SVTTopicMessage>();
//      String entrytext = null;
//      String outgoing_topic = null;
//      SVTTopicMessage entry = null;
//      String[] words = null;
//      boolean sleep = false;
      ATBackend config = null;
      int qos = 1;

      // String clientId = null;

      spray(ATBackend config) {
         this.config = new ATBackend(config);
         if (config.subscriberID != null)
            this.config.clientID = config.subscriberID;
         if (config.subscriberURI != null)
            this.config.serverURI = config.subscriberURI;
         if (config.subscriberTopic != null)
            this.config.topicName = config.subscriberTopic;

      }

      public void run() {
//         SVTTopicMessage tm = null;
         // MqttSamplePublish resendPublisher = new MqttSamplePublish(config);
         // try {
         // resendPublisher.doConnect();
         // long startTime = System.currentTimeMillis();
         MqttMessage message = null;
         message = new MqttMessage();
         message.setPayload("UNLOCK".getBytes());
         message.setQos(qos);
         message.setRetained(false);

         Random randomGenerator = new Random(System.currentTimeMillis());

         // subscriberMutex.lock();
         boolean subscriberDone = subscriberIsDone(null, null);
         // subscriberMutex.unlock();


         while (subscriberDone == false) {
            int randomInt = randomGenerator.nextInt(1024 * 1024);
//            outgoing_topic = String.format("/MOB/m%1$08d/APP/%2$s",randomInt,config.app);
//            config.log.println("publish topic: " + outgoing_topic);
//          config.log.println("randomInt: " + randomInt);

            send(config.topic[randomInt], message);

               try {
                  Thread.sleep(config.spraySleep);
               } catch (InterruptedException e) {
                  e.printStackTrace();
               }
         }

         if (config.verbose) {
            config.log.println("spray.run() calling return.");
         }
         return;
      }
   }

}
