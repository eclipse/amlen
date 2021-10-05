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
package com.ibm.ima.proxy.test;

import com.ibm.ima.proxy.ImaAuthConnection;

public class TestAuthConnection {
	ImaAuthConnection auth;
	private final char clientClass;
	private final String app;
	private final String orgID;
	private final String type;
	private final String id;
	
	TestAuthConnection(ImaAuthConnection data) {
		String clientID = data.getClientID();
        clientClass = (clientID.length()>1 && clientID.charAt(1)==':') ? clientID.charAt(0) : ' ';
		String[] pieces = data.getClientID().split(":");
		if (pieces.length < 2) throw new RuntimeException("Only 1 element in ClientID");
        if (clientID.length()>2 && clientID.charAt(1)==':') {
            int endpos = clientID.indexOf(':', 2);
            if (endpos > 0)
                orgID = clientID.substring(2, endpos);
            else orgID = "";
        } else orgID = "";
		auth = data;
		if ('a' == clientClass) {
			if (3 != pieces.length)
				throw new RuntimeException("Should be 3 elements in ClientID"+data.getClientID());
			app = pieces[2];
			type = null;
			id = null;
		} else if ('d' == clientClass) {
			if (4 != pieces.length)
				throw new RuntimeException("Should be 4 elements in ClientID"+data.getClientID());
			app = null;
			type = pieces[2];
			id = pieces[3];
		} else throw new RuntimeException("Illegal type '"+clientClass+"' in ClientID");
	}

	public ImaAuthConnection getAuth() {
		return auth;
	}

	public char getClientClass() {
		return clientClass;
	}

	public String getApp() {
		return app;
	}

	public String getOrg() {
		return orgID;
	}

	public String getType() {
		return type;
	}

	public String getId() {
		return id;
	}

}
