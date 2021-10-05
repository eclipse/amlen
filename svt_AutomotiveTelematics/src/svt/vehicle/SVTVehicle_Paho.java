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
package svt.vehicle;

import java.util.ArrayList;
import java.util.Properties;
import java.util.Random;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttSecurityException;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import svt.util.SVTConfig.BooleanEnum;
import svt.util.SVTLog;
import svt.util.SVTLog.TTYPE;

public class SVTVehicle_Paho {
   static TTYPE TYPE = TTYPE.PAHO;
   // static String ISM = "ISM2BRIDGE";
   // static String Vehicle = "/CAR/1/#";
   // static String User = "/USER/1";
   int gpsQoS = 0;
   int listenerQoS = 1;

   String ismserver = null;
   String ismserver2 = null;
   Integer ismport = null;
   int guid = (int) (Math.random() * 10000.0);
   String command = null;
   // static String dataStoreDir = System.getProperty("java.io.tmpdir");
   boolean cleanSession = true;
   int keepAlive = 36000;
   int connectionTimeout = 36000;
   boolean ssl = false;
   String password = null;
   String userName = null;
   boolean persistence = false;
   boolean done = false;
   long carId = 0;
   Byte[] pubLock = new Byte[1];
   Byte[] batch = new Byte[1];
   SVTMqttClient vehicle_client = null;

   public SVTVehicle_Paho(String ismserver, String ismserver2, Integer ismport, long carId, int gpsQ0S, boolean ssl, String userName,
         String password, int keepAlive, int connectionTimeout, boolean cleanSession) {
      this.ismserver = ismserver;
      this.ismserver2 = ismserver2;
      this.ismport = ismport;
      this.carId = carId;
      this.gpsQoS = gpsQ0S;
      this.ssl = ssl;
      this.userName = userName;
      this.password = password;
      this.keepAlive = keepAlive;
      this.connectionTimeout = connectionTimeout;
      this.cleanSession = cleanSession;
   }

   synchronized private boolean isDone(Boolean setDone) {
      if (setDone != null) {
         done = setDone;
      }
      return done;
   }

   // public MqttMessage getMessage(String ismserver, String ismserver2, Integer ismport, String topicName, String cClientId)
   // throws MqttException {
   // // MqttDefaultFilePersistence dataStore = new
   // // MqttDefaultFilePersistence(
   // // dataStoreDir + "/" + guid);
   // MqttClientPersistence dataStore = null;
   // String ism = (this.ssl == false ? "tcp" : "ssl") + "://" + ismserver + ":" + ismport;
   // MqttClient client = null;
   //
   //
   // SVTLog.log(TYPE, "ISM server " + ism);
   // client = new MqttClient(ism, cClientId, dataStore);
   //
   // MqttConnectOptions options = new MqttConnectOptions();
   //
   // if (ismserver2 != null) {
   // String ism2 = (this.ssl == false ? "tcp" : "ssl") + "://" + ismserver2
   // + ":" + ismport;
   // String[] list = new String[2];
   // list[0] = ism;
   // list[1] = ism2;
   // options.setServerURIs(list);
   // SVTLog.log(TYPE, "ISM servers " + ism + " " + ism2);
   // } else {
   // SVTLog.log(TYPE, "ISM server " + ism);
   // }
   //
   // // set CleanSession true to automatically remove server data associated
   // // with the client id at
   // // disconnect. In this sample the clientid is based on a random number
   // // and not typically
   // // reused, therefore the release of client's server side data on
   // // disconnect is appropriate.
   // options.setCleanSession(cleanSession);
   // if (userName != null) {
   // options.setUserName(userName);
   // }
   // if (password != null) {
   // options.setPassword(password.toCharArray());
   // }
   // if (ssl) {
   // Properties sslClientProps = new Properties();
   // //sslClientProps.setProperty("com.ibm.ssl.protocol", "SSLv3");
   // sslClientProps.setProperty("com.ibm.ssl.protocol", "TLSv1.2");
   // sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "TLS_RSA_WITH_AES_128_GCM_SHA256");
   //
   // String keyStore = System.getProperty("com.ibm.ssl.keyStore");
   // if (keyStore == null)
   // keyStore = System.getProperty("javax.net.ssl.keyStore");
   //
   // String keyStorePassword = System.getProperty("com.ibm.ssl.keyStorePassword");
   // if (keyStorePassword == null)
   // keyStorePassword = System.getProperty("javax.net.ssl.keyStorePassword");
   //
   // String trustStore = System.getProperty("com.ibm.ssl.trustStore");
   // if (trustStore == null)
   // trustStore = System.getProperty("javax.net.ssl.trustStore");
   //
   // String trustStorePassword = System.getProperty("com.ibm.ssl.trustStorePassword");
   // if (trustStorePassword == null)
   // trustStorePassword = System.getProperty("javax.net.ssl.trustStorePassword");
   //
   // if (keyStore != null)
   // sslClientProps.setProperty("com.ibm.ssl.keyStore", keyStore);
   // if (keyStorePassword != null)
   // sslClientProps.setProperty("com.ibm.ssl.keyStorePassword", keyStorePassword);
   // if (trustStore != null)
   // sslClientProps.setProperty("com.ibm.ssl.trustStore", trustStore);
   // if (trustStorePassword != null)
   // sslClientProps.setProperty("com.ibm.ssl.trustStorePassword", trustStorePassword);
   //
   // options.setSSLProperties(sslClientProps);
   // }
   // options.setKeepAliveInterval(keepAlive);
   // options.setConnectionTimeout(connectionTimeout);
   // ArrayList<TopicMessage> stack = new ArrayList<TopicMessage>();
   //
   // SVTVehicle_Callback callback = new SVTVehicle_Callback(client, options, stack);
   // client.setCallback(callback);
   //
   // int i = 0;
   // boolean connected = false;
   // SVTLog.logtry(TYPE, "connect");
   // while (connected == false) {
   // try {
   // client.connect(options);
   // connected = client.isConnected();
   // if (connected == false) {
   // throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
   // }
   // } catch (MqttException e) {
   // if (i++ < 100) {
   // try {
   // Thread.sleep(1000 + (int) (Math.random() * 9000.0));
   // } catch (InterruptedException e2) {
   // SVTLog.logex(TYPE, "sleep", e);
   // } } else {
   // SVTLog.logex(TYPE, "connect", e);
   // throw e;
   // }
   // SVTLog.logretry(TYPE, "connect", e, i);
   // }
   // }
   //
   // SVTLog.logdone(TYPE, "connect");
   //
   // // Subscribe to the topic. messageArrived() callback invoked when
   // // message received.
   // client.subscribe(topicName, qos);
   // SVTLog.log(TYPE, "Subscribed to topic: '" + topicName + "'.");
   //
   // TopicMessage topicMessage = null;
   // // wait for messages to arrive before disconnecting
   // try {
   // while (callback.stacksize() < 1) {
   // Thread.sleep(500);
   // }
   // topicMessage = callback.pop();
   // // isDone(false);
   // } catch (InterruptedException e) {
   // SVTLog.logex(TYPE, "sleep", e);
   // }
   //
   // // Disconnect the client
   // client.unsubscribe(topicName);
   // client.disconnect();
   //
   // return topicMessage.message;
   // }

