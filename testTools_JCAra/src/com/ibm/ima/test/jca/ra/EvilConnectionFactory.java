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

import javax.naming.NamingException;
import javax.naming.Reference;
import javax.resource.ResourceException;
import javax.resource.cci.Connection;
import javax.resource.cci.ConnectionFactory;
import javax.resource.cci.ConnectionSpec;
import javax.resource.cci.RecordFactory;
import javax.resource.cci.ResourceAdapterMetaData;
import javax.resource.spi.ConnectionManager;

import com.ibm.ima.jms.test.jca.TestProps;

public class EvilConnectionFactory implements ConnectionFactory {

	private static final long serialVersionUID = 4567474L;
	private Reference ref;
	private ConnectionManager cm;
	private EvilManagedConnectionFactory mCF;
	private TestProps test;
	private EvilResourceAdapter ra;
	private static EvilConnection _EC;
	
	public EvilConnectionFactory() {
	}
	
	public void setRA(EvilResourceAdapter ra) {
		log("setRA() ra: " + ra);
		this.ra = ra;
	}
	
	public EvilResourceAdapter getRA() {
		log("getRA() -> " + ra);
		return ra;
	}
	
	public EvilConnectionFactory(EvilManagedConnectionFactory mCF, ConnectionManager cm) {
        this.cm = cm;
        this.mCF = mCF;
    }

    private void log(String msg) {
        if (ra != null) {
            ra.log("EvilConnectionFactory", msg);
        } else {
            System.out.println("EvilConnectionFactory: " + msg);   
        }
    }
    
    public void setTest(TestProps test) {
    		this.test = test;
    }

	@Override
	public void setReference(Reference r) {
		log("setReference(r)");
		log("\tr: " + r);
		ref = r;
	}

	@Override
	public Reference getReference() throws NamingException {
		log("getReference()");
		log("\tr: " + ref);
		return ref;
	}

	@Override
	public Connection getConnection() throws ResourceException {
		log("getConnection()");
		
		if (cm == null) {
			log("\t -> new EvilConnection(test)");
			if (_EC == null) {
				_EC = new EvilConnection(test,ra);
			}
			return _EC;
		}
		log("\t -> getManagedConnection()");
		return getManagedConnection();
	}

	private Connection getManagedConnection() throws ResourceException {
        ((EvilManagedConnectionFactory) mCF).setTest(test);
		Connection con;

        con = (Connection) cm.allocateConnection(mCF, null);
        
        return con;
    }

    @Override
	public Connection getConnection(ConnectionSpec spec) throws ResourceException {
		log("getConnection(spec)");
		log("\tspec: " + spec);
		log("\t -> new EvilConnection(test)");
		log("\ttest: " + test);
		if (_EC == null) {
			_EC= new EvilConnection(test,ra);
		}
		return _EC;
	}

	@Override
	public ResourceAdapterMetaData getMetaData() throws ResourceException {
		log("getMetaData() -> null");
		return null;
	}

	@Override
	public RecordFactory getRecordFactory() throws ResourceException {
		log("getRecordFactory() -> null");
		return null;
	}

}
