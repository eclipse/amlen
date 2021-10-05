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

import java.io.Externalizable;
import java.io.IOException;
import java.io.ObjectInput;
import java.io.ObjectOutput;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.jms.JMSException;
import javax.resource.ResourceException;
import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
import javax.transaction.xa.Xid;

import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaConnectionFactory;
import com.ibm.ima.jms.impl.ImaIOException;
import com.ibm.ima.jms.impl.ImaTrace;
import com.ibm.ima.jms.impl.ImaXAException;
import com.ibm.ima.jms.impl.ImaXASession;
import com.ibm.ima.ra.common.ImaXAObserver.XAEvent;
import com.ibm.ima.ra.inbound.ImaMessageEndpointNonAsf.NonAsfWorkImpl;

public final class ImaXARProxy implements XAResource, Externalizable {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    private static final long            serialVersionUID = 3970972246049425880L;
    /** the 'real' XAResouce */
    private XAResource           xar                = null;
    /** list of XAObservers */
    private final ImaXAObserver            observer;

    private boolean                        rollbackOnly     = false;

    private ImaConnectionFactory           recoveryFactory  = null;
    private ImaXASession                   session            = null;
    private String                         userName           = null;
    private String                         password           = null;
    private ImaConnection                  recoveryConnection = null;
    private int                            traceLevel = -1;
    protected final AtomicBoolean          isClosed = new AtomicBoolean(false);
    private final NonAsfWorkImpl           work;

    public ImaXARProxy() {
        observer = null;
        work = null;
    }

    public ImaXARProxy(ImaFactoryConfig config, ImaXASession sess, ImaXAObserver xao, String trcLvlStr) throws ResourceException {
        if (sess == null) {
            ImaCommException ice = new ImaCommException("CWLNC2220", "An XA resource was not created because a session is not available. This indicates that there is no physical connection available.");
            ImaTrace.traceException(1, ice);
            throw ice;
        }
        recoveryFactory = config.getImaFactory();
        session = sess;
        userName = config.getUser();
        password = config.getPassword();
        xar = session.getXAResource();
        observer = xao;
        work = null;
        if (trcLvlStr != null)
            traceLevel = Integer.valueOf(trcLvlStr);
    }

    public ImaXARProxy(ImaFactoryConfig config, ImaXASession sess, NonAsfWorkImpl wk, String trcLvlStr) {
        recoveryFactory = config.getImaFactory();
        session = sess;
        userName = config.getUser();
        password = config.getPassword();
        xar = session.getXAResource();
        observer = null;
        work = wk;
        if (trcLvlStr != null)
            traceLevel = Integer.valueOf(trcLvlStr);
    }

    
    
    /*
     * Commit the global transaction.
     * 
     * @see javax.transaction.xa.XAResource#commit(javax.transaction.xa.Xid, boolean)
     */
    public void commit(Xid xid, boolean onePhase) throws XAException {
        try {
            if (rollbackOnly) {
                if (onePhase) {
                    // do normal rollback processing and throw the XAException will rollback condition
                    // state will be set by rollback method
                    rollback(xid);
                    // reset the flag in case this is used again
                    rollbackOnly = false;
                    ImaXAException xae = new ImaXAException("CWLNC2420", XAException.XA_RBROLLBACK, "A rollback was performed during a call to commit() because rollbackOnly was specified.");
                    if (recoveryConnection != null)
                        recoveryConnection.traceException(1, xae);
                    else
                        ImaTrace.traceException(1, xae);
                    throw xae;
                }
                // this is an impossible situation to be in so FFST
                ImaXAException xae = new ImaXAException("CWLNC2421", XAException.XAER_RMFAIL, "A rollback failure occurred when rollbackOnly was specified at the time commit() was called. This might indicate a problem with the application server.");
                if (recoveryConnection != null)
                    recoveryConnection.traceException(1, xae);
                else
                    ImaTrace.traceException(1, xae);
                throw xae;
            }
            try {
            	checkClosed(XAException.XAER_RMFAIL);
                xar.commit(xid, onePhase);
                notifyObservers(XAEvent.COMMIT);
            } catch (XAException xe) {
                notifyObservers(XAEvent.FAILED);
                throw xe;
            }
        } finally {
            closeRecoveryConnection();
        }
    }