   public MqttTopic getTopic(String pClientId, String topicName) throws Exception {
      MqttClient client = getClient(pClientId);
      return client.getTopic(topicName);
   }

   public MqttClient getClient(String pClientId) throws Exception {
      SVTMqttClient client = getSVTClient(pClientId);
      return client.client;
   }

   public SVTMqttClient getSVTClient(String pClientId) throws Exception {
      MqttClient client = null;
      SVTLog.logtry(TYPE, "client");
      int i = 0;
      String ism = (this.ssl == false ? "tcp" : "ssl") + "://" + ismserver + ":" + ismport;

      while (client == null) {
         try {
            client = new MqttClient(ism, pClientId, null);
         } catch (MqttException e) {
            if (i++ >= 100) {
               SVTLog.logex(TYPE, "client", e);
               throw e;
            }
            SVTLog.logretry(TYPE, "client", e, i);
         }
         if (client == null) {
            try {
               Thread.sleep(1000 + (int) (Math.random() * 9000.0));
            } catch (InterruptedException e) {
               SVTLog.logex(TYPE, "sleep", e);
            }
         }
      }
      SVTLog.logdone(TYPE, "client");

      MqttConnectOptions options = new MqttConnectOptions();

      if (ismserver2 != null) {
         String ism2 = (this.ssl == false ? "tcp" : "ssl") + "://" + ismserver2 + ":" + ismport;
         String[] list = new String[2];
         list[0] = ism;
         list[1] = ism2;
         options.setServerURIs(list);
      }

      // options.setCleanSession(cleanSession);

      if (userName != null) {
         options.setUserName(userName);
      }
      if (password != null) {
         options.setPassword(password.toCharArray());
      }
      if (ssl) {
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

         options.setSSLProperties(sslClientProps);
      }
      options.setKeepAliveInterval(keepAlive);
      options.setConnectionTimeout(connectionTimeout);
      ArrayList<TopicMessage> stack = new ArrayList<TopicMessage>();

      SVTVehicle_Callback callback = new SVTVehicle_Callback(client, options, stack);
      client.setCallback(callback);

      if (cleanSession) {
         options.setCleanSession(true);
         subscriber_connect(client, options);
         subscriber_disconnect(client);
      }
      options.setCleanSession(false);
      subscriber_connect(client, options);

      return new SVTMqttClient(client, options, callback, stack);

   }

   void subscriber_connect(MqttClient client, MqttConnectOptions options) throws Exception {
      int i = 0;
      boolean connected = false;
      SVTLog.logtry(TYPE, "connect");
      while (connected == false) {
         try {
            client.connect(options);
            Thread.sleep(100);
            connected = client.isConnected();
            if (connected == false) {
               throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
            }
         } catch (Exception e) {
            // Catching all exceptions for now. May need to separate logic for MqttException
            if (i++ >= 100) {
               SVTLog.logex(TYPE, "connect", e);
               throw e;
            }

            try {
               Thread.sleep(1000 + (int) (Math.random() * 9000.0));
            } catch (InterruptedException e2) {
               SVTLog.logex(TYPE, "sleep", e);
            }
            SVTLog.logretry(TYPE, "connect", e, i);
         }
      }

      SVTLog.logdone(TYPE, "connect");

   }

