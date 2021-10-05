/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.ra;

import javax.jms.JMSException;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaTestHelper;
import com.ibm.ima.jms.impl.ImaTestLocalizedEx;
import com.ibm.ima.ra.inbound.ImaActivationSpec;
import com.ibm.ima.ra.inbound.InboundTestHelper;

public class AllTests extends TestCase {
	static final String runLocalizedTest = "RUN_PSEUDOLOCALIZED_MESSAGES_TEST";
	
    public static Test suite() {
        TestSuite suite = new TestSuite("Test the IBM Messaging Appliance JMS resource adapter");
        suite.addTestSuite(TestRa.class);
        suite.addTestSuite(TestRaBeanInfo.class);
        suite.addTestSuite(TestRaMetaData.class);
        suite.addTestSuite(TestRaRecovXAResource.class);
        suite.addTestSuite(com.ibm.ima.ra.inbound.TestActSpec.class);
        suite.addTestSuite(com.ibm.ima.ra.inbound.TestASBeanInfo.class);
        if (runLocalizedTest != null && runLocalizedTest.equals("true"))
        	suite.addTestSuite(ImaTestLocalizedEx.class);
        return suite;
    }
    
    public static void reportFailure (String className, int line) {
        System.out.println("FAILURE in " + className + " at line " + line);
    }
    
    public static boolean setConnParams(ImaActivationSpec actSpec) {
    	return setConnParams(9999999, actSpec);
    }
    
    public static boolean setConnParams(int version, ImaActivationSpec actSpec) {
        ImaProperties props = actSpec.getImaFactory();
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
        ImaConnection conn = ImaTestHelper.getLoopbackConnection(version, false);
        try {
            props.put("Server","127.0.0.1");
            props.put("Port", "16102");
            props.put("Protocol", "tcp");
        } catch(JMSException ex) {
            ex.printStackTrace();
        }
        InboundTestHelper.setLoopbackConn((ImaActivationSpec) actSpec, conn);
        return false;
    }
}
