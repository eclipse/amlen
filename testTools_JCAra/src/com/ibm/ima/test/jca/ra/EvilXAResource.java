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
package com.ibm.ima.test.jca.ra;

import java.io.FileNotFoundException;
import java.io.Serializable;

import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
import javax.transaction.xa.Xid;

import com.ibm.ima.jms.test.jca.AUtils;
import com.ibm.ima.jms.test.jca.TestProps;

/**
 * Copyright I.B.M. Corporation 2013. All Rights Reserved.
 * 
 * This is an evil implementation of XAResource that throws exceptions
 * during a transaction according to the TestProps test case.
 * 
 * Notes on crashing MessageSight during XA Transactions
 *
 * In our test app, connections in evil RA related tests are created in the
 * following order (as of 2/3/14):
 * 
 *   For Container managed
 *   	1. XA Transaction Branch 1 = MDB connection receiving the incoming message
 *   	2. XA Transaction Branch 2 = Outbound connection sending reply message
 *   	3. XA Transaction Branch 3 = Evil RA Connection
 *   
 *   For Bean managed
 *   	1. XA Transaction Branch 1 = Outbound connection sending reply message
 *   	2. XA Transaction Branch 2 = Evil RA Connection
 *   
 * In WAS, prepare transaction prepares the transaction branches in reverse order,
 * while commit transaction commits the branches in order.
 * 
 * From EvilXAResource, we need to invoke an IMA crash from either prepare or
 * commit.
 * 
 * CRASH IMA DURING PREPARE:
 * If we cause a crash during EvilXAResource.prepare(Xid), then IMA will fail to
 * prepare as it cannot reach imaserver. This will cause the transaction to just rollback.
 * For container managed, this will rollback everything including the MDB receiving the
 * message. For bean managed, the message received by the MDB won't be redelivered and
 * so the reply transaction won't be retried.
 * 
 * CRASH IMA DURING COMMIT:
 * If we cause a crash during EvilXAResource.commit(Xid, boolean), then IMA 
 * will have already committed its branches of the transaction.
 * 
 * 
 * An alternative order to create connections in the test app:
 * 
 *   For Container managed
 *   	1. XA Transaction Branch 1 = MDB connection receiving the incoming message
 *   	2. XA Transaction Branch 2 = Evil RA Connection
 *   	3. XA Transaction Branch 3 = Outbound connection sending reply message
 *   
 *   For Bean managed
 *   	1. XA Transaction Branch 1 = Evil RA Connection
 *   	2. XA Transaction Branch 2 = Outbound connection sending reply message
 *   
 * CRASH IMA DURING PREPARE:
 * If Evil RA Connection is created before the reply message is sent, then for the container
 * managed case, prepare of branch 3 would succeed and prepare of branch 1 would fail. This 
 * just results in the transaction being rolled back.
 * 
 * For the bean managed case, swapping the order causes the IMA branch to prepare successfully,
 * and then fail on the commit. This is actually the behavior we have wanted, but since the
 * MDB receiving the message isn't part of the transaction, this prepared but not committed
 * transaction just... what? vanishes? Maybe it gets rolled back? TODO: find out I guess
 * 
 * CRASH IMA DURING COMMIT:
 * For the container managed case, branch 1 of the transaction will commit while branch
 * 3 fails. This means the message won't be redelivered and most likely our reply
 * message will never arrive. TODO: how does this case really work?
 * 
 * For the bean managed case, commit on branch 2 will fail. Since the message delivered to the
 * MDB will still be ack'd as a non-transacted message, we will probably again never see the
 * reply message arrive.
 * 
 * Example of transaction XID with 3 branches from WebSphere Application Server log:
 * 
 * Branch 1 (not prepared before imaserver crash, IMA RA)
 * xid=57415344:
 * 00000143F9C8AEF900000045671129E092BFD4F6194E523FAD4C590D02E109D8B9D6D6D8000000010000000000000000000000000001:
 * 00000143F9C8AEF900000045671129E092BFD4F6194E523FAD4C590D02E109D8B9D6D6D8
 * 
 * Branch 2 (prepared, TheEvilCon)
 * xid=57415344:
 * 00000143F9C8AEF900000045671129E092BFD4F6194E523FAD4C590D02E109D8B9D6D6D8000000010000000000000000000000000002:
 * 00000143F9C8AEF900000045671129E092BFD4F6194E523FAD4C590D02E109D8B9D6D6D8
 * 
 * Branch 3 (prepared before imaserver crash, IMA RA)
 * xid=57415344:
 * 00000143F9C8AEF900000045671129E092BFD4F6194E523FAD4C590D02E109D8B9D6D6D8000000010000000000000000000000000003:
 * 00000143F9C8AEF900000045671129E092BFD4F6194E523FAD4C590D02E109D8B9D6D6D8
 * 
 * Prepared transaction in imatrace.log after crashing MessageSight during EvilXAResource.prepare(xid):
 * 
 * imaserver transaction.c:894: Rehydrating global transaction for 57415344:
 * 00000143F9C8AEF900000045671129E092BFD4F6194E523FAD4C590D02E109D8B9D6D6D8000000010000000000000000000000000003:
 * 00000143F9C8AEF900000045671129E092BFD4F6194E523FAD4C590D02E109D8B9D6D6D8
 * imaserver transaction.c:968: >>> ietr_completeRehydration Hdl(281474976758272) State(2) Global(1)
 * 
 * State(2) = transaction is in prepared state.
 */