   public void sendMessage(SVTMqttClient svtclient, String payload, String topicName) throws MqttException {
      // long tid = Thread.currentThread().getId();
      // String store = dataStoreDir + "/svt_" + pClientId;
      // System.out.println("mqtt(" + tid + ") dataStore: " + store);
      // MqttDefaultFilePersistence dataStore = new
      // MqttDefaultFilePersistence(store);
      int i = 0;

      // MqttConnectOptions options = new MqttConnectOptions();
      //
      // // set CleanSession true to automatically remove server data associated
      // // with the client id at disconnect. In this sample the clientid is
      // // based on a random number and not typically
      // // reused, therefore the release of client's server side data on
      // // disconnect is appropriate.
      // options.setCleanSession(cleanSession);
      // if (userName != null) {
      // options.setUserName(userName);
      // }
      // if (password != null) {
      // options.setPassword(password.toCharArray());
      // }
      // if (ssl) {
      // Properties sslClientProps = new Properties();
      // // sslClientProps.setProperty("com.ibm.ssl.protocol", "SSLv3");
      // // sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "SSL_RSA_WITH_AES_128_CBC_SHA");
      // sslClientProps.setProperty("com.ibm.ssl.protocol", "TLSv1.2");
      // sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "TLS_RSA_WITH_AES_128_GCM_SHA256");
      //
      // String keyStore = System.getProperty("com.ibm.ssl.keyStore");
      // if (keyStore == null)
      // keyStore = System.getProperty("javax.net.ssl.keyStore");
      //
      // String keyStorePassword = System.getProperty("com.ibm.ssl.keyStorePassword");
      // if (keyStorePassword == null)
      // keyStorePassword = System.getProperty("javax.net.ssl.keyStorePassword");
      //
      // String trustStore = System.getProperty("com.ibm.ssl.trustStore");
      // if (trustStore == null)
      // trustStore = System.getProperty("javax.net.ssl.trustStore");
      //
      // String trustStorePassword = System.getProperty("com.ibm.ssl.trustStorePassword");
      // if (trustStorePassword == null)
      // trustStorePassword = System.getProperty("javax.net.ssl.trustStorePassword");
      //
      // if (keyStore != null)
      // sslClientProps.setProperty("com.ibm.ssl.keyStore", keyStore);
      // if (keyStorePassword != null)
      // sslClientProps.setProperty("com.ibm.ssl.keyStorePassword", keyStorePassword);
      // if (trustStore != null)
      // sslClientProps.setProperty("com.ibm.ssl.trustStore", trustStore);
      // if (trustStorePassword != null)
      // sslClientProps.setProperty("com.ibm.ssl.trustStorePassword", trustStorePassword);
      //
      // options.setSSLProperties(sslClientProps);
      // }
      // options.setKeepAliveInterval(36000);

      i = 0;
      boolean connected = false;
      SVTLog.logtry(TYPE, "connect");
      while (connected == false) {
         try {
            svtclient.client.connect(svtclient.options);
            connected = svtclient.client.isConnected();
            if (connected == false) {
               throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
            }
         } catch (MqttException e) {
            if (i++ >= 100) {
               SVTLog.logex(TYPE, "connect", e);
               throw e;
            }

            try {
               Thread.sleep(1000 + (int) (Math.random() * 9000.0));
            } catch (InterruptedException e2) {
               SVTLog.logex(TYPE, "sleep", e);
            }
            SVTLog.logretry(TYPE, "connect", e, i);
         }
      }

      SVTLog.logdone(TYPE, "connect");

      // Get an instance of the topic
      MqttTopic topic = svtclient.client.getTopic(topicName);

      // Construct the message to publish
      MqttMessage message = new MqttMessage(payload.getBytes());
      message.setQos(gpsQoS);

      // The default retained setting is false.
      if (persistence)
         message.setRetained(true);

      // Publish the message
      // System.out.print(" publish(" + tid + ")");
      SVTLog.logtry(TYPE, "publish[" + payload + "]");
      MqttDeliveryToken token = null;
      i = 0;
      while (token == null) {
         try {
            token = topic.publish(message);
            if (token == null) {
               throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
            }
         } catch (MqttException e) {
            if (i++ >= 100) {
               SVTLog.logex(TYPE, "publish", e);
               throw e;
            }

            try {
               Thread.sleep(1000 + (int) (Math.random() * 9000.0));
            } catch (InterruptedException e2) {
               SVTLog.logex(TYPE, "sleep", e);
            }
            SVTLog.logretry(TYPE, "publish", e, i);
         }
      }
      SVTLog.logdone(TYPE, "publish", "");
      // System.out.print(". ");

      // Wait until the message has been delivered to the server
      if (gpsQoS > 0) {
         // System.out.print(" wait(" + tid + ")");
         token.waitForCompletion();
         // System.out.print(". ");
      }

      // Disconnect the client
      // System.out.print(" disconnect(" + tid + ")");

      subscriber_disconnect(svtclient.client);

      if (cleanSession) {
         svtclient.options.setCleanSession(true);
         try {
            subscriber_connect(svtclient.client, svtclient.options);
         } catch (Exception e) {
            SVTLog.logex(TYPE, "exception thrown from subscriber_connect ", e);
         }
         subscriber_disconnect(svtclient.client);
      }
      // System.out.println(". ");

   }

