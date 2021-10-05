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
import java.util.ArrayList;
import java.util.Date;
import java.util.Hashtable;
import java.util.Properties;
import java.util.Random;

import javax.net.SocketFactory;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.MqttSecurityException;
import org.eclipse.paho.client.mqttv3.MqttTopic;

import svt.util.SVTConfig.BooleanEnum;
import svt.util.SVTConfig.IntegerEnum;
import svt.util.SVTConfig.LongEnum;
import svt.util.SVTConfig.StringEnum;
import svt.util.SVTLog;
import svt.util.SVTLog.TTYPE;
import svt.util.SVTMqttClient;
import svt.util.SVTStats;
import svt.util.SVTTopicMessage;
import svt.util.SVTUtil;

public class SVTVehicleEngine extends Thread {
//   Byte[] conLock = null;

   class SVTVehicle_Callback implements MqttCallback {
      MqttClient client = null;
      MqttConnectOptions options = null;
      ArrayList<SVTTopicMessage> stack = null;

      SVTVehicle_Callback(MqttClient client, MqttConnectOptions options, ArrayList<SVTTopicMessage> stack) {
         this.client = client;
         this.options = options;
         this.stack = stack;
      }

      public void connectionLost(Throwable arg0) {
         if (BooleanEnum.verbose_connectionLost.value == true) {
            SVTLog.logex2(TYPE, client.getClientId() + " connectionLost", arg0);
            SVTLog.log(TYPE, client.getClientId() + " connectionLost, initialConnectComplete " + initialConnectComplete);
         }
         synchronized (pubLock) {
            // check if still disconnected after getting pubLock
            if (client.isConnected() == false) {
               if (IntegerEnum.reconnectDelay.value > 0) {
                  if (BooleanEnum.verbose_connectionLost.value == true) {
                     SVTLog.log(TYPE, client.getClientId() + " connectionLost, sleep " + IntegerEnum.reconnectDelay.value);
                  }
                  try {
                     Thread.sleep(IntegerEnum.reconnectDelay.value);
                  } catch (InterruptedException e) {
                  }
               }
               IMqttToken token = null;
               while (client.isConnected() == false) {
                  SVTLog.logtry(TYPE, "connect");
                  try {
                     token = client.connectWithResult(options);
                     if (token == null)
                        throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
                     token.waitForCompletion();
                     SVTLog.logdone(TYPE, "reconnected.");
                  } catch (MqttSecurityException e) {
                     SVTLog.logex(TYPE, "reconnect", e);
                  } catch (Exception e) {
                     SVTLog.log(TYPE, "reconnect failed " + e.getMessage());
                  }
                  //seems to be a paho issue if you check isConnected() too soon
                  try {
                     Thread.sleep(100);
                  } catch (InterruptedException e2) {
                  }
                  if (client.isConnected() == false) {
                     try {
                        if (IntegerEnum.reconnectDelay.value > 0) {
                           Thread.sleep(IntegerEnum.reconnectDelay.value);
                        } else {
                           Thread.sleep(500);
                        }
                     } catch (InterruptedException e2) {
                     }
                  }
               }
            }
         }
      }

      @Override
      public void deliveryComplete(IMqttDeliveryToken imqttdeliverytoken) {
         // TODO Auto-generated method stub

      }

      @Override
      public void messageArrived(String requestTopic, MqttMessage mqttmessage) throws Exception {
         // String request = null;
         SVTLog.log3(TYPE, "messageArrived on topic " + requestTopic);
         String action = null;

         String[] token = requestTopic.split("/");
         String outgoing_topic = null;
         String entrytext = new String(mqttmessage.getPayload());
         if (BooleanEnum.iotf.value == false) {
            outgoing_topic = "/" + token[3] + "/" + token[4] + "/" + token[1] + "/" + token[2] + "/" + token[5] + "/" + token[6];
            // request = "ping";
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
               // int s = entrytext.indexOf("From");
               // int s2 = entrytext.indexOf(":", s);
               // int s3 = entrytext.indexOf(",", s);
               // String from = entrytext.substring(s2 + 1, s3);
               // String[] req = from.split("[/]+");
               // destType=req[1];
               // destID=req[3];
               //
               // s = entrytext.indexOf("To");
               // s2 = entrytext.indexOf(":", s);
               // s3 = entrytext.indexOf(",", s);
               // String to = entrytext.substring(s2 + 1, s3);
               //
               // entrytext.replaceAll("\"from\":\""+from+"\"", "\"from\":\""+to+"\"");
               // entrytext.replaceAll("\"to\":\""+to+"\"", "\"to\":\""+from+"\"");
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
         replyMessage.setQos(IntegerEnum.listenerQoS.value);
         SVTTopicMessage tm = new SVTTopicMessage(outgoing_topic, replyMessage);
         tm.setProperty("action", action);

         push(tm);

      }

      public void push(SVTTopicMessage tm) {
         synchronized (stack) {
            stack.add(tm);
         }
      }

   }

