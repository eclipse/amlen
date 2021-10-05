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

import javax.jms.JMSException;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.ibm.ima.jms.ImaProperties;

/*
 * Run all unit tests for the IBM Messaging Appliance JMS client
 */
public class AllTests extends TestCase {
    static final String runLocalizedTest = "RUN_PSEUDOLOCALIZED_MESSAGES_TEST";
    
    public static Test suite() {
        TestSuite suite = new TestSuite("Test the IBM Messaging Appliance JMS client");
        suite.addTestSuite(ImaJms11Test.class);
        suite.addTestSuite(ImaMessageTest.class);
        suite.addTestSuite(ImaBytesMessageTest.class);
        suite.addTestSuite(ImaMapMessageTest.class);
        suite.addTestSuite(ImaObjectMessageTest.class);
        suite.addTestSuite(ImaStreamMessageTest.class);
        suite.addTestSuite(ImaTextMessageTest.class);
        suite.addTestSuite(ImaLikeTest.class);
        suite.addTestSuite(ImaSelectorTest.class);
        suite.addTestSuite(ImaDestinationTest.class);
        suite.addTestSuite(ImaJmsExceptionTest.class);
        suite.addTestSuite(ImaConnectionTest.class);
        suite.addTestSuite(ImaJmsSendReceiveTest.class);
        suite.addTestSuite(ImaJndiTest.class);
        suite.addTestSuite(ImaJmsHeaderPropertiesTest.class);
        suite.addTestSuite(ImaJmsTemporaryTopicTest.class);
        suite.addTestSuite(ImaNoAckTest.class);
        suite.addTestSuite(UtilsTest.class);
        if (runLocalizedTest != null && runLocalizedTest.equals("true"))
            suite.addTestSuite(ImaTestLocalizedEx.class);
        else
        	System.err.println("ImaTestLocalizedEx will not be run because runLocalizedTest=\"" + runLocalizedTest + "\"");
        return suite;
    }
    
    public static void reportFailure (String className, int line) {
        System.out.println("FAILURE in " + className + " at line " + line);
    }
    
    public static boolean setConnParams (ImaProperties props) {
        String server = System.getenv("IMAServer");
        String port   = System.getenv("IMAPort");
        String protocol   = System.getenv("IMAProtocol");
        if ((server != null) && (server.length() > 0)) {
            //ImaProperties props = new ImaPropertiesImpl(ImaJms.PTYPE_Connection);
            try {
                props.put("Server", server);
                if (port == null || port.equals(""))
                    port = "16102";
                props.put("Port", port);
                if (protocol == null || protocol.equals(""))
                    protocol = "tcp";
                props.put("Protocol", protocol);
                return true;
            } catch(JMSException ex) {
                ex.printStackTrace();
            }
        }
        return false;
    }
}