public class EvilXAResource implements XAResource, Serializable {

	private static final long serialVersionUID = -2285323450119458310L;
	private static int id;
	private EvilResourceAdapter ra;
	
	static {
		id = 10000;
	}

	private TestProps test;
	private int myId;
	private int txTimeout;

	public EvilXAResource(TestProps tp, EvilResourceAdapter _ra) {
		if (tp == null)
			throw new Error("TestProps is null in creation of EvilXAResource");
		test = tp;
		ra = _ra;
		myId = getId();
		txTimeout = 2; // arbitrary
		if (ra != null) {
		    ra.log("EvilXAResource", "EVILXA-CONSTRUCTOR");
		}
	}
	
	public void setTest(TestProps tp) {
	    ra.log("EvilXAResource", "Setting test");
	    test = tp;
	}
	
	private static synchronized int getId() {
		int newid = id++;
		return newid;
	}
	
	public void commit(Xid xid, boolean onePhase) throws XAException {
	    ra.log("EvilXAResource", "EVILXA-COMMIT");
        ra.log("\tEvilXAResource", "xid: " + xid);
        ra.log("\tEvilXAResource", "onePhase: " + onePhase);
        if (test != null) {
            ra.log("\tEvilXAResource", "failuretype: " + test.failureType);
			if (test.failureType == TestProps.FailureType.EVIL_FAIL_COMMIT) {
				ra.log("EvilXAResource", "EVIL_FAIL_COMMIT");
				throw new XAException("EvilXAResource commit() failed as expected");
			} else if (test.failureType == TestProps.FailureType.HEURISTIC_COMMIT) {
				ra.log("EvilXAResource", "HEURISTIC_COMMIT");
				try {
					String[] appliances = AUtils.getApplianceIPs();
					if (appliances.length > 0) {
						ra.log("EvilXAResource", appliances[0]);	
					}
					AUtils.stopServer(appliances[0]);			
				} catch (FileNotFoundException e) {
					ra.log("EvilXAResource", "failed to read env.file");
					e.printStackTrace();
				}
			}
        }
		/*
		if (test.failureType == (TestProps.FailureType.IMA_CRASH)) {
			ra.log("EvilXAResource", "going to failover IMA Server in XA commit()");
			try {
				String[] appliances = AUtils.getApplianceIPs(); 
				AUtils.restartServer(appliances[0]);			
			} catch (FileNotFoundException e) {
				ra.log("EvilXAResource", "failed to read env.file");
				e.printStackTrace();
			}
			throw new XAException(XAException.XAER_RMERR);
		}
		*/
	}

	public void end(Xid xid, int flags) throws XAException {
	    ra.log("EvilXAResource", "EVILXA-END");
        ra.log("\tEvilXAResource", "xid: " + xid);
        ra.log("\tEvilXAResource", "flags: " + flags);
		if (test.failureType == (TestProps.FailureType.EVIL_FAIL_END)) {
			if (ra != null) {
			    ra.log("EvilXAResource", "EVIL_FAIL_END");
			}
			throw new XAException("EvilXAResource end() failed as expected");
		}
	}

