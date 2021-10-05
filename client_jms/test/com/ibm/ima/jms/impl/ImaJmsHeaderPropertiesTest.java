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


import java.util.Enumeration;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.ExceptionListener;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Topic;

import junit.framework.TestCase;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaSession;

public class ImaJmsHeaderPropertiesTest extends TestCase implements ExceptionListener {
    static int msgsize = 120;
    static int logLevel = 6;
    static boolean showLog = false;
    static boolean verbose = false;
    
    /*
     * Class objects
     */
    static ConnectionFactory fact;
    static Connection conn;
    static ImaSession session;
    static MessageProducer output;
    static MessageConsumer cons = null;
    static final String topic1 = "OneTopic";
    static byte [] bmsg = new byte[msgsize];
    static String rcvmsg;
    static Topic topic;
	static String xmlmsg = new String("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
			"<!-- \n"                                                     +
			" This file specifies the IP address of the IMA-CM server.\n" +
			" Several test cases use this file as the include file.   \n" +
			" Required Action:\n"                                         +
			" 1. Replace CMIP with the IP address of the IMA-CM server.\n"+
			"-->\n"														  +
			"<param type=\"structure_map\">ip_address=CMIP</param>");
    

    /*
     * Setup
     */
    protected void setUp() throws Exception {
    	super.setUp();
    	ImaProperties props;
        String cid = "testClientID";
        
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
            props.put("ClientID", cid);
            if (AllTests.setConnParams(((ImaProperties)fact))) {
                conn = fact.createConnection();
            } else {
                conn = new ImaConnection(true);
                // When ImaConnection() constructor is used, the clientID is not picked up
                // from the connection factory and cannot be reset once the connection object
                // is created.
                cid = "_TEST_";
                ((ImaConnection)conn).client = ImaTestClient.createTestClient(conn);
            }
            conn.setExceptionListener(this);
            session = (ImaSession) conn.createSession(false, 0);
          
            /*
             * Create some input destinations
             */
            topic = ImaJmsFactory.createTopic(topic1);
            cons = session.createConsumer(topic);
            output = session.createProducer(topic);
            
            conn.start();
            if (System.getenv("IMAServer") == null) {
                ((ImaMessageConsumer)cons).isStopped = false;
            }
        }catch (JMSException je) {
        	je.printStackTrace(System.err);
        }
        assertEquals(cid, conn.getClientID());
    }
    
    /* 
     * @see junit.framework.TestCase#tearDown()
     */
    protected void tearDown() throws Exception {
        super.tearDown();
    }
    
    @SuppressWarnings("unchecked")
	public void testJMSHeaderMetaData() throws JMSException {
    	int i;
        final String [] jmsxprop = {
        		"JMSXGroupID",
        		"JMSXGroupSeq",
        		"JMSXDeliveryCount"
        };
        
        if (verbose)
            System.out.println("*TESTCASE: " + this.getName());
        try {
            assertEquals("Eclipse Amlen", conn.getMetaData().getJMSProviderName());
         // assertEquals("1.1", conn.getMetaData().getJMSVersion());
            if (conn.getClientID().equals("testClientID")) { /* We have connected to a server and will receive server info */
                assertEquals(1, conn.getMetaData().getProviderMajorVersion());
                assertEquals(0, conn.getMetaData().getProviderMinorVersion());
            } else { /* We have NOT connected to a server so test client values are available */
                assertEquals(9, conn.getMetaData().getProviderMajorVersion());
                assertEquals(99, conn.getMetaData().getProviderMinorVersion());
            }
            
            i=0;
            Enumeration e = conn.getMetaData().getJMSXPropertyNames();
            while (e.hasMoreElements()) {
            	if (i >= jmsxprop.length) {
            		fail("JMSX has an extra property than allowed: " + (String)e.nextElement());
            		break;
            	}
            	assertEquals(jmsxprop[i], (String)e.nextElement());
            	i++;
            }

        }catch (JMSException je) {
        	je.printStackTrace(System.err);
        }
    }

    
    
    /*
     * Test ObjectMessage
     * An ObjectMessage can contain any Java object.  The test
     * reads the object casts it to the appropriate type.
     */
    public void testJMSHeaderProperties() throws Exception {
    
    
    /*
     * Test JMS message Header
     *     JMSDestination
     *     JMSDeliveryMode
     *     JMSMessageID
     *     JMSTimestamp
     *     JMSCorrelationID
     *     JMSReplyTo
     *     JMSRedelivered
     *     JMSType
     *     JMSExpiration
     *     JMSPriority
     */
	    Message rmsg;

	   if (verbose)
	       System.out.println("*TESTCASE: " + this.getName());
       try {   
    	   Destination pd = output.getDestination();
        	Message msg = session.createMessage();
        	
        	output.setPriority(5);
        	
        	msg.setJMSCorrelationID("MapCorrelationID_4567");
        	msg.setJMSType("<type=IMA_JMS_$_@_0987/>");
        	msg.setJMSDeliveryMode(javax.jms.DeliveryMode.NON_PERSISTENT); //The setting will be overwritten by the Producer's settting
        	Destination dest = ImaJmsFactory.createTopic("rmm://127.0.0.1:34567");
        	msg.setJMSDestination(dest);
        	msg.setJMSExpiration(234567890);
        	msg.setJMSPriority(9);
        	msg.setJMSRedelivered(false);
        	msg.setJMSReplyTo(dest);
        	msg.setJMSTimestamp(89234);
        	msg.setJMSMessageID("JMSTestID");
        	msg.setStringProperty("JMSXGroupID", "groupIDtest");
        	msg.setIntProperty("JMSXGroupSeq", 2867249);
            output.send((Message)msg);
            
            rmsg = cons.receive(500);
	       
	        String  spr0= rmsg.getJMSCorrelationID();
	        String  spr1= rmsg.getJMSType();
	        int     mode= rmsg.getJMSDeliveryMode();
	        Destination rdest = rmsg.getJMSDestination();
	        Destination replyto = rmsg.getJMSReplyTo();
	        //System.out.print("mode=" + mode);
	        
	        assertEquals("MapCorrelationID_4567", spr0);
	        assertEquals("<type=IMA_JMS_$_@_0987/>", spr1);
	        assertEquals(javax.jms.DeliveryMode.PERSISTENT, mode); //Producer setting overwrite message setting
	        /*
	         * Make sure we can still set Delivery Mode on received msg
	         */
	        rmsg.setJMSDeliveryMode(javax.jms.DeliveryMode.NON_PERSISTENT);
	        int rmode = rmsg.getJMSDeliveryMode();
	        assertEquals(javax.jms.DeliveryMode.NON_PERSISTENT, rmode);
	        
	        assertNotSame(dest.toString(), rdest.toString());
	        assertEquals(pd.toString(), rdest.toString());
	        
	        assertNotSame(234567890, rmsg.getJMSExpiration());
	        assertEquals(0, rmsg.getJMSExpiration());
	        
	        assertNotSame(9, rmsg.getJMSPriority());
	        assertEquals(output.getPriority(), rmsg.getJMSPriority());  //the producer priority
	        
	        //jmstimestamp is set by producer using currentTimeMillis. 
	        assertNotSame(89234, rmsg.getJMSTimestamp());
	        
	        assertEquals(dest.toString(), replyto.toString());
	        
	        assertNotSame("JMSTestID", rmsg.getJMSMessageID());
	        assertEquals("groupIDtest", rmsg.getStringProperty("JMSXGroupID"));
	        assertEquals(2867249, rmsg.getIntProperty("JMSXGroupSeq"));
	        
	        msg.clearBody();
	        msg.clearProperties();	        
       }catch (JMSException je) {
         	je.printStackTrace(System.err);
       }	    
    }
    
    /*
     * Test the following parameters for JMS
     *     DefaultTimeToLive
     *     DisableMessageID
     *     DisableMessageTimestamp
     *     TemporaryTopic
     *     TopicMode
     */
    void doMessageReceive2(int topicCount) throws Exception {
	    Message rmsg;

	   if (verbose)
	       System.out.println("*TESTCASE: " + this.getName());
       try {   
        	Message msg = session.createMessage();
        	
        	output.setDisableMessageID(true);
        	output.setDisableMessageTimestamp(true);
        	output.setPriority(9);
        	output.setTimeToLive(12345);
        	output.setDeliveryMode(javax.jms.DeliveryMode.PERSISTENT);
        	
            output.send((Message)msg);
            
            rmsg = cons.receive(500);
	       
           
	        assertEquals(null, rmsg.getJMSMessageID());
	        assertEquals(0, rmsg.getJMSTimestamp());
	        assertEquals(9, rmsg.getJMSPriority());
	        assertEquals(0, rmsg.getJMSExpiration());  //If Timestamp is disabled, expiration is 0
	        assertEquals(javax.jms.DeliveryMode.PERSISTENT, rmsg.getJMSDeliveryMode());
	        
	        msg.clearBody();
	        msg.clearProperties();	        	        
       }catch (JMSException je) {
         	je.printStackTrace(System.err);
       }	    
    }
    
    public void testJMSHeaderSendReceiveSessionClose() {
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
        	new ImaJmsHeaderPropertiesTest().testJMSHeaderMetaData();
            new ImaJmsHeaderPropertiesTest().testJMSHeaderProperties();
            new ImaJmsHeaderPropertiesTest().testJMSHeaderSendReceiveSessionClose();
        } catch (Exception e){
            e.printStackTrace(System.err);
        }                 
    }


	public void onException(JMSException arg0) {
		// TODO Auto-generated method stub
		
	} 
}
