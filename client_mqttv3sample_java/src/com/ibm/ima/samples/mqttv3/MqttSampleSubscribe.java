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

import java.util.Properties;

import com.ibm.micro.client.mqttv3.MqttCallback;
import com.ibm.micro.client.mqttv3.MqttClient;
import com.ibm.micro.client.mqttv3.MqttConnectOptions;
import com.ibm.micro.client.mqttv3.MqttDefaultFilePersistence;
import com.ibm.micro.client.mqttv3.MqttDeliveryToken;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttMessage;
import com.ibm.micro.client.mqttv3.MqttTopic;

/**
 * Class to encapsulate the IMA topic subscribe logic.
 * 
 * This class contains the doSubscribe method for subscribing to IMA topic messages. It also defines the necessary MQTT
 * callbacks for asynchronous subscriptions.
 * 
 */
public class MqttSampleSubscribe implements MqttCallback {
   MqttSample config = null;
   boolean done = false;
   int receivedcount = 0;

   /**
    * Callback invoked when the MQTT connection is lost
    * 
    * @param e
    *           exception returned from broken connection
    * 
    */
   public void connectionLost(Throwable e) {
      config.println("Lost connection to " + config.serverURI + ".  " + e.getMessage());
      e.printStackTrace();
      System.exit(1);
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
   public void doSubscribe(MqttSample config) throws MqttException {

      this.config = config;

      MqttDefaultFilePersistence dataStore = null;
      if (config.persistence)
         dataStore = new MqttDefaultFilePersistence(config.dataStoreDir);

      MqttClient client = new MqttClient(config.serverURI, config.clientId, dataStore);
      client.setCallback(this);

      MqttConnectOptions options = new MqttConnectOptions();

      // set CleanSession true to automatically remove server data associated
      // with the client id at
      // disconnect. In this sample the clientid is based on a random number and
      // not typically
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

      client.connect(options);

      // Subscribe to the topic. messageArrived() callback invoked when message
      // received.
      client.subscribe(config.topicName, config.qos);
      config.println("Client '" + client.getClientId() + "' subscribed to topic: '" + config.topicName + "' with QOS " + config.qos + ".");

      // wait for messages to arrive before disconnecting
      try {
         while (!isDone(null)) {
            Thread.sleep(500);
         }
      } catch (InterruptedException e) {
         e.printStackTrace();
      }

      // Disconnect the client
      client.unsubscribe(config.topicName);
      client.disconnect();

      if (config.verbose)
         config.println("Subscriber disconnected.");
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
   public void messageArrived(MqttTopic topic, MqttMessage receivedMessage) throws Exception {
      // increment count of received messages
      receivedcount++;

      if (receivedcount == 1)
         config.println("Receiving messages on topic '" + topic.getName() + "'");

      if (config.verbose)
         config.println("Message " + receivedcount + " received on topic '" + topic + "':  " + new String(receivedMessage.getPayload()));

      // if all messages have been received then print message and set done
      // flag.
      if (receivedcount == config.count) {
         config.println("Received " + config.count + " messages.");
         isDone(true);
      }
      
      return;
   }

   public void deliveryComplete(MqttDeliveryToken arg0) {
      // TODO Auto-generated method stub
      return;
   }

}
