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

package com.ibm.ima.ra;

import javax.jms.JMSException;
import javax.resource.spi.InvalidPropertyException;
import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
import javax.transaction.xa.Xid;

import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.jms.impl.ImaXAException;
import com.ibm.ima.jms.impl.ImaXASession;
import com.ibm.ima.ra.inbound.ImaActivationSpec;

final class ImaRecoveryXAResource implements XAResource {

    private final ImaActivationSpec  config;
    private final ImaResourceAdapter ra;

    ImaRecoveryXAResource(ImaResourceAdapter adapter, ImaActivationSpec spec) throws XAException, InvalidPropertyException {
        ra = adapter;
        config = (ImaActivationSpec) spec.cloneConfiguration();
        config.setLoopbackConn(spec.getLoopbackConn());
        String clientID = config.getClientId();
        if (clientID != null) {
            clientID += ".XARecover";
            config.setClientId(clientID);
        }
        config.validate();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.transaction.xa.XAResource#commit(javax.transaction.xa.Xid, boolean)
     */
    public void commit(Xid xid, boolean onePhase) throws XAException {
        XARData xd = new XARData();
        xd.xar.commit(xid, onePhase);
        xd.close();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.transaction.xa.XAResource#end(javax.transaction.xa.Xid, int)
     */
    public void end(Xid xid, int flags) throws XAException {
        // This method should not be called on a ImaRecoveryXAResource. Throwing XAException
        ImaXAException xae = ImaXAException.notAllowed("end");
        traceException(1, xae);
        throw xae;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.transaction.xa.XAResource#forget(javax.transaction.xa.Xid)
     */
    public void forget(Xid xid) throws XAException {
        XARData xd = new XARData();
        xd.xar.forget(xid);
        xd.close();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.transaction.xa.XAResource#getTransactionTimeout()
     */
    public int getTransactionTimeout() throws XAException {
        // This method should not be called on a ImaRecoveryXAResource. Throwing XAException
        ImaXAException xae = ImaXAException.notAllowed("getTransactionTimeout");
        traceException(1, xae);
        throw xae;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.transaction.xa.XAResource#isSameRM(javax.transaction.xa.XAResource)
     */
    public boolean isSameRM(XAResource xares) throws XAException {
        return false;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.transaction.xa.XAResource#prepare(javax.transaction.xa.Xid)
     */
    public int prepare(Xid xid) throws XAException {
        // This method should not be called on a ImaRecoveryXAResource. Throwing XAException
        ImaXAException xae = ImaXAException.notAllowed("prepare");
        traceException(1, xae);
        throw(xae);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.transaction.xa.XAResource#recover(int)
     */
    public Xid[] recover(int flag) throws XAException {
        XARData xd = new XARData();
        Xid[] result = xd.xar.recover(flag);
        xd.close();
        return result;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.transaction.xa.XAResource#rollback(javax.transaction.xa.Xid)
     */
    public void rollback(Xid xid) throws XAException {
        XARData xd = new XARData();
        xd.xar.rollback(xid);
        xd.close();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.transaction.xa.XAResource#setTransactionTimeout(int)
     */
    public boolean setTransactionTimeout(int seconds) throws XAException {
        // This method should not be called on a ImaRecoveryXAResource. Throwing XAException
        ImaXAException xae = ImaXAException.notAllowed("setTransactionTimeout");
        traceException(1, xae);
        throw(xae);
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.transaction.xa.XAResource#start(javax.transaction.xa.Xid, int)
     */
    public void start(Xid xid, int flags) throws XAException {
        // This method should not be called on a ImaRecoveryXAResource. Throwing XAException
        ImaXAException xae = ImaXAException.notAllowed("start");
        traceException(1, xae);
        throw xae;
    }

    private void traceException(int level, Throwable ex) {
        ImaTrace.traceException(1, ex);
    }

    private final class XARData {
        private final ImaConnection connection;
        private final ImaXASession  session;
        private final XAResource    xar;
        // private final ImaX
        XARData() throws XAException {
            try {
                connection = ra.getConnectionPool().getConnection(config);
                session = (ImaXASession) connection.createXASession();
                xar = session.getXAResource();
            } catch (JMSException e) {
                XAException xe = new ImaXAException("CWLNC2402", XAException.XAER_RMFAIL, e, "Failed to initialize XAResource for recovery due to an exception.");
                /* Do not trace here. This error will be traced by caller. */
                throw xe;
            }
        }

        void close() {
            try {
                session.close();
                connection.close();
            } catch (Exception e) {
            }
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
