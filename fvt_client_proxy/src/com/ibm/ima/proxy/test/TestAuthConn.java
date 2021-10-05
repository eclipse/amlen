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

import com.ibm.ima.proxy.ImaAuthenticator;
import com.ibm.ima.proxy.ImaJson;
import com.ibm.ima.proxy.ImaProxyListener;
import com.ibm.ima.proxy.ImaProxyResult;
import com.ibm.ima.proxy.test.TrWriter.LogLevel;

import java.util.Map;

/**
 * TestConfig is the main class that will be loaded by the proxy.
 * This is set by specifying the following in proxy.cfg:
 *   "JavaConfig": "com/ibm/ima/proxy/test/TestConfig",
 *   
 * From this class, tests are run for each group of proxy configuration
 * items:
 *   - Tenant
 *   - Server
 *   - Endpoint
 *   - User
 *   - Global
 *   
 * Each set of tests logs output to their own log file.
 * 
 *
 */
public class TestAuthConn {
	
	ImaProxyListener proxy;
	String  json;
    ImaJson parseobj = new ImaJson();
    ImaProxyResult ret; 
    Map<String, ?>  map;
    TrWriter _trWriter;
    LogLevel logLevel;
    String logFile;
    
	public TestAuthConn(ImaProxyListener listener) {
		logFile = "ClientProxyAuth.log";
		logLevel = LogLevel.valueOf(9);
		_trWriter = new TrWriter(logFile, logLevel);
        proxy = listener;
    	ImaAuthenticator auth = new TestAuthenticator(_trWriter, "users.json");
    	proxy.setAuthenticator(auth);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TESTAUTH998", "Starting Proxy authentication listener");
    }
    
    public void run() {
    	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TESTAUTH999", "In run()");
    	while (true) {
    		try {
    			Thread.sleep(10000);
    		} catch (InterruptedException e) {}
    	}
    }
    
    public void terminate(){
        proxy.terminate(0);
    }
    
	static String printSA(String [] s) {
		int  i;
		StringBuilder sb = new StringBuilder();
		for (i=0; i<s.length; i++) {
			if (i != 0)
				sb.append(", ");
			sb.append(s[i]);
		}
		return sb.toString();
	}
}
