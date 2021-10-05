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
package com.ibm.ism.mqbridge.test;

import javax.jms.JMSException;
import javax.jms.Session;

import com.ibm.jms.JMSMessage;
import com.ibm.jms.JMSTextMessage;
import com.ibm.mq.jms.MQTopic;
import com.ibm.mq.jms.MQTopicConnection;
import com.ibm.mq.jms.MQTopicConnectionFactory;
import com.ibm.mq.jms.MQTopicPublisher;
import com.ibm.mq.jms.MQTopicSession;
import com.ibm.mq.jms.MQTopicSubscriber;
import com.ibm.msg.client.wmq.WMQConstants;

public class JmsMqClientTest {

	static int numberOfMessages =10;
  public static void main(String[] args) {
    try {
      MQTopicConnectionFactory tcf = new MQTopicConnectionFactory();

      // Config
      tcf.setHostName(ConnectionDetails.mqHostAddress);
      tcf.setPort(ConnectionDetails.mqPort);
      tcf.setTransportType(WMQConstants.WMQ_CM_CLIENT );
      tcf.setQueueManager("dttest");
      tcf.setChannel("SYSTEM.ISM.SVRCONN");

      MQTopicConnection connection = (MQTopicConnection) tcf.createTopicConnection();
      MQTopicSession session = (MQTopicSession) connection.createTopicSession(false, Session.AUTO_ACKNOWLEDGE);
      MQTopic topic = (MQTopic) session.createTopic("topic://MQ/1");
      MQTopicPublisher publisher =  (MQTopicPublisher) session.createPublisher(topic);
      MQTopicSubscriber subscriber = (MQTopicSubscriber) session.createSubscriber(topic);      

      JMSTextMessage message = (JMSTextMessage) session.createTextMessage("Hello World "+ 1);     

      // Start the connection
      connection.start();
      for (int x =0; x<numberOfMessages; x++) {
    	  
    	  message = (JMSTextMessage) session.createTextMessage("Hello World "+ x);     
      publisher.publish(message);
      System.out.println("Sent message: " + message);
      }
      for (int x =0; x<numberOfMessages; x++) {
      JMSMessage receivedMessage = (JMSMessage) subscriber.receive(10000);
      System.out.println(" Received message: " + receivedMessage);
      }
      publisher.close();
      subscriber.close();
      session.close();
      connection.close();
    }
    catch (JMSException je) {
      je.printStackTrace();
      System.out.println("JMSException");
    }
    catch (Exception ex) {
      ex.printStackTrace();
      System.out.println("Other Exception");
    }
  }
}

