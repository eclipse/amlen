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
package com.ibm.ima.jms.impl;

import javax.jms.JMSException;
import javax.transaction.xa.Xid;

public class ImaTestHelper {
	public static ImaConnection getLoopbackConnection(boolean setClientId) {
		return getLoopbackConnection(9999999, setClientId);
	}
    public static ImaConnection getLoopbackConnection(int version, boolean setClientId) {
        ImaConnection conn = null;
        try {
            conn = new ImaConnection(setClientId);
            ((ImaConnection)conn).isloopbackclient = true;
            ((ImaConnection)conn).client = ImaTestClient.createTestClient(version, conn);
            return conn;
        } catch(JMSException ex) {
            throw new RuntimeException(ex);
        }
    }
    
    public static Xid getXid() {
        Xid xid = (Xid) new ImaXidImpl(1234, "abc".getBytes(), "defg".getBytes());
        return xid;
    }
}