   void subscriber_disconnect(MqttClient client) throws MqttException {

      SVTLog.logtry(TYPE, "disconnect");
      client.disconnect();
      SVTLog.logdone(TYPE, "disconnect");

   }

   public void reconnect(SVTMqttClient svtclient) throws Exception{
//   SVTLog.log3(TYPE, "client "+svtclient.client.getClientId()+" disconnecting...");
   synchronized (pubLock) {
   }
   subscriber_disconnect(svtclient.client);
   subscriber_connect(svtclient.client, svtclient.options);
   SVTLog.log3(TYPE, "client "+svtclient.client.getClientId()+" reconnected.");
   }
   public void sendReplyMessage(SVTMqttClient svtclient) throws Exception {

      TopicMessage reply = svtclient.pop();
      String topic = reply.getTopic();

      if ("disconnect".equals(reply.getProperty("action"))){
         reconnect(svtclient);
      }
      
      
      SVTLog.log(TYPE, "outgoing topic " + topic + " message " + reply.getMessageText());
      MqttTopic replyTopic = svtclient.client.getTopic(topic);
      MqttDeliveryToken token = null;
      int i = 0;

      while (token == null) {
         try {
            synchronized (pubLock) {
            }
            SVTLog.logtry(TYPE, "publish[" + reply.getMessageText() + "]");
            token = replyTopic.publish(reply.getMessage());
            if (token == null) {
               throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
            }
         } catch (MqttException e) {
            // Catching all exceptions. May need to separate logic for MqttException
            if (i++ >= 100) {
               SVTLog.logex(TYPE, "publish", e);
               throw e;
            }

            // if (e.getReasonCode() == MqttException.REASON_CODE_CLIENT_NOT_CONNECTED) {
            // SVTLog.logex(TYPE, "CLIENT_NOT_CONNECTED", e);
            // SVTLog.logex(TYPE, e.getLocalizedMessage(), e);
            // }

            try {
               Thread.sleep(1000 + new Random().nextInt(9000));
            } catch (InterruptedException e2) {
               SVTLog.logex(TYPE, "sleep", e);
            }
            SVTLog.logretry(TYPE, "publish", e, i);

         } catch (Exception e) {
            // Catching all exceptions. May need to separate logic for MqttException
            if (i++ >= 100) {
               SVTLog.logex(TYPE, "publish", e);
               throw e;
            }

            try {
               Thread.sleep(1000 + new Random().nextInt(9000));
            } catch (InterruptedException e2) {
               SVTLog.logex(TYPE, "sleep", e);
            }
            SVTLog.logretry(TYPE, "publish", e, i);
         }
      }
      SVTLog.logdone(TYPE, "publish", "");

   }

   public void sendMessage(MqttTopic topic, String payload, Integer qos, String pClientId) throws Throwable {

      // Construct the message to publish
      MqttMessage message = new MqttMessage(payload.getBytes());
      message.setQos(qos);

      // The default retained setting is false.
      if (persistence)
         message.setRetained(true);

      // Publish the message
      // System.out.print(" publish(" + tid + ")");
      SVTLog.logtry(TYPE, pClientId + ".publish[" + payload + "]");
      MqttDeliveryToken token = null;
      int i = 0;
      while (token == null) {
         try {
            synchronized (pubLock) {
            }
            token = topic.publish(message);
            if (token == null) {
               throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
            }
            // Wait until the message has been delivered to the server
            if (gpsQoS > 0) {
               // System.out.print(" wait(" + tid + ")");
               token.waitForCompletion();
               // System.out.print(". ");
            }
         } catch (MqttException e) {
            // Catching all exceptions. May need to separate logic for MqttException
            if (i++ >= 100) {
               SVTLog.logex(TYPE, "publish", e);
               throw e;
            }

            // if (e.getReasonCode() == MqttException.REASON_CODE_CLIENT_NOT_CONNECTED) {
            // SVTLog.logex(TYPE, "CLIENT_NOT_CONNECTED", e);
            // SVTLog.logex(TYPE, e.getLocalizedMessage(), e);
            // }

            try {
               Thread.sleep(1000 + new Random().nextInt(9000));
            } catch (InterruptedException e2) {
               SVTLog.logex(TYPE, "sleep", e);
            }
            SVTLog.logretry(TYPE, "publish", e, i);

         } catch (Exception e) {
            // Catching all exceptions. May need to separate logic for MqttException
            if (i++ >= 100) {
               SVTLog.logex(TYPE, "publish", e);
               throw e;
            }

            try {
               Thread.sleep(1000 + new Random().nextInt(9000));
            } catch (InterruptedException e2) {
               SVTLog.logex(TYPE, "sleep", e);
            }
            SVTLog.logretry(TYPE, "publish", e, i);
         }
      }
      SVTLog.logdone(TYPE, "publish", "");
      // System.out.print(". ");

   }

