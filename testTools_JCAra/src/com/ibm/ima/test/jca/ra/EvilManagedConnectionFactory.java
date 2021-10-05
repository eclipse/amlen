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

package com.ibm.ima.test.jca.ra;

import java.io.PrintWriter;
import java.util.HashSet;
import java.util.Set;

import javax.resource.ResourceException;
import javax.resource.spi.ConnectionManager;
import javax.resource.spi.ConnectionRequestInfo;
import javax.resource.spi.ManagedConnection;
import javax.resource.spi.ManagedConnectionFactory;
import javax.resource.spi.ResourceAdapter;
import javax.resource.spi.ResourceAdapterAssociation;
import javax.resource.spi.ValidatingManagedConnectionFactory;
import javax.security.auth.Subject;

import com.ibm.ima.jms.test.jca.TestProps;

public class EvilManagedConnectionFactory implements ResourceAdapterAssociation, ValidatingManagedConnectionFactory, ManagedConnectionFactory {
    private static final long serialVersionUID = 3759857493857295L;
    private EvilResourceAdapter   ra;
    private PrintWriter       logger; // useless, but required
    private TestProps test;
    private static EvilManagedConnection _EMC;
    private static EvilConnectionFactory _ECF;
    
    private void log(String msg) {
        if (ra != null) {
            ra.log("EvilManagedConnectionFactory", msg);
        } else {
            System.out.println("EvilManagedConnectionFactory: " + msg);
        }
    }
    
    public void setTest(TestProps test) {
        this.test = test;
    }

    public EvilManagedConnectionFactory() {
    	log("constructor()");
    }

    @Override
    public ResourceAdapter getResourceAdapter() {
    	log("getResourceAdapter(): " + ra);
        return ra;
    }

    @Override
    public void setResourceAdapter(ResourceAdapter ra) throws ResourceException {
        log("setResourceAdapter(ra): " + ra);
    	this.ra = (EvilResourceAdapter) ra;
    }

	@Override
	public Object createConnectionFactory() throws ResourceException {
		log("createConnectionFactory()");
		return createConnectionFactory(null);
	}

	@Override
	public Object createConnectionFactory(ConnectionManager cm) throws ResourceException {
		log("createConnectionFactory(cm): " + cm);
		if (_ECF == null) {
			_ECF = new EvilConnectionFactory(this,cm);
			_ECF.setRA(ra);
		} else {
		    _ECF.setTest(test);
		}
		return _ECF;
	}

	@Override
	public ManagedConnection createManagedConnection(Subject subject, ConnectionRequestInfo conreqinfo) throws ResourceException {
		log("createManagedConnection(subject, conreqinfo)");
		
		if (_EMC == null) {
			_EMC = new EvilManagedConnection(test,ra);
		} else {
		    _EMC.setTest(test);
		}
		return _EMC;
	}

	@Override
	public PrintWriter getLogWriter() throws ResourceException {
		return logger;
	}

	@Override
	public ManagedConnection matchManagedConnections(@SuppressWarnings("rawtypes") Set set, Subject subject, ConnectionRequestInfo conreqinfo) throws ResourceException {
	    if (_EMC != null) {
	        _EMC.setTest(test);
	    }
		log("matchManagedConnections: "+set.toArray()[0]);
		return (ManagedConnection) set.toArray()[0];
	}

	@Override
	public void setLogWriter(PrintWriter pw) throws ResourceException {
		logger = pw;
	}

	@SuppressWarnings("rawtypes")
	@Override
	public Set getInvalidConnections(Set set) throws ResourceException {
		log("getInvalidConnections(set) -> new HashSet()");
		log("\tset: " + set);
		return new HashSet();
	}
	
	@Override
	public boolean equals(Object o) {
		return super.equals(o);
	}
	
	@Override
	public int hashCode() {
		return super.hashCode();
	}

}
