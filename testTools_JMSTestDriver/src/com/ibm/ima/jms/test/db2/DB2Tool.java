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
package com.ibm.ima.jms.test.db2;

import java.security.AccessController;
import java.security.PrivilegedAction;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.sql.DataSource;

import com.ibm.ima.jms.test.jca.TestProps;
import com.ibm.ima.jms.test.jca.TestProps.AppServer;

public class DB2Tool {
	private TestProps test;
	private static final String simpleSelect = 
			"select count(*) from syscat.tables where type='T'";
	
	public DB2Tool(TestProps testProps) {
		test = testProps;
	}
	
	public boolean queryDB() {
		test.log("[DB2Tool] queryDB()");
		Connection con = null;
		try {
			Context ctxt = new InitialContext();
			String jndiPrefix = test.appServer == AppServer.JBOSS ? "java:/" : "";
			final DataSource dbds = (DataSource) ctxt.lookup(jndiPrefix + test.db2Name);
			test.log("[DB2Tool] dbds: " + dbds);
			con = AccessController.doPrivileged(new PrivilegedAction<Connection>() {
			    public Connection run() {
			        try {
                        return dbds.getConnection();
                    } catch (SQLException e) {
                        test.log("" + e);
                        return null;
                    }
			    }
			});
			
			if (con == null) {
				test.log("[DB2Tool] doPrivileged failed to get DB2 connection");
				return false;
			}

			PreparedStatement pstmt = con.prepareStatement(simpleSelect);
			
			ResultSet r = pstmt.executeQuery();
			if (test.failureType.equals(TestProps.FailureType.DB2_CRASH)) {
				test.log("[DB2Tool] Test is about to throw an Error on purpose");
				throw new Error("DB2 has \"failed\""); /* throw an Error to avoid catching */
			} else if (test.failureType.equals(TestProps.FailureType.DB2_NETWORK_DISCONNECT))
				{ /* not tested */ } 
			
			while (r.next())
				test.log("[DB2Tool] db select return value: " + r.getString(1));
			r.close();
			pstmt.close();
			con.close();
		} catch (Exception e) {
			test.log("[DB2Tool] error in queryDB(): " + e.getMessage());
			test.log("" + e);
			try {
				con.close();
			} catch (SQLException e1) {
				// do nothing
			}
			return false;
		}
		test.log("[DB2Tool] queryDB() exited OK");
		return true;
	}
}