   public void sendMessage(MqttTopic topic, MqttMessage message) throws Throwable {

      // Publish the message
      SVTLog.logtry(TYPE, "publish[...]");
      MqttDeliveryToken token = null;
      int i = 0;
      while (token == null) {
         try {
            synchronized (pubLock) {
            }
            token = topic.publish(message);
            if (token == null) {
               throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
            }
         } catch (MqttException e) {
            if (i++ >= 100) {
               SVTLog.logex(TYPE, "publish", e);
               throw e;
            }

            try {
               Thread.sleep(1000 + new Random().nextInt(9000));
            } catch (InterruptedException e2) {
               SVTLog.logex(TYPE, "sleep", e);
            }
            SVTLog.logretry(TYPE, "publish", e, i);
         }
      }
      SVTLog.logdone(TYPE, "publish", "");

      // Wait until the message has been delivered to the server
      if (message.getQos() > 0) {
         // System.out.print(" wait(" + tid + ")");
         token.waitForCompletion();
         // System.out.print(". ");
      }
   }

   public void sendMessage(String topicName, String pClientId, String payload, long n) throws Throwable {
      try {
         // long tid = Thread.currentThread().getId();
         // String store = dataStoreDir + "/svt_" + pClientId;
         // System.out.println("mqtt(" + tid + ") dataStore: " + store);
         // MqttDefaultFilePersistence dataStore = new
         // MqttDefaultFilePersistence(store);
         SVTLog.logtry(TYPE, "client");
         int i = 0;
         MqttClient client = null;
         String ism = (this.ssl == false ? "tcp" : "ssl") + "://" + ismserver + ":" + ismport;
         while (client == null) {
            try {
               client = new MqttClient(ism, pClientId, null);
               // client = new MqttClient("tcp://" + ismserver + ":"
               // + ismport, pClientId);
            } catch (Throwable e) {
               if (i++ < 100) {
                  Thread.sleep(1000 + new Random().nextInt(9000));
                  SVTLog.logretry(TYPE, "client", e, i);
               } else {
                  SVTLog.logex(TYPE, "client", e);
                  throw e;
               }
            }
         }
         SVTLog.logdone(TYPE, "client");

         MqttConnectOptions options = new MqttConnectOptions();

         if (ismserver2 != null) {
            String ism2 = (this.ssl == false ? "tcp" : "ssl") + "://" + ismserver2 + ":" + ismport;
            String[] list = new String[2];
            list[0] = ism;
            list[1] = ism2;
            options.setServerURIs(list);
         }

         // set CleanSession true to automatically remove server data
         // associated with the client id
         // at
         // disconnect. In this sample the clientid is based on a random
         // number and not typically
         // reused, therefore the release of client's server side data on
         // disconnect is appropriate.
         options.setCleanSession(cleanSession);
         if (userName != null) {
            options.setUserName(userName);
         }
         if (password != null) {
            options.setPassword(password.toCharArray());
         }
         if (ssl) {
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

            options.setSSLProperties(sslClientProps);
         }
         options.setKeepAliveInterval(keepAlive);
         options.setConnectionTimeout(connectionTimeout);
         ArrayList<TopicMessage> stack = new ArrayList<TopicMessage>();

         SVTVehicle_Callback callback = new SVTVehicle_Callback(client, options, stack);
         client.setCallback(callback);

         i = 0;
         boolean connected = false;
         SVTLog.logtry(TYPE, "connect");
         while (connected == false) {
            try {
               client.connect(options);
               connected = client.isConnected();
               if (connected == false) {
                  throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
               }
            } catch (MqttException e) {
               if (i++ < 100) {
                  Thread.sleep(1000 + (int) (Math.random() * 9000.0));
                  SVTLog.logretry(TYPE, "connect", e, i);
               } else {
                  SVTLog.logex(TYPE, "connect", e);
                  throw e;
               }
            }
         }
         SVTLog.logdone(TYPE, "connect");

         // Get an instance of the topic
         MqttTopic topic = client.getTopic(topicName);

         // Construct the message to publish
         MqttMessage message = new MqttMessage(payload.getBytes());
         message.setQos(gpsQoS);

         // The default retained setting is false.
         if (persistence)
            message.setRetained(true);

         MqttDeliveryToken token = null;
         i = 0;
         SVTLog.logtry(TYPE, "publish", n);
         while (token == null) {
            try {
               synchronized (pubLock) {
               }
               token = topic.publish(message);
               if (token == null) {
                  throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
               }
            } catch (MqttException e) {
               if (i++ >= 100) {
                  SVTLog.logex(TYPE, "publish", e);
                  throw e;
               }

               try {
                  Thread.sleep(1000 + new Random().nextInt(9000));
               } catch (InterruptedException e2) {
                  SVTLog.logex(TYPE, "sleep", e);
               }
               SVTLog.logretry(TYPE, "publish", e, i);
            }
         }
         SVTLog.logdone(TYPE, "publish", "");

         // Wait until the message has been delivered to the server
         // if (qos > 0) {
         // System.out.print(" wait(" + tid + ")");
         // token.waitForCompletion();
         // System.out.print(". ");
         // }

         // Thread.sleep(1000L);

         i = 0;
         done = false;
         // Disconnect the client
         SVTLog.logtry(TYPE, "disconnect");
         while (done == false) {
            try {
               client.disconnect(100L);
               done = true;
               SVTLog.logdone(TYPE, "disconnect");
            } catch (MqttException e) {
               if (e.getReasonCode() == MqttException.REASON_CODE_CLIENT_ALREADY_DISCONNECTED) {
                  done = true;
               }
               if ((done == false) && (i++ >= 100)) {
                  SVTLog.logex(TYPE, "disconnect", e);
                  throw e;
               } else {
                  SVTLog.logretry(TYPE, "disconnect", e, i);
               }
            } catch (Throwable e) {
               if (i++ < 100) {
                  SVTLog.logretry(TYPE, "disconnect", e, i);
               } else {
                  SVTLog.logex(TYPE, "disconnect", e);
                  throw e;
               }
            }
         }

      } catch (InterruptedException e1) {
         SVTLog.logex(TYPE, "sleep", e1);
         throw e1;
      }
   }