   static TTYPE TYPE = TTYPE.PAHO;
   long carId = 0;
   DateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSSZ");
   boolean done = false;
   String listenerTopic = null;
   long max = 0;
   long messages = 0;
   long minutes = 0;
   // static String dataStoreDir = System.getProperty("java.io.tmpdir");
   boolean persistence = false;
   Byte[] pubLock = new Byte[1];
   boolean initialConnectComplete = false;
   SVTStats stats = null;
   Hashtable<MqttTopic, MqttClient> tlist;
   String topicName = "";
   String userName = null;
   Properties sslClientProps = null;
   SocketFactory socketFactory = null;
   
   SVTVehicleEngine(SVTStats stats, long carId, String userName, Properties sslClientProps, SocketFactory socketFactory) {
      if (LongEnum.minutes.value != null) {
         this.minutes = LongEnum.minutes.value;
      }
      if (LongEnum.hours.value != null) {
         this.minutes += (LongEnum.hours.value * 60);
      }
      if (LongEnum.messages.value != null) {
         this.messages = LongEnum.messages.value;
      }
      this.max = this.minutes * 60000;
      this.stats = stats;
      this.carId = carId;
      this.userName = userName;
//      this.conLock = conLock;
      this.sslClientProps = sslClientProps;
      this.socketFactory = socketFactory;
   }

