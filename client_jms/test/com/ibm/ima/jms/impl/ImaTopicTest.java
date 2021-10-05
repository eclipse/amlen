/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.jms.impl;

import javax.jms.ConnectionMetaData;
import javax.jms.DeliveryMode;
import javax.jms.ExceptionListener;
import javax.jms.IllegalStateException;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Queue;
import javax.jms.QueueBrowser;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.jms.TopicConnection;
import javax.jms.TopicConnectionFactory;
import javax.jms.TopicPublisher;
import javax.jms.TopicSession;
import javax.jms.TopicSubscriber;

import junit.framework.TestCase;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaConnection;



public class ImaTopicTest extends TestCase {

    public ImaTopicTest() {
        super("IMA Topic Test");
    }
    
    public void testTopicConnection() {
        try {
            TopicConnectionFactory tcf1 = (TopicConnectionFactory) ImaJmsFactory.createConnectionFactory();
            ((ImaProperties)tcf1).put("protocol", "dummy");
            TopicConnection tc1 = tcf1.createTopicConnection();
            tc1.setClientID("tc1ID");
            assertEquals(tc1.getClientID(),"tc1ID");
            
            ((ImaConnection)tc1).client = ImaTestClient.createTestClient(tc1);
            TopicSession ts1 = tc1.createTopicSession(false,Session.AUTO_ACKNOWLEDGE);
            
            Topic tempTopic1 = null;
            tc1.close();
            try {
                tempTopic1 = ts1.createTemporaryTopic();
            } catch (IllegalStateException ise){
                // This is the expected exception
            }
            assertNull(tempTopic1);       
            
            TopicConnectionFactory tcf2 = ImaJmsFactory.createTopicConnectionFactory();
            ((ImaProperties)tcf2).put("protocol", "dummy");
            ((ImaProperties)tcf2).put("TemporaryTopic", "mytopic");
            TopicConnection tc2 = tcf2.createTopicConnection("testUser2","testPw2");
           
            ((ImaConnection)tc2).client = ImaTestClient.createTestClient(tc2);
            TopicSession ts2 = (TopicSession)tc2.createSession(false,Session.AUTO_ACKNOWLEDGE);
            Topic tempTopic2 = ts2.createTemporaryTopic();
            assertTrue(tempTopic2.toString().indexOf("mytopic_") == 0);
            assertTrue(tempTopic2.getTopicName().indexOf("mytopic_") == 0);
            
            tc2.setExceptionListener((ExceptionListener)tc2);
            assertEquals(tc2.getExceptionListener().getClass().getName(), "com.ibm.ima.jms.impl.ImaConnection");
            
            ConnectionMetaData metadata = tc2.getMetaData();
            assertEquals(metadata.getJMSMajorVersion(),1);
            assertEquals(metadata.getJMSMinorVersion(),1);
            
            // Can call start more than once
            tc2.start();
            tc2.start();
            
            //Can call stop more than once
            tc2.stop();
            tc2.stop();
            
            tc2.close();
            
            try {
                tc2.start();
            } catch (IllegalStateException ex){
                // We expect this exception after close
            }
                
            
        } catch (JMSException e) {
            e.printStackTrace();
            // We should never reach this block so fail test if we do reach it
            assertTrue(false);
        }
    }
    
    public void testTopicSession() {
        try {
            TopicConnectionFactory tcf1 = (TopicConnectionFactory) ImaJmsFactory.createConnectionFactory();
            ((ImaProperties)tcf1).put("protocol", "dummy");
            TopicConnection tc1 = tcf1.createTopicConnection();
            ((ImaConnection)tc1).client = ImaTestClient.createTestClient(tc1);
            TopicSession ts1 = tc1.createTopicSession(false,Session.AUTO_ACKNOWLEDGE);
            
            Topic topic = ts1.createTopic("topic");
            
            QueueBrowser qb1 = null;
            try {
                ts1.createBrowser((Queue)topic);
            } catch(IllegalStateException ide) { 
                //We expect this exception for topic 
            }
            assertNull(qb1);
            
            try {
                ts1.createBrowser((Queue)topic, null);
            } catch(IllegalStateException ide) { 
                //We expect this exception for topic 
            }
            assertNull(qb1);
            
            MessageConsumer mc1 = ts1.createConsumer(topic);
            assertNotNull(mc1);
            mc1 = null;
            
            mc1 = ts1.createConsumer(topic, null);
            assertNotNull(mc1);
            mc1 = null;
            
            mc1 = ts1.createConsumer(topic, null, true);
            assertNotNull(mc1);
            mc1 = null;
            
            TopicSubscriber tsub1 = ts1.createDurableSubscriber(topic, "testsub1");
            assertNotNull(tsub1);
            ts1.unsubscribe("testsub1");
            tsub1 = null;
            
            tsub1 = ts1.createDurableSubscriber(topic, "testsub1", null, true);
            assertNotNull(tsub1);
            ts1.unsubscribe("testsub1");
            tsub1 = null;
            
            TopicPublisher tpub1 = ts1.createPublisher(topic);
            assertNotNull(tpub1);
            
            tsub1 = ts1.createSubscriber(topic);
            assertNotNull(tsub1);
            tsub1 = null;
            
            tsub1 = ts1.createSubscriber(topic, null, true);
            assertNotNull(tsub1);
            tsub1 = null;
            
            tc1.close();
        } catch (JMSException e) {
            e.printStackTrace();
            // We should never reach this block so fail test if we do reach it
            assertTrue(false);
        }   
    }
    
