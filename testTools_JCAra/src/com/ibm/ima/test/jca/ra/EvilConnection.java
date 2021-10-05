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

import java.io.Serializable;

import javax.resource.ResourceException;
import javax.resource.cci.Connection;
import javax.resource.cci.ConnectionMetaData;
import javax.resource.cci.Interaction;
import javax.resource.cci.LocalTransaction;
import javax.resource.cci.ResultSetInfo;

import com.ibm.ima.jms.test.jca.TestProps;

public class EvilConnection implements Serializable, Connection {

    private static final long serialVersionUID = -819562023998352606L;
    private EvilResourceAdapter ra;
    
    private void log(String msg) {
        if (ra != null) {
            ra.log("EvilConnection", msg);
        } else {
            System.out.println("EvilConnection: " + msg);
        }
    }

    public EvilConnection(TestProps test, EvilResourceAdapter ra) {
    	log("constructor(test)");
    	setRA(ra);
    }
    
    public void setRA(EvilResourceAdapter ra) {
    	log("setRA(ra) ra: " + ra);
    	this.ra = ra;
    }
    
    public EvilResourceAdapter getRA() {
    	log("getRA() ->: " +  ra);
    	return this.ra;
    }
    
    public TestProps getTest() {
    	log("getTest() -> ra.getTest()");
    	return ra.getTest();
    }

	@Override
	public void close() throws ResourceException {
		log("close()");
	}

	@Override
	public Interaction createInteraction() throws ResourceException {
		log("createInteraction() -> null");
		return null;
	}

	@Override
	public LocalTransaction getLocalTransaction() throws ResourceException {
		log("createLocalTransaction() -> null");
		return null;
	}

	@Override
	public ConnectionMetaData getMetaData() throws ResourceException {
		log("getMetaData() -> null");
		return null;
	}

	@Override
	public ResultSetInfo getResultSetInfo() throws ResourceException {
		log("getResultSetInfo() -> null");
		return null;
	}
}
