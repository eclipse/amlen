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

import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import com.ibm.ima.proxy.ImaAuthConnection;
import com.ibm.ima.proxy.ImaAuthenticator;
import com.ibm.ima.proxy.ImaJson;
import com.ibm.ima.proxy.test.TrWriter.LogLevel;

public class TestAuthenticator implements ImaAuthenticator {
	private final TrWriter trace;
	private final LogLevel logLevel;
	private Map<String, Object> users;

	public TestAuthenticator(TrWriter trace, String jsonFileName) {
		this.trace = trace;
		logLevel = LogLevel.valueOf(9);
		Path path = FileSystems.getDefault().getPath(jsonFileName);
		List<String> lines;
		try {
			lines = Files.readAllLines(path, Charset.forName("UTF-8"));
		} catch (IOException e) {
			trace.writeTraceLine(LogLevel.LOGLEV_ERROR, "TESTAUTH001", "Exception thrown trying to read " + jsonFileName);
			trace.writeException(e);
			throw new RuntimeException(e);
		}
		StringBuffer fileData = new StringBuffer();
		Iterator<String> iter = lines.iterator();
		while (iter.hasNext()) {
			String line = iter.next();
			fileData.append(line);
		}
		ImaJson json = new ImaJson();
		int rc = json.parse(fileData.toString());
		if (0 != rc) {
			trace.writeTraceLine(LogLevel.LOGLEV_ERROR, "TESTAUTH000"
				, "rc=" + rc + "from json.parse of " + fileData.toString());
		}
		users = json.toMap("User");
	}
	
	@Override
	public void authenticate(ImaAuthConnection connect) {
		String clientID = connect.getClientID();
        char   clientClass = (clientID.length()>1 && clientID.charAt(1)==':') ? clientID.charAt(0) : ' ';
        String orgID = "";
        if (clientID.length()>2 && clientID.charAt(1)==':') {
            int endpos = clientID.indexOf(':', 2);
            if (endpos > 0)
                orgID = clientID.substring(2, endpos);
        }
		try {
			trace.writeTraceLine(logLevel, "TESTAUTH002"
					, "Authentication request made for ClientID="
					+ connect.getClientID()
					+ ", UserID=" + connect.getUserID()
					+ ", Password=" + connect.getPassword()
					+ ", clientClass=" + clientClass
					+ ", organization=" + orgID
					+ ", certificateName=" + connect.getCertificateName()
					+ ", protocolUserID=" + connect.getProtocolUserID()
					+ ", clientAddress=" + connect.getClientAddress()
					);
			Object password = users.get(connect.getUserID());
			trace.writeTraceLine(logLevel, "TESTAUTH901", "Comparing 'samuserIP' to '"
					+ orgID +"'");
			System.out.println("Comparing 'samuserIP' to '"	+ orgID +"'");
			if ("samuserIP".equals(orgID)) {
				System.out.println("EQUAL");
				password = connect.getClientAddress();
			}

			if (null == password) {
				trace.writeTraceLine(logLevel, "TESTAUTH003"
					, "User " + connect.getUserID()
					  + " not found in user list");
				connect.authenticate(false);
				return;
			}
			if (connect.getPassword().equals(password)) {
				connect.authenticate(true);
				return;
			}
			trace.writeTraceLine(logLevel, "TESTAUTH004"
					, "Password "+connect.getPassword()
					  +" does not match expected password "+password);
			connect.authenticate(false);
		} catch (Exception e) {
			trace.writeException(e);
			connect.authenticate(false);
		}
	}

	@Override
	public void authorize(ImaAuthConnection connect) {
		// TODO Auto-generated method stub
		
	}

}
