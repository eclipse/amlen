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

import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaClient;
import com.ibm.ima.jms.impl.ImaClientTcp;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaJms;
import com.ibm.ima.jms.impl.ImaPropertiesImpl;

/*
 * Run all unit tests for the IBM Messaging Appliance JMS client
 */
public class ImaTestClient {

    public static ImaClient createTestClient(int version, Connection conn) throws JMSException {
        String server = System.getenv("IMAServer");
        String port   = System.getenv("IMAPort");
        String protocol   = System.getenv("IMAProtocol");
        ImaClient client = null;
        if (server == null || server.equals("")) {
        	client = new ImaLoopbackClient(conn);
        	/* Set fake version information for testing */
        	((ImaConnection)conn).metadata.setProviderVersion(version);
        	return client;
        }
        else {
            ImaProperties props = new ImaPropertiesImpl(ImaJms.PTYPE_Connection);
            try {
                props.put("Server", server);
                if (port == null || port.equals(""))
                    port = "16102";
                props.put("Port", port);
                if (protocol == null || protocol.equals(""))
                    protocol = "tcp";
                props.put("Protocol", protocol);
                if (protocol.equals("tcp"))
                    client = new ImaClientTcp((ImaConnection)conn, server, Integer.parseInt(port), props);
                else if (protocol.equals("tcps"))
                    client = new ImaClientSSL((ImaConnection)conn, server, Integer.parseInt(port), props);
                client.startClient();
                return client;
            } catch(Exception ex) {
                ex.printStackTrace();
            }
        }
        return null;
    }
    
    public static ImaClient createTestClient(Connection conn) throws JMSException {
    	return createTestClient(9999999, conn);
    }
}
