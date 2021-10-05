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

package com.ibm.ima.plugin.unittest;


import com.ibm.ima.plugin.util.test.ImaBase64Test;


import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;



/*
 * Run all unit tests for the IBM Messaging Appliance JMS client
 */
public class AllTests extends TestCase {
    
    public static Test suite() {
        TestSuite suite = new TestSuite("Test the IBM Messaging Appliance Server Plugin");
        suite.addTestSuite(ImaBase64Test.class);
        return suite;
    }
    
    public static void reportFailure (String className, int line) {
        System.out.println("FAILURE in " + className + " at line " + line);
    }
    
   
}