    /*
     * End the global transaction
     * 
     * @see javax.transaction.xa.XAResource#end(javax.transaction.xa.Xid, int)
     */
    public void end(Xid xid, int flags) throws XAException {
    	try {
        	checkClosed(XAException.XA_RBROLLBACK);
    	} catch (XAException xae) {
    		ImaTrace.trace(1, "End failed because XAResource " + ImaXARProxy.this + "is closed.");
    		ImaTrace.traceException(1, xae);
    		return;
    	}
        try {
            xar.end(xid, flags);
            notifyObservers(XAEvent.END);
        } catch (XAException xe) {
            notifyObservers(XAEvent.FAILED);
            // rethrowing depends on the error code
            // XA_RBROLLBACK is a notifier that the transaction has been backed out by the RM,
            // so we can just log it rather than rethrowing.
            if (xe.errorCode != XAException.XA_RBROLLBACK) {
                // not XA_RBROLLBACK, so re-throw the exception
            	ImaTrace.trace(1, "End failed with XA error code: " + xe.errorCode);
                throw xe;
            }
        }
    }

    /*
     * Forget the global transaction
     * 
     * @see javax.transaction.xa.XAResource#forget(javax.transaction.xa.Xid)
     */
    public void forget(Xid xid) throws XAException {
        try {
        	checkClosed(XAException.XAER_RMFAIL);
            xar.forget(xid);
            notifyObservers(XAEvent.FORGET);
        } catch (XAException xe) {
            notifyObservers(XAEvent.FAILED);
            throw xe;
        } finally {
            closeRecoveryConnection();
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.transaction.xa.XAResource#getTransactionTimeout()
     */
    public int getTransactionTimeout() throws XAException {
        try {
            return xar.getTransactionTimeout();
        } catch (XAException xe) {
            notifyObservers(XAEvent.FAILED);
            throw xe;
        }
    }

    /*
     * Check if two resources are from the same resource manager.
     * 
     * @see javax.transaction.xa.XAResource#isSameRM(javax.transaction.xa.XAResource)
     */
    public boolean isSameRM(XAResource xares) throws XAException {
        return false;
    }

    /*
     * Prepare to commit a global transaction.
     * 
     * @see javax.transaction.xa.XAResource#prepare(javax.transaction.xa.Xid)
     */
    public int prepare(Xid xid) throws XAException {
        if (rollbackOnly) {
            try {
                rollback(xid);
                // reset the flag in case this is used again
                rollbackOnly = false;
                ImaXAException xae = new ImaXAException("CWLNC2422", XAException.XA_RBROLLBACK, "A rollback was performed during a call to prepare() because rollbackOnly was specified.");
                if (recoveryConnection != null)
                    recoveryConnection.traceException(1, xae);
                else
                    ImaTrace.traceException(1, xae);
                throw xae;
            } catch (XAException e) {
                // need to check to see if we have any of the error codes from rollback that can't
                // be thrown from prepare. These amount to the Heuristic Error codes
                // which in theory should NEVER be thrown without preparing first (which we haven't
                // yet done).
                switch (e.errorCode) {
                case XAException.XA_HEURCOM:
                case XAException.XA_HEURHAZ:
                case XAException.XA_HEURMIX:
                case XAException.XA_HEURRB:
                    ImaXAException xae = new ImaXAException("CWLNC2423", XAException.XAER_RMFAIL, e, "A rollback exception occurred when rollbackOnly was specified at the time prepare() was called.");
                    if (recoveryConnection != null)
                        recoveryConnection.traceException(1, xae);
                    else
                        ImaTrace.traceException(1, xae);
                    throw xae;
                default:
                	if (e instanceof ImaXAException)
                        throw e;
                	else
                		throw new ImaXAException("CWLNC2423", XAException.XAER_RMFAIL, e, "A rollback exception occurred when rollbackOnly was specified at the time prepare() was called.");
                }
            }
        }
        try {
        	checkClosed(XAException.XAER_NOTA);
            int rc = xar.prepare(xid);
            notifyObservers(XAEvent.PREPARE);
            return rc;
        } catch (XAException xe) {
            notifyObservers(XAEvent.FAILED);
            throw xe;
        }
    }

    /*
     * Recover a global transaction.
     * 
     * @see javax.transaction.xa.XAResource#recover(int)
     */
    public Xid[] recover(int flag) throws XAException {
        try {
        	checkClosed(XAException.XAER_RMFAIL);
            Xid[] result = xar.recover(flag);
            notifyObservers(XAEvent.RECOVER);
            return result;
        } catch (XAException xe) {
            notifyObservers(XAEvent.FAILED);
            throw xe;
        }
    }

    /*
     * Rollback a global transaction.
     * 
     * @see javax.transaction.xa.XAResource#rollback(javax.transaction.xa.Xid)
     */
    public void rollback(Xid xid) throws XAException {
        try {
        	checkClosed(XAException.XAER_RMFAIL);
            xar.rollback(xid);
            notifyObservers(XAEvent.ROLLBACK);
        } catch (XAException xe) {
            notifyObservers(XAEvent.FAILED);
            // rethrowing depends on the error code
            // XA_RBROLLBACK is a notifier that the transaction has been backed out by the RM,
            // so we can just log it rather than rethrowing.
            if (recoveryConnection != null)
                recoveryConnection.traceException(1, xe);
            else {
                ImaTrace.traceException(1, xe);
            }
            if (xe.errorCode != XAException.XA_RBROLLBACK) {
                // not XA_RBROLLBACK, so re-throw the exception
                throw xe;
            }
        } finally {
            closeRecoveryConnection();
        }
    }

    /*
     * Set the global transaction timeout.
     * 
     * @see javax.transaction.xa.XAResource#setTransactionTimeout(int)
     */
    public boolean setTransactionTimeout(int seconds) throws XAException {
        try {
            return xar.setTransactionTimeout(seconds);
        } catch (XAException xe) {
            notifyObservers(XAEvent.FAILED);
            throw xe;
        }
    }

    /*
     * Start a global transaction.
     * 
     * @see javax.transaction.xa.XAResource#start(javax.transaction.xa.Xid, int)
     */
    public void start(Xid xid, int flags) throws XAException {
        try {
        	checkClosed(XAException.XAER_RMFAIL);
            xar.start(xid, flags);
            notifyObservers(XAEvent.START);
        } catch (XAException xe) {
            notifyObservers(XAEvent.FAILED);
            throw xe;
        }
    }
    /**
     * Sets if this the prepare/commit calls should force a rollback i.e. always rollback
     * 
     * @param rollbackOnly boolean - true meaning always rollback
     */
    public void setRollbackOnly() {
        rollbackOnly = true;
    }

    /*
     * Modify the observer for a global transaction
     */
    private void notifyObservers(XAEvent event) {
        if (observer != null) {
            observer.onXAEvent(event);
        }
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.io.Externalizable#writeExternal(java.io.ObjectOutput)
     */
    public void writeExternal(ObjectOutput out) throws IOException {
        out.writeObject(recoveryFactory);
        out.writeUTF(userName);
        out.writeUTF(password);
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.io.Externalizable#readExternal(java.io.ObjectInput)
     */
    public void readExternal(ObjectInput in) throws IOException, ClassNotFoundException {
        recoveryFactory = (ImaConnectionFactory) in.readObject();
        userName = in.readUTF();
        password = in.readUTF();
        try {
            /* Neither the connection factory configuration nor the RA configuration
             * have supplied a value so use default of 4.  If they want 0, they
             * must set 0.
             */
            if (traceLevel == -1)
                traceLevel = 4;
            recoveryConnection = (ImaConnection) recoveryFactory.createXAConnection(userName, password, (traceLevel+10));
            session = (ImaXASession) recoveryConnection.createXASession();
            xar = session.getXAResource();
        } catch (JMSException e) {
            IOException ioex = new ImaIOException("CWLNC2921", e, "Failed to connect to MessageSight for recovery due to an exception.");
            if (recoveryConnection != null)
                recoveryConnection.traceException(1, ioex);
            else
                ImaTrace.traceException(1, ioex);
            throw ioex;
        }
    }
    
    public void close() {
    	isClosed.set(true);
    }
    
    private void closeRecoveryConnection() {
        if (recoveryConnection != null) {
            try {
                recoveryConnection.close();
            } catch (JMSException e) {
                ImaTrace.traceException(1, e);
            }
            recoveryConnection = null;
        }
    }
    
    private void checkClosed(int xaErr) throws XAException {
    	try {
    	if ((work != null) && isClosed.get()) {
    		session = (ImaXASession)work.refreshXaSession();
    		xar = session.getXAResource();
    	} 
    	} catch (JMSException je) {
    		throw new ImaXAException("CWLNC2424", xaErr, je, "Failed to reinitialize XAResource.");
    	}
    	return;
    }

}
