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
import javax.jms.Destination;
import javax.jms.ExceptionListener;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.TemporaryTopic;
import javax.jms.TextMessage;
import javax.jms.Topic;

import junit.framework.Assert;
import junit.framework.TestCase;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;


/*
 * TemporaryTopic will use defaults receive message and default port if not given specified uri
 * If connect specified dataport, and it doesn't match with what temporaryTopic specified or default port, this test
 * will failed.
 * 
 * We should overwrite the dateport specified in TemporaryTopic uri with connect.port
 * 
 */
public class ImaJmsTemporaryTopicTest extends TestCase implements ExceptionListener {
    static int topicCount = 0;
    static int msgsize = 120;
    static int logLevel = 6;
    static boolean showLog = false;
    static boolean verbose = false;
    
    /*
     * Class objects
     */
    static ConnectionFactory fact;
    static Connection conn;
    static Topic [] jtopics = new Topic[topicCount+1];
    static Topic [] otopics = new Topic[2];
    static ImaSession session;
    static MessageConsumer [] msgcon = new MessageConsumer[topicCount+1]; 
    static MessageProducer outtopics[] = new MessageProducer[2];
    static MessageConsumer cons = null;
    static TemporaryTopic tt;
    static MessageConsumer tempRev;
    static final String topic1 = "OneTopic";
    static final String topic2 = "MultiTopic";
    static byte [] bmsg = new byte[msgsize];
    static String rcvmsg;
	static String xmlmsg = new String("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
			"<!-- \n"                                                     +
			" This file specifies the IP address of the IMA-CM server.\n" +
			" Several test cases use this file as the include file.   \n" +
			" Required Action:\n"                                         +
			" 1. Replace CMIP with the IP address of the IMA-CM server.\n"+
			"-->\n"														  +
			"<param type=\"structure_map\">ip_address=CMIP</param>");
    
    /*
     * Event handler
     * @see javax.jms.ExceptionListener#onException(javax.jms.JMSException)
     */
    public void onException(JMSException jmse) {
            if (showLog)
                System.out.println(""+jmse);
    }

    /*
     * Setup
     */
    public void testJMSTemporaryTopicSetup() throws JMSException {
    	int i;
    	ImaProperties props;
        ImaProperties tprops;
        ImaProperties rprops;
        
        if (verbose)
            System.out.println("*TESTCASE: " + this.getName());
        try {
            /* 
             * Set up the JMS connection
             * the props here also used by createIMAConnection to setup inst/txinst/rxinst. 
             * tx/rxinst advance config can be set using XT/XX: extension. I feel it is very confusion to have a combined
             * props for three objects. what happen I want to set config for TxInst alone here?
             *
             */
            fact = ImaJmsFactory.createConnectionFactory();
            props = (ImaProperties)fact;
            props.put("TemporaryTopic", "TTopic/$CLIENTID/");
            props.put("Protocol", "@@@");
            conn = new ImaConnection(true);
            ((ImaConnection)conn).props.putAll(((ImaPropertiesImpl)props).props);
            ((ImaConnection)conn).client = ImaTestClient.createTestClient(conn);
            ((ImaConnection)conn).client = ImaTestClient.createTestClient(conn);
            conn.setExceptionListener(this);
            ((ImaConnection)conn).generated_clientid = true;
            session = (ImaSession) conn.createSession(false, 0);
            tt = session.createTemporaryTopic();
            //System.out.println(ImaJmsObject.toString(tt, "*"));
            //System.out.println(tt.toString());
	        tempRev = session.createConsumer(tt);
	        if (System.getenv("IMAServer") == null) {
	            ((ImaMessageConsumer)tempRev).isStopped = false;
	        }
        
          
            /*
             * Create some input destinations
             */
            for (i=0; i<=topicCount; i++) {
                jtopics[i] = ImaJmsFactory.createTopic(i==topicCount ? topic1 : topic2);
                rprops = (ImaProperties)jtopics[i];
                rprops.put("Group", "239.239.1.2");
                msgcon[i] = session.createConsumer(jtopics[i]);
            }  
            cons = msgcon[topicCount];
            
            /*
             * Create the output destination
             */
            otopics[0] = ImaJmsFactory.createTopic(topic1);
            tprops = (ImaProperties)otopics[0];
	        tprops.put("Destination", "rmm://239.239.1.2:31000");
	        outtopics[0] = session.createProducer(otopics[0]);
	        otopics[1] = ImaJmsFactory.createTopic(topic2);
            tprops = (ImaProperties)otopics[1];
            tprops.put("Destination", "rmm://239.239.1.2:31000");
            outtopics[1] = session.createProducer(otopics[1]);
            
            conn.start();
            if (System.getenv("IMAServer") == null) {
                ((ImaMessageConsumer)cons).isStopped = false;
                for (int which=0; which<topicCount; which++) {
                
                    ((ImaMessageConsumer)msgcon[which]).isStopped = false;
                }
            }
        }catch (JMSException je) {
        	je.printStackTrace(System.err);
        }
        assertEquals(false, session.isClosed.get());
    }

    
    
