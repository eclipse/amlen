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

import javax.resource.spi.InvalidPropertyException;
import javax.resource.spi.ResourceAdapter;
import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
import javax.transaction.xa.Xid;

import junit.framework.TestCase;

import com.ibm.ima.jms.impl.ImaTestHelper;
import com.ibm.ima.ra.inbound.ImaActivationSpec;
import com.ibm.ima.ra.testutils.MockBootstrapContext;

public class TestRaRecovXAResource extends TestCase {
    MockBootstrapContext bootstrapCtx = null;
    ResourceAdapter ra = null;
    ImaActivationSpec    actSpec      = null;
    
    public void setUp() {
        try {
            bootstrapCtx = new MockBootstrapContext();
            
            ra = new ImaResourceAdapter();
            ra.start(bootstrapCtx);
            
            actSpec = new ImaActivationSpec();
            
            AllTests.setConnParams(actSpec);
        } catch(Throwable t) {
            throw new RuntimeException(t);
        }
        
    }
    
    public TestRaRecovXAResource() {
        super("IMA JMS Resource Adapter RecovXAResource test");
    }
    
    public void testCtorV11() {
        ImaRecoveryXAResource rxar = null;
        
        // TODO: Update this test when implementation is complete
        try {
            AllTests.setConnParams(1100000, actSpec);
            actSpec.setDestination("myTopic");
            actSpec.setDestinationType("javax.jms.Topic");
            
            rxar = new ImaRecoveryXAResource((ImaResourceAdapter)ra, (ImaActivationSpec)actSpec);

            try {
                int timeout = rxar.getTransactionTimeout();
                assertEquals("Exepcted exception on call to getTransactionTimeout()","Got return value "+timeout);
            } catch (XAException xaex) {
                assertEquals(XAException.XAER_PROTO, xaex.errorCode);
            }
            try {
                rxar.setTransactionTimeout(60);
                assertEquals("Exepcted exception on call to setTransactionTimeout()","No exception from setTransactionTimeout()");
            } catch (XAException xaex) {
                assertEquals(XAException.XAER_PROTO, xaex.errorCode);
            }
            
            try {
                rxar.end(ImaTestHelper.getXid(), XAResource.TMSUCCESS);
                assertEquals("Exepcted exception on call to end()","No exception from end()");
            } catch (XAException xaex) {
                assertEquals(XAException.XAER_PROTO, xaex.errorCode);
            }
            
            try {
                rxar.forget(ImaTestHelper.getXid());
                assertEquals("Exepcted exception on call to forget()","No exception from forget()");
            } catch (XAException xaex) {
                xaex.printStackTrace();
                assertEquals(XAException.XAER_NOTA, xaex.errorCode);
            }
            
            try {
                rxar.prepare(ImaTestHelper.getXid());
                assertEquals("Exepcted exception on call to prepare()","No exception from prepare()");
            } catch (XAException xaex) {
                assertEquals(XAException.XAER_PROTO, xaex.errorCode);
            }

            try {
                rxar.rollback(ImaTestHelper.getXid());
            } catch (XAException xaex) {
                xaex.printStackTrace();
                assertEquals(null, xaex);
            }
            
            try {
                rxar.start(ImaTestHelper.getXid(), XAResource.TMNOFLAGS);
                assertEquals("Exepcted exception on call to start()","No exception from start()");
            } catch (XAException xaex) {
                assertEquals(XAException.XAER_PROTO, xaex.errorCode);
            }
            
            AllTests.setConnParams(actSpec);
            
            rxar = new ImaRecoveryXAResource((ImaResourceAdapter)ra, (ImaActivationSpec)actSpec);
            
            try {
                rxar.commit(ImaTestHelper.getXid(), true);
            } catch (XAException xaex) {
                xaex.printStackTrace();
                assertEquals(null, xaex);
            }
            
            AllTests.setConnParams(actSpec);
            actSpec.setDestination("myTopic");
            actSpec.setDestinationType("javax.jms.Topic");
            
            rxar = new ImaRecoveryXAResource((ImaResourceAdapter)ra, (ImaActivationSpec)actSpec);
            
            Xid recXids[] = rxar.recover(XAResource.TMNOFLAGS);
            assertEquals(0, recXids.length);
            
        } catch(XAException xe) {
            assertEquals(XAException.XAER_RMFAIL,xe.errorCode);
        } catch (InvalidPropertyException e) {
            e.printStackTrace();
            assertEquals(null,e);
        }
    }
    
    public void testCtorV20() {
        ImaRecoveryXAResource rxar = null;
        
        // TODO: Update this test when implementation is complete
        try {
            AllTests.setConnParams(2000000, actSpec);
            actSpec.setDestination("myTopic");
            actSpec.setDestinationType("javax.jms.Topic");
            
            rxar = new ImaRecoveryXAResource((ImaResourceAdapter)ra, (ImaActivationSpec)actSpec);

            try {
                int timeout = rxar.getTransactionTimeout();
                assertEquals("Exepcted exception on call to getTransactionTimeout()","Got return value "+timeout);
            } catch (XAException xaex) {
                assertEquals(XAException.XAER_PROTO, xaex.errorCode);
            }
            try {
                rxar.setTransactionTimeout(60);
                assertEquals("Exepcted exception on call to setTransactionTimeout()","No exception from setTransactionTimeout()");
            } catch (XAException xaex) {
                assertEquals(XAException.XAER_PROTO, xaex.errorCode);
            }
            
            try {
                rxar.end(ImaTestHelper.getXid(), XAResource.TMSUCCESS);
                assertEquals("Exepcted exception on call to end()","No exception from end()");
            } catch (XAException xaex) {
                assertEquals(XAException.XAER_PROTO, xaex.errorCode);
            }
            
            try {
                rxar.forget(ImaTestHelper.getXid());
            } catch (XAException xaex) {
                assertEquals(null, xaex);
            }
            
            AllTests.setConnParams(2000000, actSpec);
            
            rxar = new ImaRecoveryXAResource((ImaResourceAdapter)ra, (ImaActivationSpec)actSpec);
            
            try {
                rxar.commit(ImaTestHelper.getXid(), true);
            } catch (XAException xaex) {
                xaex.printStackTrace();
                assertEquals(null, xaex);
            }
            
            AllTests.setConnParams(actSpec);
            actSpec.setDestination("myTopic");
            actSpec.setDestinationType("javax.jms.Topic");
            
            rxar = new ImaRecoveryXAResource((ImaResourceAdapter)ra, (ImaActivationSpec)actSpec);
            
            Xid recXids[] = rxar.recover(XAResource.TMNOFLAGS);
            assertEquals(0, recXids.length);
            
        } catch(XAException xe) {
            assertEquals(XAException.XAER_RMFAIL,xe.errorCode);
        } catch (InvalidPropertyException e) {
            e.printStackTrace();
            assertEquals(null,e);
        }
    }
    
    public void testToDo_MoreTestsNeeded() {
        // Place holder test failure to show that we need to do more unit test work here 
        // when we decide what we're going to do for XA support.
        System.err.println("More unit test development needed for XA");
        // assertTrue(false);
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


}
