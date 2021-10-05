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

import javax.jms.Connection;
import javax.resource.ResourceException;
import javax.resource.spi.ActivationSpec;
import javax.resource.spi.ResourceAdapter;
import javax.resource.spi.endpoint.MessageEndpointFactory;
import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
import javax.transaction.xa.Xid;

import junit.framework.TestCase;

import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaTestHelper;
import com.ibm.ima.ra.inbound.ImaActivationSpec;
import com.ibm.ima.ra.inbound.ImaConnectionPool;
import com.ibm.ima.ra.inbound.InboundTestHelper;
import com.ibm.ima.ra.testutils.MockActivationSpec;
import com.ibm.ima.ra.testutils.MockBootstrapContext;
import com.ibm.ima.ra.testutils.MockMessageEndpointFactory;

public class TestRa extends TestCase {
	MockBootstrapContext bootstrapCtx = null;

	public void setUp() {
		bootstrapCtx = new MockBootstrapContext();
	}
	
	public TestRa() {
		super("IMA JMS Resource Adapter Ra test");
	}
	
	public void testStart() throws Exception {
		ResourceAdapter ra = new ImaResourceAdapter();
		ra.start(bootstrapCtx);
		// If we get this far, it means that the start() method executed without exceptions.
		// Force a true assertion to reflect this.
		assertTrue(true);
		
	}
	
    public void testEndpointActivationNullSpec() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        ra.start(bootstrapCtx);