   // public void actOnMessage(String topic, String messageText) throws Throwable {
   // SVTLog.log(TYPE, "incoming topic " + topic);
   //
   // String[] token = topic.split("/");
   // String outgoing_topic = "/" + token[3] + "/" + token[4] + "/" + token[5] + "/" + token[6] + "/" + token[1]
   // + "/" + token[2];
   // SVTLog.log(TYPE, "outgoing topic " + outgoing_topic);
   //
   // if ("UNLOCK".equals(messageText)) {
   // SVTLog.log(TYPE, "Doors unlocked");
   // sendMessage(outgoing_topic, SVTUtil.MyClientId(TYPE), "Doors unlocked", 0);
   // } else if ("LOCK".equals(messageText)) {
   // SVTLog.log(TYPE, "Doors locked");
   // sendMessage(outgoing_topic, SVTUtil.MyClientId(TYPE), "Doors locked", 0);
   // } else if ("GET_DIAGNOSTICS".equals(messageText)) {
   // SVTLog.log(TYPE, "Unsupported feature");
   // }
   // }
   //
   // public void actOnMessage(SVTMqttClient svtclient, TopicMessage alert) throws Throwable {
   // SVTLog.log(TYPE, "incoming topic " + alert.getTopic());
   //
   // String[] token = alert.getTopic().split("/");
   // String outgoing_topic = "/" + token[3] + "/" + token[4] + "/" + token[5] + "/" + token[6] + "/" + token[1]
   // + "/" + token[2];
   // SVTLog.log(TYPE, "outgoing topic " + outgoing_topic);
   //
   // String messageText = alert.getMessageText();
   // if ("UNLOCK".equals(messageText)) {
   // SVTLog.log(TYPE, "Doors unlocked");
   // sendMessage(svtclient, outgoing_topic, "Doors unlocked");
   // } else if ("LOCK".equals(messageText)) {
   // SVTLog.log(TYPE, "Doors locked");
   // sendMessage(svtclient, outgoing_topic, "Doors locked");
   // } else if ("GET_DIAGNOSTICS".equals(messageText)) {
   // SVTLog.log(TYPE, "Unsupported feature");
   // }
   // }

   public class SVTMqttClient {
      public MqttClient client;
      public MqttConnectOptions options;
      public MqttCallback callback;
      ArrayList<TopicMessage> stack = null;

      SVTMqttClient(MqttClient client, MqttConnectOptions options, MqttCallback callback, ArrayList<TopicMessage> stack) {
         this.client = client;
         this.options = options;
         this.callback = callback;
         this.stack = stack;
      }

      public int stacksize() {
         int size = 0;
         synchronized (stack) {
            size = stack.size();
         }
         return size;
      }

      public TopicMessage pop() {
         TopicMessage entry = null;
         synchronized (stack) {
            if (stack.size() > 0)
               entry = stack.remove(0);
         }
         return entry;
      }

   };

   // class SVTVehicle_Listener implements Runnable {
   // Boolean run = new Boolean(true);
   // MqttTopic topic = null;
   // SVTMqttClient svtclient = null;
   //
   // public void run() {
   // boolean r = true;
   // String lClientId = SVTUtil.MyClientId(TYPE, "l");
   // String topicName = "/CAR/" + carId + "/#";
   //
   // TopicMessage alert = null;
   // try {
   // SVTLog.log(TYPE, "SVTVehicle_ListenerThread");
   //
   // svtclient = getSVTClient(lClientId);
   //
   // while (r == true) {
   // alert = null;
   //
   // if (svtclient == null) {
   // svtclient = getSVTClient(lClientId);
   // svtclient.client.subscribe(topicName);
   // }
   //
   // synchronized (batch) {
   // try {
   // batch.wait(20000);
   // } catch (InterruptedException e) {
   // }
   // }
   //
   // alert = ((SVTVehicle_Callback)svtclient.callback).pop();
   //
   // if (alert != null) {
   // SVTLog.log(TYPE, "Alert received:  " + new String(alert.getMessageText()));
   // actOnMessage(svtclient, alert);
   // }
   //
   // synchronized (run) {
   // r = run;
   // }
   // }
   // } catch (Throwable e) {
   // SVTLog.logex(TYPE, "getMessage|actOnMessage", e);
   // }
   // return;
   // }
   //
   // public void stop() {
   // synchronized (run) {
   // run = false;
   // }
   // }
   // }

   // public SVTVehicle_Listener listener = null;
   // Thread listenerThread = null;
   //
   // public void startListener() {
   // synchronized (this) {
   // listener = new SVTVehicle_Listener();
   // listenerThread = new Thread(listener);
   // listenerThread.start();
   // }
   // }
   //
   // public void stopListener() {
   // synchronized (this) {
   // listener.stop();
   //
   // while (listenerThread.isAlive()) {
   // try {
   // listenerThread.join(1000);
   // } catch (InterruptedException e) {
   // e.printStackTrace();
   // }
   // }
   // }
   // }

