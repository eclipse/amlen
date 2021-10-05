package svt.scale.monitor;

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

   import java.sql.Timestamp;
import java.util.Collection;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.Properties;
import java.util.Stack;
import java.util.TreeSet;
import java.util.Vector;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttSecurityException;
import org.eclipse.paho.client.mqttv3.persist.MqttDefaultFilePersistence;

  
   /**
    * Class to encapsulate the ISM topic subscribe logic.
    * 
    * This class contains the doSubscribe method for subscribing to ISM topic messages. It also defines the necessary MQTT
    * callbacks for asynchronous subscriptions.
    * 
    */
   public class ATMonitorEngine implements MqttCallback {
      ATMonitor config = null;
      boolean done = false;
      long receivedcount = 0;
      Hashtable<String, String> clientstack = new Hashtable<String, String>();
      Hashtable<String, Long> clienttime = new Hashtable<String, Long>();
      private Lock smutex = new ReentrantLock();
      private Lock omutex = new ReentrantLock();
      Stack<byte[]> stack = new Stack<byte[]>();
      Stack<byte[]> ostack = new Stack<byte[]>();
      MqttClient client = null;
      MqttConnectOptions options = null;
      Long batch = 100000L;
      long startTime = 0;

      byte[] ratebytes = new String("Rate").getBytes();

      /**
       * Callback invoked when the MQTT connection is lost
       * 
       * @param e
       *           exception returned from broken connection
       * 
       */
      public void connectionLost(Throwable e) {
         config.log.println("Lost connection to " + config.serverURI + ".  " + e.getMessage());
         boolean connected = false;
         while (connected == false) {
            try {
               client.connect(options);
               connected = true;
            } catch (MqttSecurityException e1) {
               e1.printStackTrace();
            } catch (MqttException e1) {
               e1.printStackTrace();
            }
            if (connected == false) {
               try {
                  Thread.sleep(60000);
               } catch (InterruptedException e1) {
                  e1.printStackTrace();
               }
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
      synchronized private boolean isDone(Boolean setDone) {
         if (setDone != null) {
            this.done = setDone;
         }
         return this.done;
      }

      /**
       * subscribe to messages published on the topic name
       * 
       * @param config
       *           MqttSample instance containing configuration settings
       * 
       * @throws MqttException
       */
      public void doSubscribe(ATMonitor config) throws MqttException {

         this.config = config;

         MqttDefaultFilePersistence dataStore = null;
         if (config.persistence)
            dataStore = new MqttDefaultFilePersistence(config.dataStoreDir);

         client = new MqttClient(config.serverURI, config.clientID, dataStore);
         client.setCallback(this);

         options = new MqttConnectOptions();

         // set CleanSession true to automatically remove server data associated
         // with the client id at
         // disconnect. In this sample the clientid is based on a random number
         // and not typically
         // reused, therefore the release of client's server side data on
         // disconnect is appropriate.
         options.setCleanSession(config.cleanSession);
         if (config.password != null) {
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

            String trustStore = System.getProperty("com.ibm.ssl.trusStore");
            if (trustStore == null)
               trustStore = System.getProperty("javax.net.ssl.trustStore");

            String trustStorePassword = System.getProperty("com.ibm.ssl.trusStorePassword");
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
         options.setKeepAliveInterval(86400);
         options.setConnectionTimeout(86400);

         client.connect(options);

         // Subscribe to the topic. messageArrived() callback invoked when
         // message received.
         startTime = System.currentTimeMillis();
         if (config.monitor == false)
            client.subscribe(config.topicName, config.qos);
         else
            client.subscribe("/svt/#", config.qos);
         config.log.println("Client '" + client.getClientId() + "' subscribed to topic: '" + config.topicName + "' on " + client.getServerURI()
                  + " with QOS " + config.qos + ".");

         Thread batchMessage = new Thread(new batchMessage());
         batchMessage.start();

         Thread orderMessage = null;
         if (config.order == true) {
            orderMessage = new Thread(new orderMessage());
            orderMessage.start();
         }

         // wait for messages to arrive before disconnecting
         try {
            long last = System.currentTimeMillis();
            long now = last;
            boolean msg = true;
            while (!isDone(null)) {
               if (config.monitor == true) {
                  now = System.currentTimeMillis();
                  msg = msg || processStack(now);
                  if ((msg == true) && ((now - last) > 10000)) {
                     last = now;
                     config.log.println(getStatText());
                     msg = false;
                  } else {
                     Thread.sleep(1000);
                  }
               } else {
                  Thread.sleep(500);
               }
            }
         } catch (InterruptedException e) {
            e.printStackTrace();
         }

         if (config.order) {
            try {
               orderMessage.join();
            } catch (InterruptedException e) {
               e.printStackTrace();
            }
         }
         // Disconnect the client
         client.unsubscribe(config.topicName);
         client.disconnect();

         if (config.verbose)
            config.log.println("Subscriber disconnected.");

         return;
      }

      public boolean processStack(long now) throws InterruptedException {
         String payload = null;
         boolean msg = false;
         smutex.lock();
         while (stack.empty() == false) {
            payload = new String(stack.pop());
            String name = payload.substring(0, payload.indexOf(':'));
            clientstack.put(name, payload);
            clienttime.put(name, now);
            // config.log.println("clientstack.put("+payload.substring(0,payload.indexOf(':'))+","+ payload+");");
            msg = true;
         }
         smutex.unlock();

         Vector<String> remove = new Vector<String>();
         for (String name : clienttime.keySet()) {
            if (now - clienttime.get(name) > 180000) {
               java.util.Date date = new java.util.Date(clienttime.get(name));
               String time = new Timestamp(date.getTime()).toString();
               config.log.println(name + " last reported status at " + time);
               remove.add(name);
            }
         }
         for (String name : remove) {
            clienttime.remove(name);
            clientstack.remove(name);
         }

         return msg;
      }

      public String getStatText() {
         String[] words = null;
         String[] host = null;
         long cars = 0;
         long car = 0;
         Long prev = 0L;
         Double rprev = 0D;
         long count = 0;
         double delta_ms = 0;
         double rate = 0;
         double rate_sec = 0;
         String rtext = "waiting for data...";
         Collection<String> values = clientstack.values();
         TreeSet<String> names = new TreeSet<String>();
         HashMap<String, Long> times = new HashMap<String, Long>();
         HashMap<String, Long> totals = new HashMap<String, Long>();
         HashMap<String, Double> rates = new HashMap<String, Double>();
         long failed = 0;
         String note = null;
         // config.log.println("\nSnapshot:");
         for (String text : values) {
            // text = clientId + ":" + cars + ":" + count + ":" + delta_ms + ":" + rate_sec;
            words = text.split(":");
            if (words.length >= 5) {
               host = words[0].split("_");
               names.add(host[0]);
               car = Long.parseLong(words[1]);
               cars += car;
               prev = totals.get(host[0]);
               totals.put(host[0], ((prev == null) ? 0 : prev) + car);
               count += Long.parseLong(words[2]);
               delta_ms += Double.parseDouble(words[3]);
               rate = Double.parseDouble(words[4]);
               rprev = rates.get(host[0]);
               rates.put(host[0], ((rprev == null) ? 0 : rprev) +rate);
               times.put(host[0], System.currentTimeMillis());
               rate_sec += rate;
               if (words.length >= 7) {
                  failed += Long.parseLong(words[5]);
               }
               if (words.length >= 7) {
                  note = words[6];
               }
               rtext = String.format("%,d%s%,.0f%s\n", cars, " publishers are currently sending at a total rate of ", rate_sec, " msgs/sec");
               if (failed > 0)
                  rtext += " and " + failed + " publishers unable to connect.";
               if (rate < 5)
                  config.log.println(String.format("  slow publisher:  %-20s %s %,.2f", host[0], host[1], rate));
               if (note != null) {
                  config.log.println("Publisher at " + host[0] + " " + host[1] + " reported: " + note);
               }
            } else {
               config.log.println("Ignoring invalid status message:  " + text);
            }
         }

         int c = 0;
         if (names != null) {
            for (String name : names) {
               if ((System.currentTimeMillis() - times.get(name)) > 20000){ rates.remove(name);totals.remove(name);}
               config.log.println(String.format("%8s  %,5d clients\t  %,.0f msg/sec", name, totals.get(name), rates.get(name)));
//               if (c++ > 3) {
//                  config.log.println();
//                  c = 0;
//               }
            }
            config.log.println();
         }
         // config.log.println("");
         return rtext;
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
//    @Override
//    public void messageArrived(String arg0, MqttMessage arg1) throws Exception {
//       // TODO Auto-generated method stub
//       
//    }

      public void messageArrived(String topic, MqttMessage receivedMessage) throws Exception {
         // increment count of received messages
         receivedcount++;

         if (config.veryverbose) {
            config.log.println("Received message on topic '" + topic + "' " + new String(receivedMessage.getPayload()));
         }

         // NOTE: if config.hours is set to -1 then isDone(true) will never be called and subscriber will run forever.
         if (receivedcount == 1) {
            config.log.println("Receiving messages on topic '" + topic + "'");
         } else if ((config.hours > 0) && (System.currentTimeMillis() - startTime >= (config.hours * 360000))) {
            config.log.println("Received " + config.count + " messages in " + (System.currentTimeMillis() - startTime) / 360000 + "hours");
            isDone(true);
         } else if ((config.hours == 0) && (receivedcount == config.count)) {
            config.log.println("Received " + config.count + " messages.");
            isDone(true);
         }

         if (config.monitor == true) {
            smutex.lock();
            stack.push(receivedMessage.getPayload().clone());
            smutex.unlock();
         }

         if (config.order == true) {
            omutex.lock();
            ostack.push(receivedMessage.getPayload().clone());
            omutex.unlock();
         }

         if ((config.verbose == true) && (receivedcount % batch == 0)) {
            synchronized (batch) {
               batch.notify();
            }
         }

      }

      class batchMessage implements Runnable {
         long now = 0;
         long prev = 0;
         long rate = 0;

         public void run() {
            while (isDone(null) == false) {
               synchronized (batch) {
                  try {
                     batch.wait();
                  } catch (InterruptedException e) {
                  }
               }
               now = System.currentTimeMillis();

               if (prev > 0) {
                  rate = (batch * 1000) / (now - prev);
                  config.log.println(" Received 100,000 msgs at the rate " + rate + " msg/sec");
               }
               prev = now;
            }
            return;
         }
      }

      class orderMessage implements Runnable {
         Hashtable<String, Long> table = new Hashtable<String, Long>();
         Long previous = 0L;
         Long incoming = 0L;
         Stack<byte[]> mystack = new Stack<byte[]>();
         String entrytext = null;
         byte[] entry = null;
         String[] words = null;
         boolean sleep = false;
         boolean pass = true;

         public void run() {
            while (isDone(null) == false) {
               omutex.lock();
               sleep = ostack.isEmpty();
               while (ostack.isEmpty() == false) {
                  mystack.push(ostack.pop());
               }
               omutex.unlock();

               while (mystack.isEmpty() == false) {
                  entry = mystack.pop();
                  entrytext = new String(entry);
                  words = entrytext.split(":");
                  if ("ORDER".equals(words[0])) {
                     previous = table.get(words[1]);
                     incoming = Long.parseLong(words[2]);
                     if ((config.qos == 0) && ((previous == null) || (incoming > previous))) {
                        table.put(words[1], incoming);
                     } else if ((config.qos == 1) && ((previous == null) || (incoming >= previous))) {
                        table.put(words[1], incoming);
                     } else if ((config.qos == 2) && ((previous == null) || (incoming == previous + 1))) {
                        table.put(words[1], incoming);
                     } else {
                        config.log.println("Order Error for Qos " + config.qos + ", " + words[1] + " index " + words[2]
                                 + " is incorrect.  Previous message index was " + previous);
                        pass = false;
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
            }

            if ((table.size() > 0) && (pass == true)) {
               config.log.println("Message Order Pass");
            }

            return;
         }
      }

      public void deliveryComplete(MqttDeliveryToken arg0) {
         // TODO Auto-generated method stub

      }

      @Override
      public void deliveryComplete(IMqttDeliveryToken arg0) {
         // TODO Auto-generated method stub
         
      }


   }