        try {
            ra.endpointActivation(null, null);
            // If we get this far, something has gone wrong.  Force a failure.
            assertTrue(false);
        } catch(Exception ex) {
            assertTrue(ex instanceof NullPointerException);
        }
    }
    
    public void testEndpointActivationNullMef() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        ra.start(bootstrapCtx);
        
        ActivationSpec actSpec = new ImaActivationSpec();

        try {
            ((ImaActivationSpec)actSpec).setDestination("myTopic");
            ((ImaActivationSpec)actSpec).setDestinationType("javax.jms.Topic");
            ra.endpointActivation(null, actSpec);
            // If we get this far, something has gone wrong.  Force a failure.
            assertTrue(false);
        } catch(Exception ex) {
            assertTrue(ex instanceof NullPointerException);
        }
    }
    
    public void testEndpointActivationWrongSpec() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        ra.start(bootstrapCtx);
        
        MessageEndpointFactory mef = new MockMessageEndpointFactory();
        ActivationSpec actSpec = new MockActivationSpec();

        try {
            ra.endpointActivation(mef, actSpec);
            // If we get this far, something has gone wrong.  Force a failure.
            assertTrue(false);
        } catch(ResourceException re) {
            // TODO: Update expected error code when values are assigned for RA
            assertEquals("CWLNC2101", re.getErrorCode());
        }
    }
    
    public void testJustEndpointActivation() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        /* Intentionally omit call to start ra */
        
        MessageEndpointFactory mef = new MockMessageEndpointFactory();
        ImaActivationSpec actSpec = new ImaActivationSpec();
        
        AllTests.setConnParams(actSpec);
        try {
            ra.endpointActivation(mef, actSpec);
            // If we get this far, something has gone wrong. Force failure.
            assertTrue(false);
        } catch(Exception ex) {
            // TODO: Check into this more
            // We get a resource exception here because the queue name has
            // not been set.  That happens because there's not actually a
            // destination specified in the activation specification.  Bean
            // validation traps this in WAS but there is no bean validation 
            // in unit tests. 
            assertTrue(ex instanceof ResourceException);
        }
    }
    	
	public void testEndpointActivation() throws Exception {
		ResourceAdapter ra = new ImaResourceAdapter();
		ra.start(bootstrapCtx);
		
		MessageEndpointFactory mef = new MockMessageEndpointFactory();
        ImaActivationSpec actSpec = new ImaActivationSpec();
        actSpec.setDestination("MyQueue");
		
        AllTests.setConnParams(actSpec);
        actSpec.setDestination("myTopic");
        actSpec.setDestinationType("javax.jms.Topic");
		ra.endpointActivation(mef, actSpec);
	    // If we get this far, it means that the endpointActivation() method executed without exceptions.
        // Force a true assertion to reflect this.
        assertTrue(true);
	}
	
    public void testJustStop() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        
        ra.stop();
        // If we get this far, it means that the stop() method executed without exceptions.
        // Force a true assertion to reflect this.
        assertTrue(true);
    }
        
    
    
    public void testStop1() throws Exception {
         ResourceAdapter ra = new ImaResourceAdapter();
         ra.start(bootstrapCtx);
         
         MessageEndpointFactory mef = new MockMessageEndpointFactory();
         ImaActivationSpec actSpec = new ImaActivationSpec();
         actSpec.setDestination("MyQueue");
         actSpec.setDestinationType("javax.jms.Topic");
         
        AllTests.setConnParams(actSpec);
        ra.endpointActivation(mef, actSpec);
         
         ra.stop();
         // If we get this far, it means that the stop() method executed without exceptions.
         // Force a true assertion to reflect this.
         assertTrue(true);
         
     }
    
    public void testJustEndpointDeactivation() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        
        MessageEndpointFactory mef = new MockMessageEndpointFactory();
        ActivationSpec actSpec = new ImaActivationSpec();
        
        ra.endpointDeactivation(mef, actSpec);

        // If we get this far, it means that the endpointDeactivation() method executed without exceptions.
        // Force a true assertion to reflect this.
        assertTrue(true);
        
    }
	   
    public void testEndpointDeactivation() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        ra.start(bootstrapCtx);
        
        MessageEndpointFactory mef = new MockMessageEndpointFactory();
        ImaActivationSpec actSpec = new ImaActivationSpec();
        actSpec.setDestination("MyQueue");
        
        AllTests.setConnParams(actSpec);
        
        ra.endpointDeactivation(mef, actSpec);

        // If we get this far, it means that the endpointDeactivation() method executed without exceptions.
        // Force a true assertion to reflect this.
        assertTrue(true);   
    }
    
    public void testStop2() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        ra.start(bootstrapCtx);
        
        MessageEndpointFactory mef = new MockMessageEndpointFactory();
        ImaActivationSpec actSpec = new ImaActivationSpec();
        actSpec.setDestination("MyQueue");
        actSpec.setDestinationType("javax.jms.Topic");
        
        AllTests.setConnParams(actSpec);
        ra.endpointActivation(mef, actSpec);
        ra.endpointDeactivation(mef, actSpec);
        
        ra.stop();
        // If we get this far, it means that the stop() method executed without exceptions.
        // Force a true assertion to reflect this.
        assertTrue(true);  
    }
    
    public void testGetXAResourcesNullSpecs() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        ra.start(bootstrapCtx);
        
        @SuppressWarnings("unused")
        XAResource[] xaresources = null;
        
        try {
            xaresources = ra.getXAResources(null);
            // If we get to this line, the test needs to be updated.  Force a failure;
            assertTrue(false);
        } catch(Exception ex) {
            assertTrue(ex instanceof NullPointerException);
        }
    }
    
    
    public void testJustGetXAResources() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        ra.start(bootstrapCtx);
        
        ActivationSpec[] actSpec = new ActivationSpec[1];
        
        actSpec[0] = new ImaActivationSpec();
        AllTests.setConnParams((ImaActivationSpec)actSpec[0]);
        ((ImaActivationSpec)actSpec[0]).setDestination("MyTopic");
        ((ImaActivationSpec)actSpec[0]).setDestinationType("javax.jms.Topic");
        
        @SuppressWarnings("unused")
        XAResource[] xaresources = null;
        
        try {
            xaresources = ra.getXAResources(actSpec);
        } catch(ResourceException re) {
            re.printStackTrace();
            assertEquals(null, re);
        }
    }
    
    public void testGetXAResourcesV11() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        ra.start(bootstrapCtx);
        
        ImaActivationSpec[] actSpec = new ImaActivationSpec[1];
        
        MessageEndpointFactory mef = new MockMessageEndpointFactory();
        actSpec[0] = new ImaActivationSpec();
        actSpec[0].setDestination("MyQueue");
        actSpec[0].setDestinationType("javax.jms.Topic");
        
        AllTests.setConnParams(1100000, actSpec[0]);
        ra.endpointActivation(mef, actSpec[0]);
        
        // TODO: Rework this test when required code is implemented.  For now, code test to pass.
        //@SuppressWarnings("unused")
        XAResource[] xaresources = null;
        
        try {
            xaresources = ra.getXAResources(actSpec);
            assertEquals(1, xaresources.length);
            
            for (int i = 0; i < xaresources.length; i++) {
                assertEquals("com.ibm.ima.ra.ImaRecoveryXAResource", xaresources[i].getClass().getName());
                try {
                    int timeout = xaresources[i].getTransactionTimeout();
                    assertEquals("Exepcted exception on call to getTransactionTimeout()","Got return value "+timeout);
                } catch (XAException xaex) {
                    assertEquals(XAException.XAER_PROTO, xaex.errorCode);
                }
                try {
                    xaresources[i].setTransactionTimeout(60);
                    assertEquals("Exepcted exception on call to setTransactionTimeout()","No exception from setTransactionTimeout()");
                } catch (XAException xaex) {
                    assertEquals(XAException.XAER_PROTO, xaex.errorCode);
                }
                
                try {
                    xaresources[i].end(ImaTestHelper.getXid(), XAResource.TMSUCCESS);
                    assertEquals("Exepcted exception on call to end()","No exception from end()");
                } catch (XAException xaex) {
                    assertEquals(XAException.XAER_PROTO, xaex.errorCode);
                }
                
                try {
                    xaresources[i].forget(ImaTestHelper.getXid());
                    assertEquals("Exepcted exception on call to forget()","No exception from forget()");
                } catch (XAException xaex) {
                    xaex.printStackTrace();
                    assertEquals(XAException.XAER_NOTA, xaex.errorCode);
                }
                
                try {
                    xaresources[i].prepare(ImaTestHelper.getXid());
                    assertEquals("Exepcted exception on call to prepare()","No exception from prepare()");
                } catch (XAException xaex) {
                    assertEquals(XAException.XAER_PROTO, xaex.errorCode);
                }

                try {
                    xaresources[i].rollback(ImaTestHelper.getXid());
                } catch (XAException xaex) {
                    xaex.printStackTrace();
                    assertEquals(null, xaex);
                }
                
                try {
                    xaresources[i].start(ImaTestHelper.getXid(), XAResource.TMNOFLAGS);
                    assertEquals("Exepcted exception on call to start()","No exception from start()");
                } catch (XAException xaex) {
                    assertEquals(XAException.XAER_PROTO, xaex.errorCode);
                }
            }
            
            AllTests.setConnParams(1100000, actSpec[0]);
            xaresources = ra.getXAResources(actSpec);
            assertEquals(1, xaresources.length);
            
            for (int i = 0; i < xaresources.length; i++) {               
                try {
                    xaresources[i].commit(ImaTestHelper.getXid(), true);
                } catch (XAException xaex) {
                    xaex.printStackTrace();
                    assertEquals(null, xaex);
                }
            }

            AllTests.setConnParams(actSpec[0]);
            xaresources = ra.getXAResources(actSpec);
            assertEquals(1, xaresources.length);
            
            for (int i = 0; i < xaresources.length; i++) {   
              Xid recXids[] = xaresources[i].recover(XAResource.TMNOFLAGS);
              assertEquals(0, recXids.length);
            }
        } catch(ResourceException re) {
            XAException xae = (XAException)re.getCause();
            assertEquals(XAException.XAER_RMFAIL,xae.errorCode);
        }
    }
    
    public void testGetXAResourcesV20() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        ra.start(bootstrapCtx);
        
        ImaActivationSpec[] actSpec = new ImaActivationSpec[1];
        
        MessageEndpointFactory mef = new MockMessageEndpointFactory();
        actSpec[0] = new ImaActivationSpec();
        actSpec[0].setDestination("MyQueue");
        actSpec[0].setDestinationType("javax.jms.Topic");
        
        AllTests.setConnParams(2000000, actSpec[0]);
        ra.endpointActivation(mef, actSpec[0]);
        
        // TODO: Rework this test when required code is implemented.  For now, code test to pass.
        //@SuppressWarnings("unused")
        XAResource[] xaresources = null;
        
        try {
            xaresources = ra.getXAResources(actSpec);
            assertEquals(1, xaresources.length);
            
            for (int i = 0; i < xaresources.length; i++) {
                assertEquals("com.ibm.ima.ra.ImaRecoveryXAResource", xaresources[i].getClass().getName());
                try {
                    int timeout = xaresources[i].getTransactionTimeout();
                    assertEquals("Exepcted exception on call to getTransactionTimeout()","Got return value "+timeout);
                } catch (XAException xaex) {
                    assertEquals(XAException.XAER_PROTO, xaex.errorCode);
                }
                try {
                    xaresources[i].setTransactionTimeout(60);
                    assertEquals("Exepcted exception on call to setTransactionTimeout()","No exception from setTransactionTimeout()");
                } catch (XAException xaex) {
                    assertEquals(XAException.XAER_PROTO, xaex.errorCode);
                }
                
                try {
                    xaresources[i].end(ImaTestHelper.getXid(), XAResource.TMSUCCESS);
                    assertEquals("Exepcted exception on call to end()","No exception from end()");
                } catch (XAException xaex) {
                    assertEquals(XAException.XAER_PROTO, xaex.errorCode);
                }
                
                try {
                    xaresources[i].prepare(ImaTestHelper.getXid());
                    assertEquals("Exepcted exception on call to prepare()","No exception from prepare()");
                } catch (XAException xaex) {
                    assertEquals(XAException.XAER_PROTO, xaex.errorCode);
                }

                try {
                    xaresources[i].rollback(ImaTestHelper.getXid());
                } catch (XAException xaex) {
                    xaex.printStackTrace();
                    assertEquals(null, xaex);
                }
                
                try {
                    xaresources[i].start(ImaTestHelper.getXid(), XAResource.TMNOFLAGS);
                    assertEquals("Exepcted exception on call to start()","No exception from start()");
                } catch (XAException xaex) {
                    assertEquals(XAException.XAER_PROTO, xaex.errorCode);
                }
            }
            
            AllTests.setConnParams(2000000, actSpec[0]);
            xaresources = ra.getXAResources(actSpec);
            assertEquals(1, xaresources.length);
            
            for (int i = 0; i < xaresources.length; i++) {               
                try {
                    xaresources[i].forget(ImaTestHelper.getXid());
                } catch (XAException xaex) {
                    xaex.printStackTrace();
                    assertEquals(null, xaex);
                }
            }
            
            AllTests.setConnParams(2000000, actSpec[0]);
            xaresources = ra.getXAResources(actSpec);
            assertEquals(1, xaresources.length);
            
            for (int i = 0; i < xaresources.length; i++) {               
                try {
                    xaresources[i].commit(ImaTestHelper.getXid(), true);
                } catch (XAException xaex) {
                    xaex.printStackTrace();
                    assertEquals(null, xaex);
                }
            }

            AllTests.setConnParams(actSpec[0]);
            xaresources = ra.getXAResources(actSpec);
            assertEquals(1, xaresources.length);
            
            for (int i = 0; i < xaresources.length; i++) {   
              Xid recXids[] = xaresources[i].recover(XAResource.TMNOFLAGS);
              assertEquals(0, recXids.length);
            }
        } catch(ResourceException re) {
            XAException xae = (XAException)re.getCause();
            assertEquals(XAException.XAER_RMFAIL,xae.errorCode);
        }
    }

