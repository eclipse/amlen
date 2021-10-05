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
package com.ibm.ima.jca.mdb;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.naming.NamingException;

import javax.resource.ResourceException;

import com.ibm.ima.jms.test.jca.TestProps;
import com.ibm.ima.jms.test.jca.TestProps.AppServer;
import com.ibm.ima.test.jca.ra.EvilConnection;
import com.ibm.ima.test.jca.ra.EvilConnectionFactory;

public class EvilTool {

	private TestProps test;

	public EvilTool(TestProps test) {
		this.test = test;
	}
	
	/**
	 * Create a connection using the ima.evilra ConnectionFactory
	 * TheEvilCon to be included as a branch in a global transaction.
	 * 
	 * @return true on success
	 */
	public boolean execute() {
		test.log("[EvilTool] execute()");
		try {
			Context ctx = new InitialContext();
			
			String jndiPrefix = test.appServer == AppServer.JBOSS ? "java:/" : "";
			EvilConnectionFactory ecf = (EvilConnectionFactory) ctx.lookup(jndiPrefix + "TheEvilCon");
			ecf.setTest(test);
			
			ecf.getRA().setTest(test); // aww yea, take that!
			test.log("[EvilTools] Trying to get connection of new ecf.getConnection()");
			EvilConnection ec = (EvilConnection) ecf.getConnection();
			
			test.log("[EvilTool] EvilConnection exists: " + ec);
			
			ec.close();
			test.log("[EvilTool] EvilConnection has closed");
			
			//test = ec.getTest();
			ctx.close();
		} catch (NamingException e) {
			test.log("execute NamingException...");
			test.log(e.getMessage());
			e.getRootCause().printStackTrace();
			e.printStackTrace();
			return false;
		} catch (ResourceException e) {
			// EVIL_FAIL_START comes back from call to getConnection() as a
			// resource exception, so we should still return true.
			if (test.failureType == TestProps.FailureType.EVIL_FAIL_START &&
					test.logs().contains("EVIL_FAIL_START")) {
				test.log(e.getMessage());
				return true;
			}
			test.log("execute ResourceException...");
			test.log(e.getMessage());			
			e.printStackTrace();
			return false;
		} catch (Exception e) {
			test.log("execute Exception...");
			test.log(e.getMessage());
			e.printStackTrace();
			return false;
		}
		return true;
	}
}
