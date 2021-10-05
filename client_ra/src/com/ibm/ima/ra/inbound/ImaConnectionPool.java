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
package com.ibm.ima.ra.inbound;

import javax.jms.Connection;
import javax.jms.JMSException;

import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaConnectionFactory;
import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.ra.common.ImaUtils;

public class ImaConnectionPool {

    public ImaConnection getConnection(ImaActivationSpec spec) throws JMSException {
        if (spec.getLoopbackConn() == null) {
            ImaConnectionFactory cf = spec.getImaFactory();
            int connTraceLevel = Integer.valueOf(spec.getTraceLevel());
            /* Neither the activation spec configuration nor the RA configuration
             * have supplied a value so use default of 4.  If they want 0, they
             * must set 0.
             */
            if (connTraceLevel == -1)
                connTraceLevel = 4;
            ImaTrace.trace(1, "Setting trace level for " + cf + " to "+connTraceLevel);
            Connection conn = cf.createConnection(spec.getUser(), spec.getPassword(), (connTraceLevel + 10));
            ImaUtils.checkServerCompatibility(conn);
            return (ImaConnection)conn;
        }
        return spec.getLoopbackConn();
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
