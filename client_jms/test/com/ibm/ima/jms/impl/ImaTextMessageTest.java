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
import javax.jms.JMSException;
import javax.jms.Session;
import javax.jms.TextMessage;

import junit.framework.TestCase;


public class ImaTextMessageTest extends TestCase {

    public static final String txt1 = "This is some text \u0234\u8081\ud804\udc03 and some more";
    public static final String bad1 = "abc \udc03\ud804";
    public static String longstr;
    
    static {
        longstr = txt1;
        for (int i=0; i<5; i++)
            longstr = longstr + longstr;
    }
    
    /*
     * Text the IBM Messaging Appliance JMS client text message
     */
    public ImaTextMessageTest() {
        super("IBM Messaging Appliance JMS client text message test");
    }
    
    /*
     * Test bytes message (common to all message types)
     */
    public void testText() throws Exception {
    	Connection conn = new ImaConnection(true);
        ((ImaConnection)conn).client = ImaTestClient.createTestClient(conn);
        Session sess = conn.createSession(false, 0);
        
        TextMessage tmsg = sess.createTextMessage();
        tmsg.setText(txt1);
        assertEquals(txt1, tmsg.getText());
        tmsg.setText(longstr);
        assertEquals(longstr, tmsg.getText());
        
        
        tmsg = sess.createTextMessage(txt1);
        assertEquals(txt1, tmsg.getText());
        
        //System.out.println(ImaJmsObject.toString(tmsg, "*b"));
        
        try {
            tmsg = sess.createTextMessage(bad1);
            assertTrue(false);
        } catch (JMSException jmse) {
        }
        
        conn.close();
    }    
    
    /*
     * Main test 
     */
    public static void main(String args[]) {
        try {
            new ImaTextMessageTest().testText();
        } catch (Exception e) {
            e.printStackTrace();
        } 
    }
}
