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

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.Topic;
import javax.jms.TopicConnection;
import javax.jms.TopicConnectionFactory;
import javax.jms.TopicSession;
import javax.jms.TopicSubscriber;

import junit.framework.AssertionFailedError;
import junit.framework.TestCase;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaPropertiesImpl;



public class ImaNoAckTest extends TestCase {

    public ImaNoAckTest() {
        super("IMA Topic Test");
    }
    
    public void testTopicNoAck() {
        int failures = 0;
        try {
            ConnectionFactory tcf1 = ImaJmsFactory.createConnectionFactory();
            ((ImaProperties)tcf1).put("DisableACK", true);
            try {
                assertEquals((Boolean)true,(Boolean)((ImaProperties)tcf1).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            Connection tc1 = null;
            if (AllTests.setConnParams(((ImaProperties)tcf1)))
                tc1 = tcf1.createConnection();
            else {
                tc1 = new ImaConnection(true);
                ((ImaConnection)tc1).props.putAll(((ImaPropertiesImpl)tcf1).props);
                ((ImaConnection)tc1).client = ImaTestClient.createTestClient(tc1);
            }
            try {
                assertEquals((Boolean)true,(Boolean)((ImaProperties)tc1).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            Session ts1 = tc1.createSession(false,Session.AUTO_ACKNOWLEDGE);  
            try {
                assertEquals((Boolean)true,(Boolean)((ImaProperties)ts1).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            Topic topic1 = ts1.createTopic("topic1");
            try {
                assertNull((Boolean)((ImaProperties)topic1).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            TopicSubscriber tsub1 = ((TopicSession)ts1).createSubscriber(topic1);
            try {
                assertEquals((Boolean)true,(Boolean)((ImaProperties)tsub1).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            ((ImaProperties)topic1).put("DisableACK",false);
            try {
                assertEquals((Boolean)false,(Boolean)((ImaProperties)topic1).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            /*
             *  Acks are now allowed for topics
             */
            MessageConsumer cons1 = ts1.createConsumer(topic1);
            try {
                assertEquals((Boolean)false,(Boolean)((ImaProperties)cons1).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            
            TopicConnectionFactory tcf2 = (TopicConnectionFactory) ImaJmsFactory.createConnectionFactory();
            try {
                assertNull((Boolean)((ImaProperties)tcf2).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            tc1.close();
            
            TopicConnection tc2 = null;
            if (AllTests.setConnParams(((ImaProperties)tcf2)))
                tc2 = tcf2.createTopicConnection();
            else {
                tc2 = new ImaConnection(true);
                ((ImaConnection)tc2).props.putAll(((ImaPropertiesImpl)tcf2).props);
            }
            try {
                assertNull((Boolean)((ImaProperties)tc2).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            ((ImaConnection)tc2).client = ImaTestClient.createTestClient(tc2);
            TopicSession ts2 = tc2.createTopicSession(false,Session.AUTO_ACKNOWLEDGE); 
            try {
                assertNull((Boolean)((ImaProperties)ts2).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            Topic topic2 = ts2.createTopic("topic2");
            try {
            assertNull((Boolean)((ImaProperties)topic2).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            MessageConsumer cons2 = ts2.createConsumer(topic2, null);
            try {
            	assertNull((Boolean)((ImaProperties)cons2).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            ((ImaProperties)topic2).put("DisableACK",false);
            try {
                assertEquals((Boolean)false,(Boolean)((ImaProperties)topic2).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            /*
             * Ack is allowed for topics 
             */
            MessageConsumer cons22 = ts2.createConsumer(topic2, null, false);
            try {
                assertEquals((Boolean)false,(Boolean)((ImaProperties)cons22).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            tc2.close();
            
            TopicConnectionFactory tcf3 = (TopicConnectionFactory) ImaJmsFactory.createConnectionFactory();
            ((ImaProperties)tcf3).put("DisableACK", false);
            try {
                assertEquals((Boolean)false,(Boolean)((ImaProperties)tcf3).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
             
            TopicConnection tc3 = null;
            if (AllTests.setConnParams(((ImaProperties)tcf3)))
                tc3 = tcf3.createTopicConnection();
            else {
                tc3 = new ImaConnection(true);
                ((ImaConnection)tc3).props.putAll(((ImaPropertiesImpl)tcf3).props);
            }
            try {
                assertEquals((Boolean)false,(Boolean)((ImaProperties)tc3).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            /* This is needed when running the junit tests with a server */
            if (tc3.getClientID() == null)
            	tc3.setClientID("IsmNoAck_tc3");
            ((ImaConnection)tc3).client = ImaTestClient.createTestClient(tc3);
            TopicSession ts3 = tc3.createTopicSession(false,Session.AUTO_ACKNOWLEDGE); 
            try {
                assertEquals((Boolean)false,(Boolean)((ImaProperties)ts3).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            Topic topic3 = ts3.createTopic("topic3");
            try {
                assertNull((Boolean)((ImaProperties)topic3).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            /*
             *  IMPORTANT NOTE:
             *    Currently this tests enforcement of a server restriction that does 
             *    not permit ack for topics.  This test will need to be updated when
             *    this restriction is lifted.  When that is the case, we should see that
             *    the topic setting for DisableAck overrides the connection setting.
             */
            MessageConsumer sub3 = ts3.createSubscriber(topic3, null, false);
            try {
                assertEquals((Boolean)false,(Boolean)((ImaProperties)sub3).get("DisableACK"));
            } catch (AssertionFailedError afe) {
                failures++;
                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            }
            
            ((ImaProperties)topic3).put("DisableACK", true);
            MessageConsumer cons3 = ts3.createDurableSubscriber(topic3, "subscription3");
            try {
            	assertEquals((Boolean)true,(Boolean)((ImaProperties)cons3).get("DisableACK"));
            } catch (AssertionFailedError afe) {
            	failures++;
            	AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            } finally {
            	cons3.close();
            	ts3.unsubscribe("subscription3");
            }
            
            ((ImaProperties)topic3).put("DisableACK", false);
            TopicSubscriber dsub3 = ts3.createDurableSubscriber(topic3, "subscription3", null, false);
            try {
            	assertEquals((Boolean)false,(Boolean)((ImaProperties)dsub3).get("DisableACK"));
            } catch (AssertionFailedError afe) {
            	failures++;
            	AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
            } finally {
            	dsub3.close();
            	ts3.unsubscribe("subscription3");
            }
            
            tc3.close();
            
            assertEquals(failures == 1 ? "There was 1 failure" : "There were " + failures + " failures",failures,0);    
        } catch (JMSException e) {
            e.printStackTrace();
            // We should never reach this block so fail test if we do reach it
            assertTrue(false);
        } 
    }

    /*
     * TODO Uncomment when engine restriction is lifted.
     * For now only topics are supported. 
     */
//    @Ignore
//    public void testQueueNoAck() {
//        int failures = 0;
//        try {
//            ConnectionFactory cf1 = ImaJmsFactory.createConnectionFactory();
//            ((ImaProperties)cf1).put("protocol", "dummy");
//            ((ImaProperties)cf1).put("DisableACK", true);
//            try {
//                assertEquals((Boolean)true,(Boolean)((ImaProperties)cf1).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            Connection c1 = cf1.createConnection();
//            try {
//                assertEquals((Boolean)true,(Boolean)((ImaProperties)c1).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            ((ImaConnection)c1).client = IsmTestClient.createTestClient(c1);
//            Session s1 = c1.createSession(false,Session.AUTO_ACKNOWLEDGE);
//            try {
//                assertEquals((Boolean)true,(Boolean)((ImaProperties)s1).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            Queue queue1 = s1.createQueue("queue1");
//            try {
//                assertNull((Boolean)((ImaProperties)queue1).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//
//            QueueReceiver qrec1 = ((QueueSession)s1).createReceiver(queue1);
//            try {
//                assertEquals((Boolean)true,(Boolean)((ImaProperties)qrec1).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            ((ImaProperties)queue1).put("DisableACK",false);
//            MessageConsumer cons1 = s1.createConsumer(queue1);
//            try {
//                assertEquals((Boolean)false,(Boolean)((ImaProperties)cons1).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            c1.close();
//
//            QueueConnectionFactory qcf2 = (QueueConnectionFactory) ImaJmsFactory.createConnectionFactory();
//            try {
//                assertNull((Boolean)((ImaProperties)qcf2).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            ((ImaProperties)qcf2).put("protocol", "dummy");
//            //TODO: Implement createQueueConnection()
//            //QueueConnection qc2 = qcf2.createQueueConnection();
//            QueueConnection qc2 = (QueueConnection)((ConnectionFactory)qcf2).createConnection();
//            try {
//                assertNull((Boolean)((ImaProperties)qc2).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            ((ImaConnection)qc2).client = IsmTestClient.createTestClient(qc2);
//            QueueSession qs2 = qc2.createQueueSession(false,Session.AUTO_ACKNOWLEDGE);
//            try {
//                assertNull((Boolean)((ImaProperties)qs2).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            Queue queue2 = qs2.createQueue("queue2");
//            try {
//                assertNull((Boolean)((ImaProperties)queue2).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            MessageConsumer cons2 = qs2.createConsumer(queue2, null);
//            try {
//                //No forced default for DisableACK property for queue destinations
//                assertNull((Boolean)((ImaProperties)cons2).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            ((ImaProperties)queue2).put("DisableACK",false);
//            MessageConsumer cons22 = qs2.createConsumer(queue2, null, false);
//            try {
//                assertEquals((Boolean)false,(Boolean)((ImaProperties)cons22).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            qc2.close();
//            
//            QueueConnectionFactory qcf3 = (QueueConnectionFactory) ImaJmsFactory.createConnectionFactory();
//            ((ImaProperties)qcf3).put("protocol", "dummy");
//            ((ImaProperties)qcf3).put("DisableACK", false);
//            try {
//                assertEquals((Boolean)false,(Boolean)((ImaProperties)qcf3).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            Connection c3 = qcf3.createConnection(); 
//            try {
//                assertEquals((Boolean)false,(Boolean)((ImaProperties)c3).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            ((ImaConnection)c3).client = IsmTestClient.createTestClient(c3);
//            QueueSession qs3 = (QueueSession)c3.createSession(false,Session.AUTO_ACKNOWLEDGE);
//            try {
//                assertEquals((Boolean)false,(Boolean)((ImaProperties)qs3).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            Queue queue3 = qs3.createQueue("queue3");
//            try {
//                assertNull((Boolean)((ImaProperties)queue3).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            MessageConsumer cons3 = qs3.createReceiver(queue3, null);
//            try {
//                assertEquals((Boolean)false,(Boolean)((ImaProperties)cons3).get("DisableACK"));
//            } catch (AssertionFailedError afe) {
//                failures++;
//                AllTests.reportFailure(this.getClass().getName(), Thread.currentThread().getStackTrace()[2].getLineNumber());
//            }
//            
//            c3.close();
//            
//            assertEquals(failures == 1 ? "There was 1 failure" : "There were " + failures + " failures",failures,0);
//            
//        } catch (JMSException e) {
//            e.printStackTrace();
//            // We should never reach this block so fail test if we do reach it
//            assertTrue(false);
//        }
//    }
    
    public static void main(String args[]){
        new ImaNoAckTest().testTopicNoAck();
        /*
         * TODO Uncomment when engine restriction is lifted.
         * For now only topics are supported. 
         */
//        new IsmNoAckTest().testQueueNoAck();
    }
}
