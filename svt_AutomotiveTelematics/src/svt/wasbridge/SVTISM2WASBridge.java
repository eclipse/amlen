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
package svt.wasbridge;

import java.util.Hashtable;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Queue;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.naming.Context;
import javax.naming.directory.DirContext;
import javax.naming.directory.InitialDirContext;

import com.ibm.ima.jms.ImaProperties;
import com.ibm.websphere.sib.api.jms.JmsConnectionFactory;
import com.ibm.websphere.sib.api.jms.JmsFactoryFactory;

public class SVTISM2WASBridge implements Runnable {

   static boolean debug = true;

   /**
    * @param args
    */
   public void run() {

      String consumerDestName = null;
      Connection consumerConnISM = null;
      Session consumerSessionISM = null;
      MessageConsumer theISMConsumer = null;
      TextMessage theMessage = null;
      String producerDestName = null;
      Connection WASproducerConn = null;
      Session WASproducerSession = null;
      MessageProducer theWASProducer = null;

      String ismserver = "10.10.3.22";
      String ismport = "16102";
      int clientIdNum = 0 + (int) (Math.random() * ((1000000 - 0) + 1));
      String clientId = "id" + clientIdNum;

      Hashtable<String, String> env = new Hashtable<String, String>();
      env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.ldap.LdapCtxFactory");
      env.put(Context.PROVIDER_URL, "ldap://9.3.179.108:389/o=jndiTest");
      env.put(Context.SECURITY_AUTHENTICATION, "simple");
      env.put(Context.SECURITY_PRINCIPAL, "cn=Manager,o=jndiTest"); // specify
      // the
      // username
      env.put(Context.SECURITY_CREDENTIALS, "secret");

      try {

         DirContext ctx = new InitialDirContext(env);

         if (debug)
            System.out.println("SVTISM2MQBridge clientId is:" + clientId);

         // Get Connection factory name with LDAP - may be used for better
         // portability
         ConnectionFactory ISMconnectionFactory = (ConnectionFactory) ctx.lookup("cn=ISM_CONNECTION_FACTORY");

         // Non-LDAP way to use factory
         // ConnectionFactory ISMconnectionFactory =
         // ImaJmsFactory.createConnectionFactory();
         if (ismserver != null)
            ((ImaProperties) ISMconnectionFactory).put("Server", ismserver);
         if (ismport != null)
            ((ImaProperties) ISMconnectionFactory).put("Port", ismport);

         ((ImaProperties) ISMconnectionFactory).put("KeepAlive", 0); // zero
         // sets
         // it to
         // infinite
         // connection

         consumerConnISM = ISMconnectionFactory.createConnection();
         consumerConnISM.setClientID(clientId);
      } catch (Throwable e) {
         e.printStackTrace();
         return;
      }

      try {

         JmsFactoryFactory jmsFact = JmsFactoryFactory.getInstance();

         ConnectionFactory consWASConnectionFactory = jmsFact.createConnectionFactory();
         ((JmsConnectionFactory) consWASConnectionFactory).setBusName("SVTBus");
         // ((JmsConnectionFactory)
         // consWASConnectionFactory).setProviderEndpoints(providerEndpoints);
         ((JmsConnectionFactory) consWASConnectionFactory).setTargetTransportChain("InboundBasicMessaging");

         WASproducerConn = consWASConnectionFactory.createConnection("wasuser", "wasuser");
      } catch (Throwable e) {
         e.printStackTrace();
         return;
      }

      /*
       * Create consumer connection. Create consumer session from connection; false means session is not transacted.
       * Create consumer, then start message delivery. Receive all text messages from destination until a non-text
       * message is received indicating end of message stream. Close connection.
       */
      try {
         consumerSessionISM = consumerConnISM.createSession(false, Session.AUTO_ACKNOWLEDGE);
         if (debug)
            System.out.println("SVTISM2MQBridge ConsumerCF.createConnection(): " + consumerConnISM.toString());
         if (debug)
            System.out.println("SVTISM2MQBridge Consumer.createSession(..):" + consumerSessionISM);
         Topic topic = consumerSessionISM.createTopic("ISM2BRIDGE");

         theISMConsumer = consumerSessionISM.createConsumer(topic);
         if (debug)
            System.out.println("SVTISM2MQBridge ConsumerSession.createConsumer(" + consumerDestName + "): " + theISMConsumer);
         consumerConnISM.start();

      } catch (JMSException e) {
         if (debug)
            System.out.println("SVTISM2MQBridge Exception occurred with Consumer Session-Connection: " + e.toString());
         e.printStackTrace();
      }

      /*
       * Create producer connection. Create producer session from connection; false means session is not transacted.
       * Create producer and text message.
       */

      try {
         WASproducerSession = WASproducerConn.createSession(false, Session.AUTO_ACKNOWLEDGE);
         if (debug)
            System.out.println("SVTISM2MQBridge ProducerCF.createConnection(): " + WASproducerConn.toString());
         if (debug)
            System.out.println("SVTISM2MQBridge ProducerConn.createSession(..)" + WASproducerSession);
         // Topic topic = producerSession.createTopic("topic://SVTBridge");
         Queue MQqueue = WASproducerSession.createQueue("BRIDGE2MQ");
         theWASProducer = WASproducerSession.createProducer(MQqueue);
         if (debug)
            System.out.println("SVTISM2MQBridge ProducerSession.createProducer(" + producerDestName + "): " + theWASProducer);

      } catch (JMSException e) {
         if (debug)
            System.out.println("SVTISM2MQBridge Exception occurred with Producer Session-Connection: " + e.toString());
         e.printStackTrace();
      }

      /*
       * Create producer connection. Create producer session from connection; false means session is not transacted.
       * Create producer and text message. Send messages, varying text slightly. Send end-of-messages message. Finally,
       * close connection.
       */
      try {

         if (debug)
            System.out.println("SVTISM2MQBridge while(true)");
         while (true) {
            Message m = theISMConsumer.receive(100);

            if (m != null) {
               Queue replyToQueue = WASproducerSession.createQueue("MQ2BRIDGE");
               if (m instanceof TextMessage) {
                  if (debug)
                     System.out.println("SVTISM2MQBridge  m.getJMSReplyTo().toString() is: " + ((Topic) m.getJMSReplyTo()).getTopicName());
                  String BridgeReplyToTopic = ((Topic) m.getJMSReplyTo()).getTopicName();

                  theMessage = WASproducerSession.createTextMessage();
                  theMessage.setText(((TextMessage) m).getText());
                  theMessage.setJMSReplyTo(replyToQueue);
                  theMessage.setStringProperty("BridgeReplyToTopic", BridgeReplyToTopic);
                  if (debug)
                     System.out
                              .println("SVTISM2MQBridge Reading message: " + theMessage.getText() + ", of Class: " + theMessage.getClass().getName());

                  theWASProducer.send(theMessage);
                  if (debug)
                     System.out.println("SVTISM2MQBridge Sending message: " + theMessage.getText());

               } else {
                  System.exit(1); // currently not supported
               }
            }
         }
      } catch (JMSException e) {
         if (debug)
            System.out.println("SVTISM2MQBridge Exception occurred with Consumer Receive/Producer Send: " + e.toString());
         e.printStackTrace();
      } finally {
         if (debug)
            System.out.println("SVTISM2MQBridge finally");
         if (consumerConnISM != null) {
            try {
               consumerConnISM.close();
            } catch (JMSException e) {
               if (debug)
                  System.out.println("SVTISM2MQBridge Consumer connection.close exception " + e.getMessage());
            }
         }
         if (WASproducerConn != null) {
            try {
               WASproducerConn.close();
            } catch (JMSException e) {
               if (debug)
                  System.out.println("SVTISM2MQBridge Producer connection.close exception " + e.getMessage());
            }
         }
      } // finally

      if (debug)
         System.out.println("done.");
   } // main

}
