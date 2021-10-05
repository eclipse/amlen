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

import java.io.PrintWriter;
import java.util.HashSet;
import java.util.LinkedList;

import javax.resource.ResourceException;
import javax.resource.spi.ConnectionEventListener;
import javax.resource.spi.ConnectionRequestInfo;
import javax.resource.spi.LocalTransaction;
import javax.resource.spi.ManagedConnection;
import javax.resource.spi.ManagedConnectionMetaData;
import javax.security.auth.Subject;
import javax.transaction.xa.XAResource;

import com.ibm.ima.jms.test.jca.TestProps;

public class EvilManagedConnection implements ManagedConnection {
	private PrintWriter logger;
	private final LinkedList<ConnectionEventListener> eventListeners = new LinkedList<ConnectionEventListener>();
	private final HashSet<EvilConnection> connectionHandles = new HashSet<EvilConnection>();
	private TestProps test;
	private EvilXAResource xar;
	private EvilResourceAdapter ra;
	private static EvilConnection _EC;
	
	public EvilManagedConnection(TestProps test, EvilResourceAdapter ra) {
	    super();
	    this.test = test;
	    this.ra = ra;
	    xar = new EvilXAResource(test,ra);
	}
	
	private void log(String msg) {
	    if (ra != null) {
	        ra.log("EvilManagedConnection", msg);
	    } else {
	        System.out.println("EvilManagedConnection: " + msg);   
	    }
    }
	
	public void setTest(TestProps test) {		
		this.test = test;
		if (this.xar != null) {
		    this.xar.setTest(test);
		}
	}
	
	public void setRA(EvilResourceAdapter ra) {
		log("setRA(ra) ra: " + ra);
		this.ra = ra;
	}
	
	@Override
	public void addConnectionEventListener(ConnectionEventListener listener) {
		log("addConnectionEventListener(listener)");
		log("\tlistener: " + listener);
		synchronized (eventListeners) {
			eventListeners.add(listener);
		}
	}

	@Override
	public void associateConnection(Object o) throws ResourceException {
		log("associateConnection(o)");
		log("\to: " + o);
		if (o instanceof EvilConnection) {
			EvilConnection ec = (EvilConnection) o;
			ec.setRA(ra);
			synchronized (this) {
				connectionHandles.add(ec);
			}
			// reassociate
		} else
			throw new ResourceException("Invalid connection type: " + o.getClass().getName());
	}

	@Override
	public void cleanup() throws ResourceException {
		log("cleanup()");
	}

	@Override
	public void destroy() throws ResourceException {
		log("destroy()");
	}

	@Override
	public Object getConnection(Subject subject, ConnectionRequestInfo conreqinfo) throws ResourceException {
		log("getConnection(subject, conreqinfo)");
		log("\tsubject: " + subject);
		log("\tconreqinfo: " + conreqinfo);
		log("\t-> new EvilConnection(test)");
		if (_EC == null) {
			_EC = new EvilConnection(test,ra);
		}
		return _EC;
	}

	@Override
	public LocalTransaction getLocalTransaction() throws ResourceException {
		log("getLocalTransaction() -> null");
		return null;
	}

	@Override
	public PrintWriter getLogWriter() throws ResourceException {
		return logger;
	}

	@Override
	public ManagedConnectionMetaData getMetaData() throws ResourceException {
		log("getMetaData() -> null");
		return null;
	}

	@Override
	public XAResource getXAResource() throws ResourceException {
		log("getXAResource() -> xar: " + xar);
		return xar;
	}

	@Override
	public void removeConnectionEventListener(ConnectionEventListener listener) {
		log("removeConnectionEventListener(listener)");
		synchronized (eventListeners) {
			eventListeners.remove(listener);
		}
	}

	@Override
	public void setLogWriter(PrintWriter log) throws ResourceException {
		logger = log;
	}

}