   // public void startInteractive() {
   // String pClientId = SVTUtil.MyClientId(TYPE, "p");
   // String cClientId = SVTUtil.MyClientId(TYPE, "c");
   // String topicName = "/APP/1/CAR/" + carId + "/CAR/" + carId;
   // MqttMessage message = null;
   // String messageText = null;
   //
   // startListener();
   //
   // BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
   //
   // try {
   // SVTLog.log(TYPE, "commands are exit, get_alerts, send_gps, send_errorcode, update_system:  ");
   // String command = br.readLine();
   // while (!("exit".equalsIgnoreCase(command))) {
   // if ("get_alerts".equalsIgnoreCase(command)) {
   // sendMessage(topicName, pClientId, "QUERY_ALERT", 0);
   // message = getMessage(ismserver, ismserver2, ismport, topicName, cClientId);
   // SVTLog.log(TYPE, "Reply message:  " + message);
   // while (message != null) {
   // messageText = new String(message.getPayload());
   // actOnMessage(topicName, messageText);
   // message = getMessage(ismserver, ismserver2, ismport, topicName, cClientId);
   // SVTLog.log(TYPE, "Reply message:  " + messageText);
   // }
   // } else if ("send_gps".equalsIgnoreCase(command)) {
   // int x = (int) (Math.random() * 10000.0);
   // int y = (int) (Math.random() * 10000.0);
   // sendMessage(topicName, pClientId, "GPS " + x + ", " + y + ")", 0);
   // SVTLog.log(TYPE, "sent message: gps(" + x + ", " + y + ")");
   // message = getMessage(ismserver, ismserver2, ismport, topicName, cClientId);
   // SVTLog.log(TYPE, "Reply message:  " + message);
   // } else if ("send_errorcode".equalsIgnoreCase(command)) {
   // int x = (int) (Math.random() * 10000.0);
   // sendMessage(topicName, pClientId, "RECORD_ERRORCODE " + x + ")", 0);
   // SVTLog.log(TYPE, "sent message: errorcode(" + x + ")");
   // message = getMessage(ismserver, ismserver2, ismport, topicName, cClientId);
   // SVTLog.log(TYPE, "Reply message:  " + new String(message.getPayload()));
   // } else if ("update_system".equalsIgnoreCase(command)) {
   // sendMessage(topicName, pClientId, "SYSTEM_UPDATE", 0);
   // SVTLog.log(TYPE, "sent message: update_system");
   // message = getMessage(ismserver, ismserver2, ismport, topicName, cClientId);
   // SVTLog.log(TYPE, "Reply message:  " + new String(message.getPayload()));
   // } else {
   // SVTLog.log(TYPE, "Did not understand command:  " + command);
   // }
   // SVTLog.log(TYPE, "commands are exit, get_alerts, send_gps, send_errorcode, update_system:  ");
   // command = br.readLine();
   // }
   // } catch (Throwable e) {
   // SVTLog.logex(TYPE, "sendMessage|getMessage", e);
   // }
   //
   // stopListener();
   //
   // }

   // public static void main(String[] args) {
   // String ismserver = null;
   // Integer ismport = null;
   // long carId = 0;
   // int qos = 0;
   // boolean ssl = false;
   // String userName = null;
   // String password = null;
   // int keepAlive = 36000;
   // int connectionTimeout = 36000;
   // boolean cleanSession = true;
   //
   // if (args.length == 6) {
   // ismserver = args[0];
   // ismport = Integer.parseInt(args[1]);
   // carId = Long.parseLong(args[2]);
   // qos = Integer.parseInt(args[3]);
   // ssl = Boolean.parseBoolean(args[4]);
   // userName = args[5];
   // password = args[6];
   // } else {
   // System.out.println("Usage: <ismserver> <ismport> <carId> <qos> <ssl> <userName> <password> ");
   // }
   //
   // SVTVehicle_Paho vehicle = new SVTVehicle_Paho(ismserver, null, ismport, carId, qos, ssl, userName, password, keepAlive,
   // connectionTimeout, cleanSession);
   // // start() method loops until user enters "exit"
   // vehicle.startInteractive();
   //
   // }

   // class ClientCallback {
   // private MqttClient client;
   // private SVTVehicle_Callback callback;
   //
   // public ClientCallback(MqttClient client) {
   // this.client = client;
   // this.callback = new SVTVehicle_Callback();
   // }
   //
   // public MqttClient getClient() {
   // return this.client;
   // }
   //
   // public SVTVehicle_Callback getCallback() {
   // return this.callback;
   // }
   //
   // public String getClientId() {
   // return client.getClientId();
   // }
   //
   // public int getStackSize() {
   // return callback.stacksize();
   // }
   //
   // public TopicMessage pop() {
   // return callback.pop();
   // }
   // }

   class TopicMessage {
      private String topic;
      private MqttMessage message;
      private Properties properties;

      public TopicMessage(String topic, MqttMessage message) {
         this.topic = topic;
         this.message = message;
         this.properties = new Properties();
      }

      public void setProperty(String key, String value) {
         properties.setProperty(key, value);
      }

      public String getProperty(String key) {
         return properties.getProperty(key);
      }

      public String getTopic() {
         return this.topic;
      }

