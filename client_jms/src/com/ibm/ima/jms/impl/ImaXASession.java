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

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.concurrent.atomic.AtomicReference;

import javax.jms.JMSException;
import javax.jms.QueueSession;
import javax.jms.Session;
import javax.jms.TopicSession;
import javax.jms.XAQueueSession;
import javax.jms.XASession;
import javax.jms.XATopicSession;
import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
import javax.transaction.xa.Xid;

import com.ibm.ima.jms.ImaProperties;

/*
 * Implement the XASession interface for the IBM MessageSight JMS client.
 */
public final class ImaXASession extends ImaSession implements XASession, XATopicSession, XAQueueSession {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private AtomicReference<XAResourceImpl> xar = new AtomicReference<ImaXASession.XAResourceImpl>(null);

    public ImaXASession(ImaConnection connect, ImaProperties props, int domain)
            throws JMSException {
        super(connect, props, true, Session.AUTO_ACKNOWLEDGE, domain, true);
    }
    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.XAQueueSession#getQueueSession()
     */
    public QueueSession getQueueSession() throws JMSException {
        return this;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.XATopicSession#getTopicSession()
     */
    public TopicSession getTopicSession() throws JMSException {
        return this;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.XASession#getSession()
     */
    public Session getSession() throws JMSException {
        return this;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.XASession#getXAResource()
     */
    public XAResource getXAResource() {
        return new XAResourceImpl();
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.XASession#commit()
     */
    public void commit() throws JMSException {
        throw new ImaTransactionInProgressException("CWLNC0480", "A call to commit() or rollback() on a XASession object failed because these method calls are not permitted for XASession objects.");
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.XASession#getTransacted()
     */
    public boolean getTransacted() throws JMSException {
        return true;
    }

    /*
     * (non-Javadoc)
     * 
     * @see javax.jms.XASession#rollback()
     */
    public void rollback() throws JMSException {
        throw new ImaTransactionInProgressException("CWLNC0480", "A call to commit() or rollback() on a XASession object failed because these method calls are not permitted for XASession objects.");
    }

    /*
     * Internal close function which does not notify the connection. This is called from the connection to do the close.
     */
    synchronized void closeInternal() throws JMSException {
        XAResourceImpl xarImpl = xar.getAndSet(null);
        if (xarImpl != null) {
            xarImpl.closeXAResource();
            xarImpl = null;
        }
        super.closeInternal();
    }
    
    synchronized void markClosed() {
        XAResourceImpl xarImpl = xar.getAndSet(null);
        if (xarImpl != null) {
            xarImpl.closeXAResource();
            xarImpl = null;
        }
        super.markClosed();
    }

    private final class XAResourceImpl implements XAResource {
        private boolean recoveryInProcess = false;
        private Xid     currentXid = null;
        private final ImaSessionAction startAction = new ImaSessionAction(ImaAction.startGlobalTrans, ImaXASession.this, 256);
        private final ImaSessionAction endAction = new ImaSessionAction(ImaAction.endGlobalTrans, ImaXASession.this, 256);
        XAResourceImpl() {
        }

        private void checkClosed(int ec) throws XAException {
            if (isClosed.get()) {
                // TODO: Trace
                // No need to provide IMA message code here. ID will be assigned by caller.
                throw new XAException(ec);
            }
        }

        /*
         * Commit the global transaction.
         * 
         * @see javax.transaction.xa.XAResource#commit(javax.transaction.xa.Xid, boolean)
         */
        public void commit(Xid xid, boolean onePhase) throws XAException {
            if (ImaTrace.isTraceable(7)) {
                ImaTrace.trace("Commit XA transaction: xar=" + this + " client=" + connect.clientid +
                        " xid=" + ImaXidImpl.toString(xid) + " onePhase=" + onePhase);
            }
            try {
                checkClosed(XAException.XAER_RMFAIL);
                final ImaSessionAction action = new ImaSessionAction(ImaAction.commitGlobalTrans, ImaXASession.this, 256);
                action.outBB = ImaUtils.putXidValue(action.outBB, xid);
                action.outBB = ImaUtils.putBooleanValue(action.outBB, onePhase);
                action.setHeaderCount(2);
                action.request(true);
                if (action.rc != 0) {
                    throw createXAException(action.rc, "commit", xid);
                }
            } catch (JMSException e) {
                throw createXAException(0, "commit", xid, e);
            }
        }

        /*
         * End the global transaction
         * 
         * @see javax.transaction.xa.XAResource#end(javax.transaction.xa.Xid, int)
         */
        public void end(Xid xid, int flags) throws XAException {
            try {
            	try {
                    checkClosed(XAException.XA_RBROLLBACK);
            	} catch (XAException xae) {
            		ImaTrace.trace(1, "End failed because session " + ImaXASession.this + "is closed.");
            		ImaTrace.traceException(1, xae);
            		return;
            	}
                if (currentXid == null) {
                    if (ImaTrace.isTraceable(5)) {
                        ImaTrace.trace("End XA transaction that is already ended: xar=" + this + "client="
                                + connect.clientid + " xid=" + ImaXidImpl.toString(xid));
                    }
                    return;
                }
                if (ImaTrace.isTraceable(7)) {
                    ImaTrace.trace("End XA transaction: xar=" + this + " client=" + connect.clientid + " xid="
                            + ImaXidImpl.toString(xid) + " flags=" + flags);
                }
                checkXid(xid, "end");
                currentXid = null;
                xar.set(null);
                endAction.reset(ImaAction.endGlobalTrans);
                endAction.outBB = ImaUtils.putIntValue(endAction.outBB, flags);
                endAction.setHeaderCount(1);
                writeAckSqns("xa_end", endAction, null);
                endAction.request(true);
                if (endAction.rc != 0) {
                    throw createXAException(endAction.rc, "end", xid);
                }
            } catch (JMSException e) {
//                throw createXAException(0, "end", xid, e, XAException.XAER_RMERR);
                throw createXAException(0, "end", xid, e);
            }
        }

        /*
         * Forget the global transaction
         * 
         * @see javax.transaction.xa.XAResource#forget(javax.transaction.xa.Xid)
         */
        public void forget(Xid xid) throws XAException {
        	// Check server version.  If the version is not at least 2.0, then throw an
        	// exception.  Support for forget() was added as of the 2.0 release.
        	try {
            	if (((ImaMetaData)connect.getMetaData()).getImaServerVersion() < 2.0)
            		throw createXAException(0, "forget", xid, null, XAException.XAER_NOTA);
        	} catch (JMSException ex) {
        		throw createXAException(0, "forget", xid, ex, XAException.XAER_NOTA);
        	}
        	
            if (ImaTrace.isTraceable(4)) {
                ImaTrace.trace("Forget global transaction: xar=" + this + " client=" + connect.clientid +
                        " xid=" + ImaXidImpl.toString(xid));
            }
            try {
                checkClosed(XAException.XAER_RMFAIL);
                final ImaSessionAction action = new ImaSessionAction(ImaAction.forgetGlobalTrans, ImaXASession.this, 256);
                action.outBB = ImaUtils.putXidValue(action.outBB, xid);
                action.setHeaderCount(1);
                action.request(true);
                if (action.rc != 0) {
                    throw createXAException(action.rc, "forget", xid);
                }
            } catch (JMSException e) {
                throw createXAException(0, "forget", xid, e);
            }
        }

        /*
         * (non-Javadoc)
         * 
         * @see javax.transaction.xa.XAResource#getTransactionTimeout()
         */
        public int getTransactionTimeout() throws XAException {
            return Integer.MAX_VALUE;
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
            try {
                checkClosed(XAException.XAER_NOTA);
                if (ImaTrace.isTraceable(7)) {
                    ImaTrace.trace("Prepare XA transaction: xar=" + this + " client=" + connect.clientid + " xid="
                            + ImaXidImpl.toString(xid));
                }
                final ImaSessionAction action = new ImaSessionAction(ImaAction.prepareGlobalTrans, ImaXASession.this, 256);
                action.outBB = ImaUtils.putXidValue(action.outBB, xid);
                action.setHeaderCount(1);
                action.request(true);
                if (action.rc != 0) {
                    throw createXAException(action.rc, "prepare", xid);
                }
            } catch (JMSException e) {
                throw createXAException(0, "prepare", xid, e);
            }
            return XA_OK;
        }

        /*
         * Recover a global transaction.
         * 
         * @see javax.transaction.xa.XAResource#recover(int)
         */
        public Xid[] recover(int flags) throws XAException {
            // http://docs.oracle.com/cd/E16655_01/appdev.121/e17663/oracle/jdbc/xa/OracleXAResource.html#recover_int_
            try {
                checkClosed(XAException.XAER_RMFAIL);
                switch (flags) {
                case TMNOFLAGS:
                    if (recoveryInProcess)
                        return new Xid[0];
                    break;
                case (TMSTARTRSCAN | TMENDRSCAN):
                    recoveryInProcess = false;
                    break;
                case TMSTARTRSCAN:
                    recoveryInProcess = true;
                    break;
                case TMENDRSCAN:
                    if (recoveryInProcess) {
                        recoveryInProcess = false;
                        return new Xid[0];
                    }
                    break;
                default:
                    throw new ImaXAException("CWLNC0481", XAException.XAER_INVAL, "A recover operation failed due to invalid flags ({0}).", flags);
                }
                flags = TMSTARTRSCAN;
                LinkedList<Xid> xids = new LinkedList<Xid>();
                final ImaSessionAction action = new ImaSessionAction(ImaAction.recoverGlobalTrans, ImaXASession.this, 256);
                do {
                    action.reset(ImaAction.recoverGlobalTrans);
                    action.outBB = ImaUtils.putIntValue(action.outBB, flags);
                    action.outBB = ImaUtils.putIntValue(action.outBB, 256);
                    action.setHeaderCount(2);
                    action.request();
                    if (action.rc != 0) {
                        throw createXAException(action.rc, "recover", null);
                    }
                    int count = ImaUtils.getInteger(action.inBB);
                    if (count > 0) {
                        int otype = ImaUtils.getObjectType(action.inBB);
                        ByteBuffer bb = ImaUtils.getByteBufferValue(action.inBB, otype);
                        for (int i = 0; i < count; i++) {
                            Xid xid = (Xid) ImaUtils.getObjectValue(bb);
                            xids.addLast(xid);
                        }
                    }
                    if (count < 256) {
                        action.reset(ImaAction.recoverGlobalTrans);
                        action.outBB = ImaUtils.putIntValue(action.outBB, TMENDRSCAN);
                        action.outBB = ImaUtils.putIntValue(action.outBB, 0);
                        action.setHeaderCount(2);
                        action.request();
                        break;
                    }
                    flags = TMNOFLAGS;
                } while (true);
                Xid[] result = new Xid[xids.size()];
                return xids.toArray(result);
            } catch (JMSException e) {
                throw createXAException(0, "recover", null, e);
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
                if (ImaTrace.isTraceable(7)) {
                    ImaTrace.trace("rollback XA transaction: xar=" + this + " client=" + connect.clientid + " xid="
                            + ImaXidImpl.toString(xid));
                }
                final ImaSessionAction action = new ImaSessionAction(ImaAction.rollbackGlobalTrans, ImaXASession.this, 256);
                action.outBB = ImaUtils.putXidValue(action.outBB, xid);
                action.setHeaderCount(1);
                action.request(true);
                if (action.rc != 0) {
                    throw createXAException(action.rc, "rollback", xid);
                }
            } catch (JMSException e) {
                throw createXAException(0, "rollback", xid, e);
            }
        }

        /*
         * Set the global transaction timeout.
         * 
         * @see javax.transaction.xa.XAResource#setTransactionTimeout(int)
         */
        public boolean setTransactionTimeout(int seconds) throws XAException {
            return false;
        }

        /*
         * Start a global transaction.
         * 
         * @see javax.transaction.xa.XAResource#start(javax.transaction.xa.Xid, int)
         */
        public void start(Xid xid, int flags) throws XAException {
            try {
                checkClosed(XAException.XAER_RMFAIL);
                if (ImaTrace.isTraceable(7)) {
                    ImaTrace.trace("Start XA transaction: xar=" + this + " client=" + connect.clientid + " xid="
                            + ImaXidImpl.toString(xid) + " flags=" + flags);
                }
                if (!xar.compareAndSet(null, this)) {
                    // TODO: Throw exception
                }
                startAction.reset(ImaAction.startGlobalTrans);
                startAction.outBB = ImaUtils.putXidValue(startAction.outBB, xid);
                startAction.outBB = ImaUtils.putIntValue(startAction.outBB, flags);
                startAction.setHeaderCount(2);
                startAction.request();
                if (startAction.rc != 0) {
                    xar.set(null);
                    throw createXAException(startAction.rc, "start", xid);
                }
                currentXid = xid;
            } catch (JMSException e) {
                xar.set(null);
                throw createXAException(0, "start", xid, e, XAException.XAER_RMERR);
            }
        }

        private final void closeXAResource() {
            try {
                if (currentXid != null)
                    end(currentXid, XAResource.TMFAIL);
            } catch (XAException ex) {
                // Do not rethrow. There is no action to take.
            	currentXid = null;
            }
        }

        private final void checkXid(Xid xid, String caller) throws XAException {
            if (xid == null)
            	throw new ImaXAException("CWLNC0482", XAException.XAER_INVAL, "A call to {0} failed because Xid is null.", caller);
            if (currentXid.getFormatId() != xid.getFormatId())
            	throw new ImaXAException("CWLNC0483", XAException.XAER_NOTA, "A call to {0} failed.  Expected Xid {1} but found {2}.", caller, currentXid.getFormatId(), xid.getFormatId());
            if (!Arrays.equals(currentXid.getBranchQualifier(), xid.getBranchQualifier()))
            	throw new ImaXAException("CWLNC0484", XAException.XAER_NOTA, "A call to {0} failed.  Expected branch {1} but found {2}.", caller, currentXid.getBranchQualifier(), xid.getBranchQualifier());
            if (!Arrays.equals(currentXid.getGlobalTransactionId(), xid.getGlobalTransactionId()))
            	throw new ImaXAException("CWLNC0485", XAException.XAER_NOTA, "A call to {0} failed.  Expected global transaction {1} but found {2}.", caller, currentXid.getGlobalTransactionId(), xid.getGlobalTransactionId());
        }

        private final XAException createXAException(int rc, String operation, Xid xid) {
            return createXAException(rc, operation, xid, null);
        }
        private final XAException createXAException(int rc, String operation, Xid xid, Exception cause) {
            return createXAException(rc, operation, xid, cause, 0);
        }

        private final XAException createXAException(int rc, String operation, Xid xid, Exception cause, int xaerr) {
            ImaXAException ex = null;
            switch (rc) {
            case 0:
                if (xaerr != 0)
                    ex = new ImaXAException("CWLNC0486", xaerr, "Global transaction failed for {0}: IMA server was unreachable.", operation);
                else
                	ex = new ImaXAException("CWLNC0486", XAException.XAER_RMFAIL, "A global transaction failed during operation {0} because MessageSight was unavailable. This can happen if MessageSight stops when the transaction is in progress, or if there are network issues.", operation);
                break;
            case ImaReturnCode.IMARC_NotFound:
            case ImaReturnCode.IMARC_ObjectNotValid:
            	ex = new ImaXAException("CWLNC0487", XAException.XAER_RMERR, "A global transaction failed for operation {0}: Xid {1} is unknown.", operation, xid);
                break;
            case ImaReturnCode.IMARC_RolledBack:
                ex = new ImaXAException("CWLNC0488", XAException.XA_RBROLLBACK, "A global transaction failed for operation {0}: Global transaction was marked as rollback only.", operation);
                break;
            case ImaReturnCode.IMARC_TransactionInUse:
                ex = new ImaXAException("CWLNC0489", XAException.XAER_DUPID, "A global transaction failed for operation {0}: Requested Xid {1} already exists.", operation, xid);
                break;
            case ImaReturnCode.IMARC_InvalidParameter:
            	// TODO: Does this error come from the server for global transactions???
                ex = new ImaXAException("CWLNC0490", XAException.XAER_INVAL, "A global transaction failed for operation {0}: Invalid parameter.", operation);
                break;
            case ImaReturnCode.IMARC_HeuristicallyCommitted:
                ex = new ImaXAException("CWLNC0491", XAException.XA_HEURCOM, "A global transaction failed for operation {0} because it was heuristically committed.", operation);
                break;
            case ImaReturnCode.IMARC_HeuristicallyRolledBack:
                ex = new ImaXAException("CWLNC0492", XAException.XA_HEURRB, "A global transaction failed for operation {0} because it was heuristically rolled back.", operation);
                break;
            default:
                ex = new ImaXAException("CWLNC0493", XAException.XAER_RMERR, "A global transaction failed for operation {0}: Error {1} was returned by MessageSight.", operation, rc);
                break;
            }
            if (ImaTrace.isTraceable(2)) {
                String msg = "Unable to complete " + operation + " request for global transaction: client="
                        + connect.clientid;
                if (xid != null) {
                    msg += " xid=" + ImaXidImpl.toString(xid) + '.';
                }
                msg += " The reason is \"" + ex.getMessage() + "\".";
                ImaTrace.trace(msg);
            }

            if (cause != null)
                ex.initCause(cause);
            ImaTrace.traceException(2, ex);
            return ex;
        }
    }
}
