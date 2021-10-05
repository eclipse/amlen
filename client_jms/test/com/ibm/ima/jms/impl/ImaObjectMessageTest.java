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

import java.util.HashMap;

import javax.jms.Connection;
import javax.jms.JMSException;
import javax.jms.ObjectMessage;
import javax.jms.Session;

import com.ibm.ima.jms.impl.ImaConnection;

import junit.framework.TestCase;


/*
 *
 *
 */
public class ImaObjectMessageTest extends TestCase {
    
    /*
     * Test bytes message (common to all message types)
     */
    @SuppressWarnings("unchecked")
    public void testObject() throws Exception {
    	Connection conn = new ImaConnection(true);
        ((ImaConnection)conn).client = ImaTestClient.createTestClient(conn);
        Session sess = conn.createSession(false, 0);
        
        ObjectMessage omsg = sess.createObjectMessage();
        
        HashMap <String, Object> map = new HashMap <String, Object>();
        HashMap <String, Object> map2;
        map.put("Boolean", true);
        map.put("Byte", (byte)55);
        map.put("Char", 'A');
        map.put("Double", 3.1);
        map.put("Float", -5.0f);
        map.put("Int", 100);
        map.put("Long", 123456789012L);
        map.put("Short", (short)-55);
        map.put("String", "This is a test");
        omsg.setObject(map);
        try {
        map2 = (HashMap) omsg.getObject();
        } catch (JMSException ex) {
            assertTrue(ex instanceof ImaJmsSecurityException);
        } finally {
        	conn.close();
        }
// Defect 136634 - Saving commented out code in case we eventually find a better way
// to manage the object deserialization security risk.  Currently, we're running JUnit
// with default setting for IMAEnforceObjectMessageSecurity so getObject() always throws
// an exception.  If we're able to restore the commented out code here, then the try/catch/finally
// block preceding this comment can be deleted.
//        map2 = (HashMap) omsg.getObject();
//        assertTrue(map.equals(map2));
//
//        omsg = sess.createObjectMessage(map);
//        map2 = (HashMap) omsg.getObject();
//        assertTrue(map.equals(map2));
//
//        conn.close();
    }

    /*
     * Main test 
     */
    public static void main(String args[]) {
        try {
            new ImaObjectMessageTest().testObject();
        } catch (Exception e) {
            e.printStackTrace();
        } 
    }
}