	public void forget(Xid xid) throws XAException {
	    ra.log("EvilXAResource", "EVILXA-FORGET");
        ra.log("\tEvilXAResource", "xid: " + xid);
		if ((test != null) && (test.failureType == (TestProps.FailureType.EVIL_FAIL_FORGET))) {
			if (ra != null) {
			    ra.log("EvilXAResource", "EVIL_FAIL_FORGET");
			}
			throw new XAException("XAResource forget() failed as expected");
		}
	}

	public int getTransactionTimeout() throws XAException {
	    ra.log("EvilXAResource", "EVILXA-GET_TRANSACTION_TIMEOUT");
		if (test.failureType == (TestProps.FailureType.EVIL_FAIL_GET_TRANSACTION_TIMEOUT)) {
			if (ra != null) {
			    ra.log("EvilXAResource", "EVIL_FAIL_GET_TRANSACTION_TIMEOUT");
			}
			throw new XAException("XAResource getTransactionTimeout() failed as expected");
		}
		return txTimeout;
	}

	public boolean isSameRM(XAResource xares) throws XAException {
	    ra.log("EvilXAResource", "isSameRM(xares)");
		if (xares instanceof EvilXAResource)
			return myId == ((EvilXAResource) xares).myId;
		return false;
	}

	// This is the method where real work would be done in a real
	// Resource Adapter.
	public int prepare(Xid xid) throws XAException {
		ra.log("EvilXAResource", "EVILXA-PREPARE");
		ra.log("EvilXAResource", "xid: " + xid);
		if (test.failureType == (TestProps.FailureType.EVIL_FAIL_PREPARE)) {
			if (ra != null) {
			    ra.log("EvilXAResource", "EVIL_FAIL_PREPARE");
			}
			throw new XAException("XAResource prepare() failed as expected");
		} else if (test.failureType == (TestProps.FailureType.HEURISTIC_PREPARE)) {
			ra.log("EvilXAResource", "HEURISTIC_PREPARE");
			try {
				String[] appliances = AUtils.getApplianceIPs(); 
				AUtils.stopServer(appliances[0]);			
			} catch (FileNotFoundException e) {
				ra.log("EvilXAResource", "failed to read env.file");
				e.printStackTrace();
			}
		}
		return XA_OK;
	}

	public Xid[] recover(int flag) throws XAException {
	    ra.log("EvilXAResource", "EVILXA-RECOVER");
	    ra.log("\tEvilXAResource", "flag: " + flag);
		if ((test != null) && (test.failureType == (TestProps.FailureType.EVIL_FAIL_RECOVER))) {
			ra.log("EvilXAResource", "EVIL_FAIL_RECOVER");
			throw new XAException("XAResource recover() failed as expected");
		}	
		return new Xid[0];
	}

	public void rollback(Xid xid) throws XAException {
	    ra.log("EvilXAResource", "EVILXA-ROLLBACK");
        ra.log("\tEvilXAResource", "xid: " + xid);
		if ((test != null) && (test.failureType == (TestProps.FailureType.EVIL_FAIL_ROLLBACK))) {
			if (ra != null) {
			    ra.log("EvilXAResource", "EVIL_FAIL_ROLLBACK");
			}
			throw new XAException("XAResource rollback() failed as expected");
		}
	}

	public void start(Xid xid, int flags) throws XAException {
	    ra.log("EvilXAResource", "EVILXA-START");
        ra.log("\tEvilXAResource", "xid: " + xid);
        ra.log("\tEvilXAResource", "flags: " + flags);
		if (test.failureType == (TestProps.FailureType.EVIL_FAIL_START)) {
			if (ra != null) {
			    ra.log("EvilXAResource", "EVIL_FAIL_START");
			}
			throw new XAException("XAResource start() failed as expected");
		}
	}

	public boolean setTransactionTimeout(int seconds) throws XAException {
	    ra.log("EvilXAResource", "EVILXA-SET_TRANSACTION_TIMEOUT");
	    ra.log("\tEvilXAResource", "seconds: " + seconds);
		if (test.failureType == (TestProps.FailureType.EVIL_FAIL_SET_TRANSACTION_TIMEOUT)) {
			if (ra != null) {
			    ra.log("EvilXAResource", "EVIL_FAIL_SET_TRANSACTION_TIMEOUT");
			}
			throw new XAException("XAResource setTransactionTimeout failed as expected");
		}		
		txTimeout = seconds;
		return true;
	}
	
}
