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
package com.ibm.ima.ra.inbound;

import javax.jms.JMSException;

import com.ibm.ima.jms.impl.ImaConnection;

public class InboundTestHelper {  
    public static void setLoopbackConn(ImaActivationSpec spec, ImaConnection conn) {
        spec.setLoopbackConn(conn);
    }
    
    public static ImaConnection getConnFromPool(ImaConnectionPool pool, ImaActivationSpec spec) throws JMSException {
        return pool.getConnection(spec);
    }

}