    public void testPubSub() {
        try {
            TopicConnectionFactory tcf1 = (TopicConnectionFactory) ImaJmsFactory.createConnectionFactory();
            ((ImaProperties)tcf1).put("protocol", "dummy");
            TopicConnection tc1 = tcf1.createTopicConnection();
            ((ImaConnection)tc1).client = ImaTestClient.createTestClient(tc1);
            TopicSession ts1 = tc1.createTopicSession(false,Session.AUTO_ACKNOWLEDGE);
            
            Topic topic = ts1.createTopic("topic");
            
            TopicPublisher tpub1 = ts1.createPublisher(topic);
            TopicSubscriber tsub1 = ts1.createSubscriber(topic);
            TextMessage msg = ts1.createTextMessage("test");
            
            tc1.start();
            tpub1.publish(msg);
            TextMessage rmsg = (TextMessage)tsub1.receive();
            assertEquals(rmsg.getText(),"test");
            long expiretime = System.currentTimeMillis() + 10L;
            tpub1.publish(msg,DeliveryMode.NON_PERSISTENT,7,10L);
            rmsg = (TextMessage)tsub1.receive();
            assertEquals(rmsg.getText(),"test");
            assertEquals(rmsg.getJMSDeliveryMode(),DeliveryMode.NON_PERSISTENT);
            assertEquals(rmsg.getJMSPriority(), 7);
            assertEquals(rmsg.getJMSExpiration(), expiretime);
            
            tpub1.close();
            tpub1 = null;
            msg = null;
            rmsg = null;
            
            tpub1 = ts1.createPublisher(null);
            msg = ts1.createTextMessage("test");
            tpub1.publish(topic, msg);
            rmsg = (TextMessage)tsub1.receive();
            assertEquals(rmsg.getText(),"test");
            
            Topic topic2 = ts1.createTopic("topic2");
            Topic topic3 = ts1.createTopic("topic3");
            TextMessage msg2 = ts1.createTextMessage("test2");
            expiretime = System.currentTimeMillis() + 10L;
            tpub1.publish(topic2, msg2,DeliveryMode.NON_PERSISTENT,7,10L);
            rmsg = (TextMessage)tsub1.receive();
            assertEquals(rmsg.getText(),"test2");
            assertEquals(rmsg.getJMSDeliveryMode(),DeliveryMode.NON_PERSISTENT);
            assertEquals(rmsg.getJMSPriority(), 7);
            assertEquals(rmsg.getJMSExpiration(), expiretime);
            TextMessage msg3 = ts1.createTextMessage("test3");
            tpub1.publish(topic3, msg3);
            rmsg = (TextMessage)tsub1.receive();
            assertEquals(rmsg.getJMSDeliveryMode(),Message.DEFAULT_DELIVERY_MODE);
            assertEquals(rmsg.getJMSPriority(), Message.DEFAULT_PRIORITY);
            assertEquals(rmsg.getJMSExpiration(), Message.DEFAULT_TIME_TO_LIVE);
            assertEquals(rmsg.getText(),"test3");
            
            Exception ex = null;
            try {
            tpub1.publish(msg,DeliveryMode.NON_PERSISTENT,7,10L);
            } catch (UnsupportedOperationException e) {
                //This exception is expected here because there is no destination set for the publisher
                ex = e;
            }
            assertEquals(ex.getClass().getName() , UnsupportedOperationException.class.getName());
            
            try {
                tpub1.publish(msg);
            } catch (UnsupportedOperationException e) {
                //This exception is expected here because there is no destination set for the publisher
                ex = e;
            }
            assertEquals(ex.getClass().getName() , UnsupportedOperationException.class.getName());
            
            ts1.close();
            tc1.close();
        } catch (JMSException e) {
            e.printStackTrace();
            // We should never reach this block so fail test if we do reach it
            assertTrue(false);
        }
    }
    
    public static void main(String args[]){
        new ImaTopicTest().testTopicConnection();
        new ImaTopicTest().testTopicSession();
        new ImaTopicTest().testPubSub();
    }
}
