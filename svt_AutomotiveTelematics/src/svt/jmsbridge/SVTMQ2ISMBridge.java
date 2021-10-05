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
package svt.jmsbridge;

import java.util.Hashtable;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
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
import com.ibm.mq.jms.JMSC;
import com.ibm.mq.jms.MQQueueConnectionFactory;

public class SVTMQ2ISMBridge implements Runnable {

   static boolean debug = true;

   /**
    * @param args
    */
   public void run() {

      String consumerDestName = null;
      Connection consumerConn = null;
      Session consumerSession = null;
      MessageConsumer theConsumer = null;
      TextMessage theMessage = null;
      String producerDestName = null;
      ConnectionFactory producerConnFactory = null;
      Connection producerConn = null;
      Session producerSession = null;
      Destination producerDest = null;
      MessageProducer theProducer = null;

      Hashtable<String, String> env = new Hashtable<String, String>();
      env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.ldap.LdapCtxFactory");
      env.put(Context.PROVIDER_URL, "ldap://9.3.179.108:389/o=jndiTest");
      env.put(Context.SECURITY_AUTHENTICATION, "simple");
      env.put(Context.SECURITY_PRINCIPAL, "cn=Manager,o=jndiTest"); // specify
      // the
      // username
      env.put(Context.SECURITY_CREDENTIALS, "secret");

      String ismserver = "10.10.3.22";
      String ismport = "16102";
      int clientIdNum = 0 + (int) (Math.random() * ((10000000 - 0) + 1));
      String clientId = "id" + clientIdNum;
      try {
         DirContext ctx = new InitialDirContext(env);

         if (debug)
            System.out.println("SVTMQ2ISMBridge ISM clientId is:" + clientId);

         // Get Connection factory name with LDAP - may be used for better
         // portability
         ConnectionFactory prodConnectionFactory = (ConnectionFactory) ctx.lookup("cn=ISM_CONNECTION_FACTORY");

         // Non-LDAP Way to get Connection factory
         // ConnectionFactory prodConnectionFactory =
         // ImaJmsFactory.createConnectionFactory();
         if (ismserver != null)
            ((ImaProperties) prodConnectionFactory).put("Server", ismserver);
         if (ismport != null)
            ((ImaProperties) prodConnectionFactory).put("Port", ismport);

         ((ImaProperties) prodConnectionFactory).put("KeepAlive", 0); // zero
         // sets
         // it
         // to
         // infinite
         // connection

         producerConn = prodConnectionFactory.createConnection();
         producerConn.setClientID(clientId);
      } catch (Throwable e) {
         e.printStackTrace();
         return;
      }

      try {

         /* Queue-based consumer version */

         DirContext ctx = new InitialDirContext(env);

         // Get Connection factory name with LDAP - may be used for better
         // portability
         ConnectionFactory consMQConnectionFactory = (ConnectionFactory) ctx.lookup("cn=MQ_CONNECTION_FACTORY");

         // Non-LDAP Way to get Connection factory
         // ConnectionFactory consMQConnectionFactory = new
         // MQQueueConnectionFactory();
         ((MQQueueConnectionFactory) consMQConnectionFactory).setHostName("9.3.179.231");
         ((MQQueueConnectionFactory) consMQConnectionFactory).setPort(16102);
         ((MQQueueConnectionFactory) consMQConnectionFactory).setTransportType(JMSC.MQJMS_TP_CLIENT_MQ_TCPIP);
         ((MQQueueConnectionFactory) consMQConnectionFactory).setQueueManager("SVTBRIDGE.QUEUE.MANAGER");
         ((MQQueueConnectionFactory) consMQConnectionFactory).setChannel("BRIDGE2MQ.CHANNEL");

         consumerConn = consMQConnectionFactory.createConnection("mquser", "mquser");

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
         producerSession = producerConn.createSession(false, Session.AUTO_ACKNOWLEDGE);
         if (debug)
            System.out.println("SVTMQ2ISMBridge ProducerCF.createConnection(): " + producerConn.toString());
         if (debug)
            System.out.println("SVTMQ2ISMBridge Producer.createSession(..):" + producerSession);

      } catch (JMSException e) {
         if (debug)
            System.out.println("SVTMQ2ISMBridge Exception occurred with Producer Session-Connection: " + e.toString());
         e.printStackTrace();
      }

      /*
       * Create producer connection. Create producer session from connection; false means session is not transacted.
       * Create producer and text message.
       */

      try {

         /* Queue Based Connection */
         consumerSession = consumerConn.createSession(false, Session.AUTO_ACKNOWLEDGE);

         if (debug)
            System.out.println("SVTMQ2ISMBridge ConsumerCF.createConnection(): " + consumerConn.toString());
         if (debug)
            System.out.println("SVTMQ2ISMBridge ConsumerConn.createSession(..)" + consumerSession);
         consumerConn.start();
         Queue queue = consumerSession.createQueue("MQ2BRIDGE");
         theConsumer = consumerSession.createConsumer(queue);
         if (debug)
            System.out.println("SVTMQ2ISMBridge ConsumerSession.createConsumer(" + consumerDestName + "): " + theConsumer);
      } catch (JMSException e) {
         if (debug)
            System.out.println("SVTMQ2ISMBridge Exception occurred with Consumer Session-Connection: " + e.toString());
         e.printStackTrace();
      }

      /*
       * Create producer connection. Create producer session from connection; false means session is not transacted.
       * Create producer and text message. Send messages, varying text slightly. Send end-of-messages message. Finally,
       * close connection.
       */
      try {
         if (debug)
            System.out.println("SVTMQ2ISMBridge while(true)");
         while (true) {
            Message m = theConsumer.receive(100);

            if (m != null) {
               Topic topic = producerSession.createTopic(m.getStringProperty("BridgeReplyToTopic"));
               theProducer = producerSession.createProducer(topic);
               if (debug)
                  System.out.println("SVTMQ2ISMBridge ProducerSession.createProducer(" + producerDestName + "): " + theProducer + "On Topic:"
                           + m.getStringProperty("BridgeReplyToTopic"));
               producerConn.start();

               if (m instanceof TextMessage) {
                  theMessage = (TextMessage) m;
                  if (debug)
                     System.out
                              .println("SVTMQ2ISMBridge Reading message: " + theMessage.getText() + ", of Class: " + theMessage.getClass().getName());

                  theProducer.send(theMessage);
                  if (debug)
                     System.out.println("SVTMQ2ISMBridge Sending message: " + theMessage.getText());

               } else {
                  if (debug)
                     System.out.println("SVTMQ2ISMBridge Reading message: " + m.toString() + ", of Class: " + m.getClass().getName() + " - "
                              + m.getClass().getSimpleName());

                  theProducer.send(m);
                  if (debug)
                     System.out.println("SVTMQ2ISMBridge Sending message... " + ", of Class: " + m.getClass().getName());
               }
            }
            producerConn.stop();
         }
      } catch (JMSException e) {
         if (debug)
            System.out.println("SVTMQ2ISMBridge Exception occurred with Consumer Receive/Producer Send: " + e.toString());
         e.printStackTrace();
      } finally {
         if (debug)
            System.out.println("SVTMQ2ISMBridge finally");
         if (consumerConn != null) {
            try {
               consumerConn.close();
            } catch (JMSException e) {
               if (debug)
                  System.out.println("SVTMQ2ISMBridge Consumer connection.close exception " + e.getMessage());
            }
         }
         if (producerConn != null) {
            try {
               producerConn.close();
            } catch (JMSException e) {
               if (debug)
                  System.out.println("SVTMQ2ISMBridge Producer connection.close exception " + e.getMessage());
            }
         }
      } // finally

      if (debug)
         System.out.println("done.");
   } // main

}