      public MqttMessage getMessage() throws MqttException {
         return message;
      }

      // public String getTopicName() {
      // return topic;
      // }

      public String getMessageText() throws MqttException {
         return new String(message.getPayload());
      }
   }

   class SVTVehicle_Callback implements MqttCallback {
      MqttConnectOptions options = null;
      MqttClient client = null;
      ArrayList<TopicMessage> stack = null;

      SVTVehicle_Callback(MqttClient client, MqttConnectOptions options, ArrayList<TopicMessage> stack) {
         this.client = client;
         this.options = options;
         this.stack = stack;
      }

      public void sleep(long s) {
         try {
            SVTLog.log(TYPE, "sleep " + s);
            Thread.sleep(s);
         } catch (InterruptedException e2) {
            System.exit(1);
         }
      }

      public void connectionLost(Throwable arg0) {
         synchronized (pubLock) {
            while (client.isConnected() == false) {
               SVTLog.logtry(TYPE, "connect");
               try {
                  client.connect(options);
                  SVTLog.logdone(TYPE, "reconnected.");
               } catch (MqttSecurityException e) {
                  SVTLog.logex(TYPE, "reconnect", e);
               } catch (Exception e) {
                  SVTLog.log(TYPE, "reconnect failed " + e.getMessage());
               }
               sleep(500);
            }
         }
      }

      public void deliveryComplete(MqttDeliveryToken arg0) {
         // TODO Auto-generated method stub
      }

      // public void messageArrived(MqttTopic topic, MqttMessage message) throws Exception {
      // }

      public void push(TopicMessage tm) {
         synchronized (stack) {
            stack.add(tm);
         }
      }

      @Override
      public void deliveryComplete(IMqttDeliveryToken imqttdeliverytoken) {
         // TODO Auto-generated method stub

      }

      // @Override
      // public void messageArrived(String s, MqttMessage mqttmessage) throws Exception {
      // synchronized (stack) {
      // stack.push(new TopicMessage(s, mqttmessage));
      // }
      // synchronized (batch) {
      // batch.notify();
      // }
      // }

      @Override
      public void messageArrived(String requestTopic, MqttMessage mqttmessage) throws Exception {
         String request = null;
         SVTLog.log3(TYPE, "messageArrived on topic " + requestTopic);
         String action = null;

         String[] token = requestTopic.split("/");
         String outgoing_topic = null;
         String entrytext = new String(mqttmessage.getPayload());
         if (BooleanEnum.iotf.value == false) {
            outgoing_topic = "/" + token[3] + "/" + token[4] + "/" + token[5] + "/" + token[6] + "/" + token[1] + "/" + token[2];
            request = "ping";
            action = "send";
         } else {
            // iot-2/cmd/${event}/fmt/${fmt}
            String cmd = null;
            String destType = null;
            String destID = null;
            if (token[2].contains(":")) {
               String[] evt = token[2].split("[:]+");
               cmd = evt[0];
               destType = evt[1];
               destID = evt[2];
            } else {
               cmd = token[6];

               // need to fix this code to handle messages from user to car and from car to user
//               int s = entrytext.indexOf("From");
//               int s2 = entrytext.indexOf(":", s);
//               int s3 = entrytext.indexOf(",", s);
//               String from = entrytext.substring(s2 + 1, s3);
//               String[] req = from.split("[/]+");
//               destType=req[1];
//               destID=req[3];
//               
//               s = entrytext.indexOf("To");
//               s2 = entrytext.indexOf(":", s);
//               s3 = entrytext.indexOf(",", s);
//               String to = entrytext.substring(s2 + 1, s3);
//
//               entrytext.replaceAll("\"from\":\""+from+"\"", "\"from\":\""+to+"\"");
//               entrytext.replaceAll("\"to\":\""+to+"\"", "\"to\":\""+from+"\"");
            }

            if ("ping".equals(cmd)) {
               outgoing_topic = "iot-2/evt/pingresp:" + destType + ":" + destID + "/fmt/json";
               action = "send";
            } else if ("disconnect".equals(cmd)) {
               outgoing_topic = "iot-2/evt/disconnect_resp:" + destType + ":" + destID + "/fmt/json";
               action = "disconnect";
            } else {
               outgoing_topic = "iot-2/evt/unknowncommand:" + destType + ":" + destID + "/fmt/json";
               action = "send";

            }
         }
         SVTLog.log3(TYPE, "replyTopic: " + outgoing_topic);


         // if ("UNLOCK".equals(messageText)) {
         // SVTLog.log(TYPE, "Doors unlocked");
         // replyText = "Doors unlocked";
         // } else if ("LOCK".equals(messageText)) {
         // SVTLog.log(TYPE, "Doors locked");
         // replyText = "Doors locked";
         // } else if ("GET_DIAGNOSTICS".equals(messageText)) {
         // SVTLog.log(TYPE, "Unsupported feature");
         // replyText = "Unsupported feature";
         // } else {
         // SVTLog.log(TYPE, "Unrecognized command");
         // replyText = "Unrecognized command";
         // }

         MqttMessage replyMessage = new MqttMessage(entrytext.getBytes());
         replyMessage.setQos(listenerQoS);
         TopicMessage tm = new TopicMessage(outgoing_topic, replyMessage);
         tm.setProperty("action", action);

         push(tm);

      }

   }

}