    /*
     * Test ObjectMessage
     * An ObjectMessage can contain any Java object.  The test
     * reads the object casts it to the appropriate type.
     */
    public void testJMSTemporaryTopic() throws Exception {
        doTempTopicReceive(1);
        //doMessageReceive(topicCount);
    }
    
    
    private void doTempTopicReceive(int topicCount) throws Exception {
	    Message rmsg;
	    Destination replyto = null;
	    MessageProducer output = topicCount==1 ? outtopics[0] : outtopics[1];

	   if (verbose)
	       System.out.println("*TESTCASE: " + this.getName());
       try {   
	        /*
	         * Send regular msg with tmp topic attached
	         */
	        Message msg = session.createMessage();
        	msg.setJMSReplyTo(tt);
        	output.send((Message)msg);
        	
            for (int which=0; which<topicCount; which++) {
                rmsg = (topicCount==1 ? cons : msgcon[which]).receive(500);
    	        replyto = rmsg.getJMSReplyTo();
            }
            msg.clearBody();
	        msg.clearProperties();	 
	        
	        /*
	         * Consumer use received tmp topic to create a new producer to ack
	         */
	        if (replyto != null) {        
	            MessageProducer tp = session.createProducer(replyto);	      
	            TextMessage jmsg = session.createTextMessage("It is temporaryTopic test");
        	    tp.send((Message)jmsg);
	        }
	        
	        Message ack = tempRev.receive(500);
	        assertEquals("It is temporaryTopic test", ((TextMessage)ack).getText());
	        
       }catch (JMSException je) {
         	je.printStackTrace(System.err);
       }	    
    }
    
    /*
     * Test that we construct the topic name correctly.  This uses internal
     * values to reset the session TemporaryTopic property.
     */
    public void testTempTopicNames() throws Exception {
        Destination topic;
        
        session.ttopicName = null;
        session.putInternal("TemporaryTopic", null);
        topic = session.createTemporaryTopic();
        assertTrue(((Topic)topic).getTopicName().indexOf("TT") == 1);

        session.ttopicName = null;
        session.putInternal("TemporaryTopic", "topic_");
        topic = session.createTemporaryTopic();
        assertTrue(((Topic)topic).getTopicName().indexOf("topic_") == 0);

        session.ttopicName = null;
        session.putInternal("TemporaryTopic", "/topic_");
        topic = session.createTemporaryTopic();
        assertTrue(((Topic)topic).getTopicName().indexOf("topic_") > 0);

    }
 
    public void testJMSTemporaryTopicDurable() {
    	try {
    		session.createDurableSubscriber(tt, "test1");
    		Assert.fail("Cannot create durable subscription for temporary topic");
    	} catch (JMSException e) {
    		
    	}

    	try {
    		session.createDurableSubscriber(tt, "test2", null, true);
    		Assert.fail("Cannot create durable subscription for temporary topic");
    	} catch (JMSException e) {    		
    	}
    	

    }
    
    public void testJMSTemporaryTopicSessionClose() {
        if (verbose)
            System.out.println("*TESTCASE: " + this.getName());
    	
    	try {
	        conn.close();
    	} catch (JMSException je) {
    		je.printStackTrace(System.err);
    	}
    }
    
    /*
     * Main test 
     */
    public static void main(String args[]) {      
        try {
        	new ImaJmsTemporaryTopicTest().testJMSTemporaryTopicSetup();
            new ImaJmsTemporaryTopicTest().testJMSTemporaryTopic();
            new ImaJmsTemporaryTopicTest().testTempTopicNames();
            new ImaJmsTemporaryTopicTest().testJMSTemporaryTopicDurable();
            new ImaJmsTemporaryTopicTest().testJMSTemporaryTopicSessionClose();
        } catch (Exception e){
            e.printStackTrace(System.err);
        }                 
    } 
}
