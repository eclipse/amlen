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
/**
 */
package com.ibm.ima.ra.testutils;

import java.util.Timer;

import javax.resource.spi.BootstrapContext;
import javax.resource.spi.UnavailableException;
import javax.resource.spi.XATerminator;
import javax.resource.spi.work.WorkContext;
import javax.resource.spi.work.WorkManager;
import javax.transaction.TransactionSynchronizationRegistry;

/**
 *
 */
public class MockBootstrapContext implements BootstrapContext {
	Timer timer = null;
	TransactionSynchronizationRegistry transSyncReg = null;
	WorkManager workManager = null;
	XATerminator xaTerminaor = null;
	

	/* (non-Javadoc)
	 * @see javax.resource.spi.BootstrapContext#createTimer()
	 */
	@Override
	public Timer createTimer() throws UnavailableException {
		// TODO Auto-generated method stub
		return timer;
	}

	/* (non-Javadoc)
	 * @see javax.resource.spi.BootstrapContext#getTransactionSynchronizationRegistry()
	 */
	@Override
	public TransactionSynchronizationRegistry getTransactionSynchronizationRegistry() {
		// TODO Auto-generated method stub
		return transSyncReg;
	}

	/* (non-Javadoc)
	 * @see javax.resource.spi.BootstrapContext#getWorkManager()
	 */
	@Override
	public WorkManager getWorkManager() {
		// TODO Auto-generated method stub
	    if(workManager == null) {
	        workManager = new MockWorkManager();
	    }
		return workManager;
	}

	/* (non-Javadoc)
	 * @see javax.resource.spi.BootstrapContext#getXATerminator()
	 */
	@Override
	public XATerminator getXATerminator() {
		// TODO Auto-generated method stub
		return xaTerminaor;
	}

	/* (non-Javadoc)
	 * @see javax.resource.spi.BootstrapContext#isContextSupported(java.lang.Class)
	 */
	@Override
	public boolean isContextSupported(Class<? extends WorkContext> arg0) {
		// TODO Auto-generated method stub
		return false;
	}
	
	// Unit test helper methods
	public Timer getTimer() {
		return timer;
	}

}