   public SVTMqttClient getSVTClient(String pClientId) throws Exception {
      MqttClient client = null;
      SVTLog.logtry(TYPE, "client");
      int i = 0;
      String ism = (BooleanEnum.ssl.value == false ? "tcp" : "ssl") + "://" + StringEnum.server.value + ":" + IntegerEnum.port.value;

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

      if (StringEnum.server2.value != null) {
         String ism2 = (BooleanEnum.ssl.value == false ? "tcp" : "ssl") + "://" + StringEnum.server2.value + ":" + IntegerEnum.port.value;
         String[] list = new String[2];
         list[0] = ism;
         list[1] = ism2;
         options.setServerURIs(list);
      }

      // options.setCleanSession(cleanSession);
      if (StringEnum.oauthProviderURI.value != null) {
         String oauthToken = new svt.util.GetOAuthAccessToken().getOAuthToken(StringEnum.oauthUser.value, StringEnum.oauthPassword.value,
               StringEnum.oauthProviderURI.value, StringEnum.oauthTrustStore.value, StringEnum.oauthTrustStorePassword.value,
               StringEnum.oauthKeyStore.value, StringEnum.oauthKeyStorePassword.value);
         if (oauthToken == null) {
            SVTLog.log(TYPE, "Unable to get oauth token");
            new Exception().printStackTrace();
         } else {
            SVTLog.log(TYPE, "using oauth token");
            options.setUserName("IMA_OAUTH_ACCESS_TOKEN");
            options.setPassword(oauthToken.toCharArray());
         }
      } else {
         if (userName != null) {
            options.setUserName(userName);
         }
         if (StringEnum.password.value != null) {
            options.setPassword(StringEnum.password.value.toCharArray());
         }
      }
      //      if (BooleanEnum.ssl.value) {
//         Properties sslClientProps = new Properties();
//         // sslClientProps.setProperty("com.ibm.ssl.protocol", "SSLv3");
//         // sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "SSL_RSA_WITH_AES_128_CBC_SHA");
//         sslClientProps.setProperty("com.ibm.ssl.protocol", "TLSv1.2");
//         sslClientProps.setProperty("com.ibm.ssl.enabledCipherSuites", "TLS_RSA_WITH_AES_128_GCM_SHA256");
//
//         String keyStore = System.getProperty("com.ibm.ssl.keyStore");
//         if (keyStore == null)
//            keyStore = System.getProperty("javax.net.ssl.keyStore");
//
//         String keyStorePassword = System.getProperty("com.ibm.ssl.keyStorePassword");
//         if (keyStorePassword == null)
//            keyStorePassword = System.getProperty("javax.net.ssl.keyStorePassword");
//
//         String trustStore = System.getProperty("com.ibm.ssl.trustStore");
//         if (trustStore == null)
//            trustStore = System.getProperty("javax.net.ssl.trustStore");
//
//         String trustStorePassword = System.getProperty("com.ibm.ssl.trustStorePassword");
//         if (trustStorePassword == null)
//            trustStorePassword = System.getProperty("javax.net.ssl.trustStorePassword");
//
//         if (keyStore != null)
//            sslClientProps.setProperty("com.ibm.ssl.keyStore", keyStore);
//         if (keyStorePassword != null)
//            sslClientProps.setProperty("com.ibm.ssl.keyStorePassword", keyStorePassword);
//         if (trustStore != null)
//            sslClientProps.setProperty("com.ibm.ssl.trustStore", trustStore);
//         if (trustStorePassword != null)
//            sslClientProps.setProperty("com.ibm.ssl.trustStorePassword", trustStorePassword);
//
////         options.setSSLProperties(sslClientProps);
//         
//         SSLSocketFactoryFactory factoryFactory = new SSLSocketFactoryFactory();
//         factoryFactory.initialize(sslClientProps, null);
//         SocketFactory socketFactory = factoryFactory.createSocketFactory(null);
//         options.setSocketFactory(socketFactory);
//      }
      options.setSSLProperties(sslClientProps);
      options.setSocketFactory(socketFactory);
      
      options.setKeepAliveInterval(IntegerEnum.keepAlive.value);
      options.setConnectionTimeout(IntegerEnum.connectionTimeout.value);
     
      ArrayList<SVTTopicMessage> stack = new ArrayList<SVTTopicMessage>();

      SVTVehicle_Callback callback = new SVTVehicle_Callback(client, options, stack);
      client.setCallback(callback);

      // synchronize with connectionLost
      synchronized (pubLock) {
         if (BooleanEnum.forceCleanSession.value) {
            options.setCleanSession(BooleanEnum.cleanSession.value);
            subscriber_connect(client, options);
         } else {
            if (BooleanEnum.cleanSession.value) {
               options.setCleanSession(true);
               subscriber_connect(client, options);
               subscriber_disconnect(client);
            }
            options.setCleanSession(false);
            subscriber_connect(client, options);
         }
         initialConnectComplete = true;
      }
      
      return new SVTMqttClient(client, options, callback, stack);

   }
 
    
   public void run() {
      try {
      String pClientId = null;
      Random random = null;
      if (BooleanEnum.disconnect.value) {
         random = new Random(System.currentTimeMillis());
      }

      if (BooleanEnum.iotf.value == true) {
         pClientId = userName;
         userName = "use-token-auth";
      } else if (BooleanEnum.genClientID.value) {
         pClientId = SVTUtil.MyClientId(TYPE);
      } else {
         pClientId = userName;
      }
      this.topicName = SVTUtil.MyTopicId(StringEnum.appid.value, pClientId);
      String message = "";

         try {
            this.stats.reinit();

            SVTLog.log3(TYPE, pClientId + " to send messages on topic " + topicName + " Connection mode is " + StringEnum.mode.value);

            SVTMqttClient svtclient = getSVTClient(pClientId);
            MqttTopic topic = svtclient.client.getTopic(topicName);
            String messagetext = null;
            Long startTimeStamp = null;
            Long prevTimeStamp = null;

            if (BooleanEnum.listener.value == true) {
               if (BooleanEnum.iotf.value == false) {
                  listenerTopic = "/CAR/" + pClientId + "/APP/" + StringEnum.appid.value + "/#";
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

               if (true == BooleanEnum.order.value) {
                  messagetext = "ORDER:" + pClientId + ":" + IntegerEnum.qos.value + ":" + this.stats.getCount() + ":"
                        + (LongEnum.messages.value - 1) + ":" + " ";
               } else {
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
                           + "\" , \"Topic\":\"" + topicName + "\" , \"QoS\":" + IntegerEnum.qos.value + " , \"Index\":"
                           + this.stats.getCount() + " , \"payload\":\"" + payload + "\"  }}";
                  } else {
                     messagetext = "GPS " + x + "," + y;
                  }
               }
               try {
                  sendMessage(topic, messagetext, IntegerEnum.qos.value);
                  this.stats.increment();
               } catch (Exception e) {
                  SVTLog.logex(TYPE, "sendMessage", e);
                  subscriber_reconnect(svtclient);
               }

               if ((BooleanEnum.listener.value) && (svtclient.stacksize() > 0))
                  sendReplyMessage(svtclient);

               if ((BooleanEnum.disconnect.value) && (random.nextInt(9999) < IntegerEnum.disconnectRate.value)) {
                  subscriber_reconnect(svtclient);
               }

               // if (((max > 0) && (this.stats.getDeltaRunTimeMS() >= max)) || ((this.messages > 0) && (this.stats.getCount() >=
               // this.messages))) {
               // break;
               // }
               this.stats.afterMessageRateControl();
            }

            if (BooleanEnum.listener.value == true) {
               svtclient.client.unsubscribe(listenerTopic);
            }

            try {
               svtclient.client.disconnect();
            } catch (MqttException e) {
               SVTLog.logex(TYPE, pClientId + ": Exception caught from client.disconnect()", e);
            }

         } catch (Throwable e) {
         SVTLog.logex(TYPE, "sendMessage", e);
         SVTUtil.stop(true);
      }

      int t = SVTUtil.threadcount(-1);

      String text = "mqttclient ," + pClientId + ", finished__" + message + ". Ran for ," + this.stats.getRunTimeSec() + ", sec. Sent ,"
            + this.stats.getCount() + ", messages. Rate: ," + this.stats.getRatePerMin() + ", msgs/min ," + this.stats.getRatePerSec()
            + ", msgs/sec " + t + " clients remain.";

      SVTLog.log3(TYPE, text);
      } catch (Throwable e) {
         SVTLog.logex(TYPE, "Run Exeption caught", e);
         e.printStackTrace();
      }
      
      return;
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

   public void sendMessage(MqttTopic topic, String payload, Integer qos) throws Throwable {

      // Construct the message to publish
      MqttMessage message = new MqttMessage(payload.getBytes());
      message.setQos(qos);

      // The default retained setting is false.
      if (persistence)
         message.setRetained(true);

      // Publish the message
      // System.out.print(" publish(" + tid + ")");
      SVTLog.logtry(TYPE, "publish[" + payload + "]");
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
            if (IntegerEnum.qos.value > 0) {
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
            if (e instanceof java.net.SocketException) {
               SVTLog.logex(TYPE, "publish", e);
               throw e;
            }
            if (e instanceof java.io.EOFException) {
               SVTLog.logex(TYPE, "publish", e);
               throw e;
            }
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

   public void sendReplyMessage(SVTMqttClient svtclient) throws Exception {

      SVTTopicMessage reply = svtclient.pop();
      String topic = reply.getTopic();

      if ("disconnect".equals(reply.getProperty("action"))) {
         subscriber_reconnect(svtclient);
      }

      // dont change the message contents.. just send it back as-is
      //      String text = Long.toString(System.currentTimeMillis());
      //      reply.setMessageText(text.getBytes());
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

   void subscriber_connect(MqttClient client, MqttConnectOptions options) throws Exception {
      int i = 0;
      IMqttToken token = null;
      SVTLog.logtry(TYPE, "connect");
      while (client.isConnected() == false) {
         try {
            token = client.connectWithResult(options);
            if (token == null)
               throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
            token.waitForCompletion();
            //seems to be a paho issue if you check isConnected() too soon
            try {
               Thread.sleep(100);
            } catch (InterruptedException e2) {
            }
            if (client.isConnected() == false) {
               throw new MqttException(MqttException.REASON_CODE_UNEXPECTED_ERROR);
            }
         } catch (Exception e) {

            // Catching all exceptions for now. May need to separate logic for MqttException
            if (i++ >= 500) {
               SVTLog.logex(TYPE, "connect", e);
               throw e;
            } else if (BooleanEnum.verbose_connect.value == true) {
               SVTLog.logex(TYPE, "connect", e);
            }

            try {
               Thread.sleep(10000 + (int) (Math.random() * 50000.0));
            } catch (InterruptedException e2) {
               SVTLog.logex(TYPE, "sleep", e);
            }
            SVTLog.logretry(TYPE, "connect", e, i);
         }
      }

      SVTLog.logdone(TYPE, "connect");

   }

   void subscriber_disconnect(MqttClient client) throws MqttException {

      SVTLog.logtry(TYPE, "disconnect");
      client.disconnect();
      try {
         Thread.sleep(100);
         while (client.isConnected() == true) {
            Thread.sleep(100);
         }
      } catch (InterruptedException e) {
      }
      SVTLog.logdone(TYPE, "disconnect");

   }

   public void subscriber_reconnect(SVTMqttClient svtclient) throws Exception {
      synchronized (pubLock) {
      }
      subscriber_disconnect(svtclient.client);
      subscriber_connect(svtclient.client, svtclient.options);
      SVTLog.log3(TYPE, "client " + svtclient.client.getClientId() + " reconnected.");
   }

}
