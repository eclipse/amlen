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

import com.ibm.ima.proxy.ImaJson;
import com.ibm.ima.proxy.ImaProxyListener;
import com.ibm.ima.proxy.ImaProxyResult;
import com.ibm.ima.proxy.test.TrWriter.LogLevel;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

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
public class TestConfig {
	
	ImaProxyListener proxy;
	String  json;
    ImaJson parseobj = new ImaJson();
    ImaProxyResult ret; 
    Map<String, ?>  map;
    static TrWriter _trWriter;
    LogLevel logLevel;
    String logFile;
    
	public TestConfig(ImaProxyListener listener) {
		logFile = "ClientProxy.log";
		logLevel = LogLevel.valueOf(9);
		_trWriter = new TrWriter(logFile, logLevel);
        proxy = listener;
    }
    
    public void run() {
    	TestTenantConfig tenantTester = new TestTenantConfig(proxy);
    	if (tenantTester.runTenantTests()) {
    		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "PROXYCFG", "Tenant Tests Passed!");
    	} else {
    		_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "PROXYCFG", "Tenant Tests Failed!");
    	}
        
        TestUserConfig userTester = new TestUserConfig(proxy);
        if (userTester.runUserTests()) {
    		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "PROXYCFG", "User Tests Passed!");
    	} else {
    		_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "PROXYCFG", "User Tests Failed!");
    	}
        
        TestServerConfig serverTester = new TestServerConfig(proxy);
        if (serverTester.runServerTests()) {
    		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "PROXYCFG", "Server Tests Passed!");
    	} else {
    		_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "PROXYCFG", "Server Tests Failed!");
    	}
        
        TestEndpointConfig endpointTester = new TestEndpointConfig(proxy);
        if (endpointTester.runEndpointTests()) {
    		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "PROXYCFG", "Endpoint Tests Passed!");
    	} else {
    		_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "PROXYCFG", "Endpoint Tests Failed!");
    	}
        
        TestGlobalConfig globalTester = new TestGlobalConfig(proxy);
        if (globalTester.runGlobalConfigTests()) {
    		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "PROXYCFG", "Global Tests Passed!");
    	} else {
    		_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "PROXYCFG", "Global Tests Failed!");
    	}
        
        //terminate the proxy after tests have run
        terminate();
       
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
	
	static boolean compareMap(Map<String,?> a, Map<String,?> b, TrWriter trWriter) {
		//check that both maps are equal
        Set<String> removedKeys = new HashSet<String>(a.keySet());
        removedKeys.removeAll(b.keySet());
        if (removedKeys.isEmpty()) {
        	trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "PROXYCFG", "Removed all keys");
        	return true;
        } else {
        	trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "PROXYCFG", "Keys did not match");
        	Iterator<String> iter = removedKeys.iterator();
        	while (iter.hasNext()) {
        		trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "PROXYCFG", "{0}", iter.next());
        	}
        	return false;
        }
	}
}