//    public void testJustGetConnPoolSize() throws Exception {
//        ResourceAdapter ra = new ImaResourceAdapter();
//        
//        String poolSize = ((ImaResourceAdapter)ra).getConnectionPoolSize();
//
//        // Current default is 10
//        assertEquals("10", poolSize);
//        
//    }
//    
//    public void testGetConnPoolSize() throws Exception {
//        ResourceAdapter ra = new ImaResourceAdapter();
//        ra.start(bootstrapCtx);
//        
//        MessageEndpointFactory mef = new MockMessageEndpointFactory();
//        ImaActivationSpec actSpec = new ImaActivationSpec();
//        actSpec.setDestination("MyQueue");
//        actSpec.setDestinationType("javax.jms.Topic");
//        
//        AllTests.setConnParams(actSpec);
//        ra.endpointActivation(mef, actSpec);
//        
//        String poolSize = ((ImaResourceAdapter)ra).getConnectionPoolSize();
//        
//        // Current default is 10
//        assertEquals("10", poolSize);
//        
//    }
//    
//    public void testJustSetConnPoolSize() throws Exception {
//        ResourceAdapter ra = new ImaResourceAdapter();
//        
//        String poolSize = "55";
//        ((ImaResourceAdapter)ra).setConnectionPoolSize(poolSize);
//        String getPoolSize = ((ImaResourceAdapter)ra).getConnectionPoolSize();
//
//        assertEquals("55", getPoolSize);
//        
//    }
//    
//    public void testSetConnPoolSize() throws Exception {
//        ResourceAdapter ra = new ImaResourceAdapter();
//        ra.start(bootstrapCtx);
//        
//        MessageEndpointFactory mef = new MockMessageEndpointFactory();
//        ImaActivationSpec actSpec = new ImaActivationSpec();
//        actSpec.setDestination("MyQueue");
//        actSpec.setDestinationType("javax.jms.Topic");
//        
//        AllTests.setConnParams(actSpec);
//        ra.endpointActivation(mef, actSpec);
//        
//        String poolSize = "55";
//        ((ImaResourceAdapter)ra).setConnectionPoolSize(poolSize);
//        String getPoolSize = ((ImaResourceAdapter)ra).getConnectionPoolSize();
//
//        assertEquals("55", getPoolSize);
//        
//    }
    
    public void testJustGetConnPool() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        
        ImaConnectionPool pool = ((ImaResourceAdapter)ra).getConnectionPool();

        assertEquals(null, pool);
        
    }
    
    public void testGetConnPool() throws Exception {
        ResourceAdapter ra = new ImaResourceAdapter();
        ra.start(bootstrapCtx);
        
        MessageEndpointFactory mef = new MockMessageEndpointFactory();
        ImaActivationSpec actSpec = new ImaActivationSpec();
        actSpec.setDestination("MyQueue");
        actSpec.setDestinationType("javax.jms.Topic");
        
        AllTests.setConnParams(actSpec);
        ra.endpointActivation(mef, actSpec);
        
        ImaConnectionPool pool = ((ImaResourceAdapter)ra).getConnectionPool();
        
        Connection conn = InboundTestHelper.getConnFromPool(pool, (ImaActivationSpec)actSpec);
        
        assertTrue(conn instanceof ImaConnection);
    }

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
