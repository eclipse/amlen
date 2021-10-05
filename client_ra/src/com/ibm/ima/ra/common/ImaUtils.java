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
package com.ibm.ima.ra.common;

import javax.jms.Connection;
import javax.jms.JMSException;

import com.ibm.ima.jms.impl.ImaJmsExceptionImpl;

/** 
 * This class contains common utility method for the RA.
 */
public class ImaUtils {
    /**
     * Checks that the IBM MessageSight appliance is running a valid version for interoperation with the RA.
     * @param conn A JMS Connection object representing the physical connection to the MessageSight appliance
     * @throws JMSException if the MessageSight appliance version is not at least 1.1
     */
	public static void checkServerCompatibility(Connection conn) throws JMSException {
        if ((conn.getMetaData().getProviderMajorVersion() < 1) || ((conn.getMetaData().getProviderMajorVersion() == 1) && (conn.getMetaData().getProviderMinorVersion() < 1))) {
        	/* 
        	 * We are going to reject this connection becuase it is not compatible with the RA.  
        	 * So clean up by closing the connection before we throw the exception for the incompatibility.
        	 */
        	String version = conn.getMetaData().getProviderVersion();
        	try {
        		conn.close();
        	} catch (JMSException ex) {
        		/* Ignore */
        	}
        	conn = null;
        	throw new ImaJmsExceptionImpl("CWLNC2520", "The connection was rejected because IBM MessageSight version {0} is not compatible with the resource adapter.", version);
        }
	}

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
