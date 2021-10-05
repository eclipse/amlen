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

import com.ibm.ima.jms.impl.ImaLike;

import junit.framework.TestCase;

public class ImaLikeTest extends TestCase {
    public ImaLikeTest() {
        super("IMA JMS Client Like test");
    }
    
    /*
     * Test bytes message (common to all message types)
     */
    public void testLike() throws Exception {
        ImaLike rule;
        rule = new ImaLike("abc");
        assertEquals(true, rule.match("abc"));
        assertEquals(false, rule.match("def"));
        assertEquals(false, rule.match("abcd"));
        assertEquals(false, rule.match(""));
        
        rule = new ImaLike("%");
        assertEquals(true, rule.match(""));
        assertEquals(true, rule.match("abcdefg"));
        
        rule = new ImaLike("%", '$');
        assertEquals(true, rule.match(""));
        assertEquals(true, rule.match("abcdefg"));
        
        rule = new ImaLike("a$%", '$');
        assertEquals(false, rule.match("a$%"));
        assertEquals(true, rule.match("a%"));
        
        rule = new ImaLike("_");
        assertEquals(false, rule.match(""));
        assertEquals(true, rule.match("a"));
        assertEquals(false, rule.match("abc"));
        
        rule = new ImaLike("a_b");
        assertEquals(true, rule.match("acb"));
        assertEquals(false, rule.match("ab"));
        
        rule = new ImaLike("%abc");
        assertEquals(true, rule.match("abc"));
        assertEquals(true, rule.match("junkabc"));
        assertEquals(false, rule.match("cab"));
        
        rule = new ImaLike("ab%cd");
        assertEquals(true, rule.match("abxxxcd"));
        assertEquals(true, rule.match("abcd"));
        assertEquals(true, rule.match("abandabuncdofjunkcd"));
        assertEquals(false, rule.match("abb"));
        assertEquals(true, rule.match("abcdcd"));
        
        rule = new ImaLike("ab%");
        assertEquals(true, rule.match("abc"));
        assertEquals(true, rule.match("ab"));
        
        rule = new ImaLike("%a");
        assertEquals(true, rule.match("gaba"));

        rule = new ImaLike("%_a");
        assertEquals(true, rule.match("gaba"));

        rule = new ImaLike("%__a");
  //    System.out.println(rule.dumpLike());
        assertEquals(true, rule.match("gaba"));

        rule = new ImaLike("%___a");
        assertEquals(true, rule.match("gaba"));

        rule = new ImaLike("%_%__a");
        assertEquals(true, rule.match("gaba"));

        rule = new ImaLike("%%%%a");
        assertEquals(true, rule.match("gaba"));

        rule = new ImaLike("%_._%");
        assertEquals(true, rule.match("rmm.1"));
        assertEquals(false, rule.match("rmm."));
        
        /*
         * Check that specifying a null match sting generates an exception.
         */
        try {
            rule.match(null);
            assertTrue(false);
        } catch (Exception e) {
            assertTrue(e instanceof NullPointerException);
        }
        try {
            rule = new ImaLike(null);
            assertTrue(false);
        } catch (Exception e) {
            assertTrue(e instanceof NullPointerException);
        }
    }
    
    /*
     * Main test 
     */
    public static void main(String args[]) {
        try {
            new ImaLikeTest().testLike();
        } catch (Exception e) {
            e.printStackTrace();
        } 
    }
}
