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
package com.ibm.ima.jms.impl;

import java.util.Set;

import javax.jms.JMSException;
import javax.jms.Session;
import javax.jms.TopicConnectionFactory;

import junit.framework.TestCase;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaClient;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaJms;
import com.ibm.ima.jms.impl.ImaPropertiesImpl;
import com.ibm.ima.jms.impl.ImaSession;

public class ImaConnectionTest extends TestCase {
    public ImaConnectionTest() {
        super("IMA JMS Client connection test");
    }
    
    /*
     * Test for getStringList
     */
    public void testStringList() throws Exception {
        String [] slist = ImaJms.getStringList("abc");
        assertEquals(1, slist.length);
        assertEquals("abc", slist[0]);
        
        slist = ImaJms.getStringList("abc def");
        assertEquals(2, slist.length);
        assertEquals("abc", slist[0]);
        assertEquals("def", slist[1]);

        slist = ImaJms.getStringList("abc ;\tdef  ");
        assertEquals(2, slist.length);
        assertEquals("abc", slist[0]);
        assertEquals("def", slist[1]);

        slist = ImaJms.getStringList("  ; abc def;");
        assertEquals(2, slist.length);
        assertEquals("abc", slist[0]);
        assertEquals("def", slist[1]);
    }
    
    /*
     * Test for parse options
     */
    public void testParseOptions() throws Exception {
        String [] options;
        String    val;
        
        options = ImaJms.parseOptions("abc=def,ghi,lmn=3,jgk==xx");
        assertEquals(8, options.length);
        assertEquals("abc", options[0]);
        assertEquals("def", options[1]);
        assertEquals("ghi", options[2]);
        assertEquals(null,  options[3]);
        assertEquals("lmn", options[4]);
        assertEquals("3",   options[5]);
        assertEquals("jgk", options[6]);
        assertEquals("=xx", options[7]);
        
        val = ImaJms.getStringKeyValue(options, "abc");
        assertEquals("def", val);
        
        val = ImaJms.getStringKeyValue(options, "def");
        assertEquals(null, val);
        
        val = ImaJms.getStringKeyValue(options, "jgk");
        assertEquals("=xx", val);
        
        assertTrue(ImaJms.stringKeyValueExists(options, "ghi"));
        assertFalse(ImaJms.stringKeyValueExists(options, "def"));
    }
    
    
    /*
     * Create a session and text various properties 
     */
    public void testcreateSession() throws Exception {
    	String clientID = "ClientId1";
    	TopicConnectionFactory factory = ImaJmsFactory.createTopicConnectionFactory();
    	ImaProperties props = (ImaProperties)factory;
    	try {
            ((ImaProperties)factory).addValidProperty("abc", "K");
        } catch (Exception e) {
        	e.printStackTrace();
            assertTrue(e instanceof IllegalArgumentException);
        }
    	props.put("ClientID", clientID);
    	
    	/*
    	 * Test properties for IMAJmsPropertiesImpl
    	 */
    	props.clear();
        assertEquals(null, props.put("ClientID", clientID));
        assertEquals(1, props.size());
        Set<String> propset = (Set<String>)props.propertySet();
        assertEquals(1, propset.size());
        assertEquals(true, propset.contains("ClientID"));
        assertEquals(false, propset.contains("LogLevel"));
        assertEquals(true, props.exists("ClientID"));
        
    	ImaConnection conn = new ImaConnection(true);
    	conn.props.putAll(((ImaPropertiesImpl)props).props);
    	
    	/*
    	 * Test properties for IMAReadonlyProperties
    	 */
    	assertEquals(1, conn.size());
    	propset = conn.propertySet();
        assertEquals(1, conn.size());
    	assertEquals("_TEST_", conn.getClientID());
    	try {
    	    conn.clear();
    	} catch (Exception e) {
    	    assertTrue(e instanceof IllegalStateException);
    	}
    	try {
            conn.addValidProperty("abc", "S");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalStateException);
        }
    	try {
            conn.remove("Name");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalStateException);
        }
    	((ImaConnection)conn).client = ImaTestClient.createTestClient(conn);
    	
    	/*
    	 * Test session constructor
    	 */
    	Session session = conn.createSession(false, 1);
    	/*If transacted is false, then ackmode will be returned*/
    	assertEquals(session.getAcknowledgeMode(),1);
    	assertEquals(session.getTransacted(),false);
    	
    	session.close();
        assertEquals(true, ((ImaSession)session).get("isClosed"));
    	
    	session = conn.createSession(true, 1);
    	/*If transacted is true, then ackmode is zero*/
    	assertEquals(session.getAcknowledgeMode(),0);
    	assertEquals(session.getTransacted(),true);
    	
    	session.close();
    	
    	conn.close();
    	assertEquals(true, conn.get("isClosed"));
    }
    
    /*
     * Create multiple connections and multiple sessions and check that no two client IDs are the same
     */
    public void testuniqueclientid() throws Exception {
        TopicConnectionFactory factory = ImaJmsFactory.createTopicConnectionFactory();
        ImaProperties props = (ImaProperties)factory;
        
        /*
         * Test properties for IMAJmsPropertiesImpl
         */
        props.clear();
        assertEquals(false, props.exists("ClientID"));
        
        ConnThread[] ct = new ConnThread [1000];
        ImaConnection[] conn = new ImaConnection [1000];
        Thread[] t = new Thread[1000];
        
        for (int i = 0; i < 1000; i++) {
            ct[i] = new ConnThread();
            conn[i] = ct[i].getConn();
            assertEquals(null, conn[i].getClientID());
        }
        
//        for (int i = 0; i < 999; i++) {
//            for (int j=i+1; j<1000; j++) {
//                if (conn[i].hashCode() == conn[j].hashCode()) {
//                    System.out.println("Equal hashcode: " + conn[i].hashCode() + " " + i + ", " + j);
//                }
//            }    
//        }
        
        for (int i = 0; i < 1000; i++) {
            t[i] = new Thread (ct[i]);
            t[i].start();
            String id = null;
            while (id == null) {
                id = conn[i].getClientID();
            }
        }
        
        for (int i = 0; i < 999; i++) {
            for (int j = i + 1; j < 1000; j++) {
                String idi = conn[i].getClientID();
                String idj = conn[j].getClientID();

                if (idi.equals(idj)) {
                    System.out.println(idi + " (" + i + ")" + (idi.equals(idj) ? " = ": " != ") + idj + " (" + j +")");
                    System.out.println("Return from hashCode(): conn[" + i +"]: " + conn[i].hashCode() + " conn[" + j +"]: " + conn[j].hashCode());
                    System.out.println("conn[" + i +"]: " + conn[i].toString(ImaConstants.TS_All));
                    System.out.println("conn[" + j +"]: " + conn[j].toString(ImaConstants.TS_All));
                    System.out.println("conn[" + i +"]: " + conn[i] + (conn[i]==conn[j]? " = ":" != " + "conn[" + j +"]: ") + conn[j]); 
                }
                assertFalse(idi.equals(idj));
            }
        }
        for (int i = 0; i < 1000; i++) {
            conn[i].close();
            while(((ImaProperties)conn[i]).getBoolean("isClosed", false) == false) {
                try { Thread.sleep(5); } catch (InterruptedException ex) {};
            }
            t[i].interrupt();
            t[i].join();
        }
    }
    
    /*
     * Test make version
     */
    public void testMakeVersion() throws Exception {
    	int ver;
    	ver = ImaClient.makeVersion("1.0.0.0");
    	assertEquals(1000000, ver);
    	ver = ImaClient.makeVersion("1.2.3.4");
    	assertEquals(1020304, ver);
    	ver = ImaClient.makeVersion("11.22.33.44");
    	assertEquals(11223344, ver);
    	ver = ImaClient.makeVersion("1.2");
    	assertEquals(102, ver);
    }
   
    
    /*
     * Main test 
     */
    public static void main(String args[]) {
        try {
            new ImaConnectionTest().testStringList();
            new ImaConnectionTest().testParseOptions();
            new ImaConnectionTest().testcreateSession();
            new ImaConnectionTest().testuniqueclientid();
            new ImaConnectionTest().testMakeVersion();
        } catch (Exception e) {
            e.printStackTrace();
        } 
    }
    public class ConnThread implements Runnable {
        ImaConnection conn = null;
        
        ConnThread() {
            try {
                conn = new ImaConnection(false);  
                ((ImaConnection)conn).client = ImaTestClient.createTestClient(conn);
            }
            catch (Exception e) {
                assertTrue(false);
                e.printStackTrace();
              }
        }

        ImaConnection getConn() {
            int majorVersion = 0;
 
            while(majorVersion == 0)
                try {
                    majorVersion = ((ImaConnection)conn).metadata.getProviderMajorVersion();
                } catch (Exception ex) {}
                if (majorVersion == 0)
                    try { Thread.sleep(5); } catch (InterruptedException ex) {};
            return conn;
        }
        
        @Override
        public void run() {
            try {
                conn.connect();
            } catch (JMSException e) { /* Ignore */ }
        }
    }

}
