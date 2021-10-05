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

import com.ibm.ima.proxy.test.TrWriter;
import com.ibm.ima.proxy.test.TrWriter.LogLevel;
import com.ibm.ima.proxy.ImaJson;
import com.ibm.ima.proxy.ImaProxyListener;
import com.ibm.ima.proxy.ImaProxyResult;

import java.util.HashMap;
import java.util.Map;

/**
 * Test proxy tenant configuration.
 * 
 *
 */
public class TestTenantConfig {
	
	ImaProxyListener proxy;
	String  json;
    ImaJson parseobj = new ImaJson();
    ImaProxyResult ret; 
    Map<String, ?>  map;
    TrWriter _trWriter;
    String logFile;
    LogLevel logLevel;
    
	public TestTenantConfig(ImaProxyListener listener) {
		// TODO: get log properties from somewhere
		logFile = "./TestTenantConfig.log";
		logLevel = LogLevel.valueOf(9);
		_trWriter = new TrWriter(logFile, logLevel);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG", "Beginning Tenant Configuration Tests");
        proxy = listener;
    }
	
	public boolean runTenantTests() {
		boolean result = true;
		if (!TestTenantFred()) {
			result = false;
		}
		if (!GetandSetTenantFreddy()) {
			result = false;
		}
		if (!getTenantList()) {
			result = false;
		}
		if (!deleteTenantTest()) {
			result = false;
		}
		return result;
	}
    
	public boolean TestTenantFred(){
		try {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG00", "BEGIN TestTenantFred");
            map = proxy.getTenant("fred");
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG01", "{0}", map); //before changes are made
            
            HashMap<String,Object>  value = new HashMap<String,Object>();
            value.put("Server", "Server2");
            value.put("MaxConnections", 10);
            value.put("Enabled", false);
            value.put("Authenticate", "Token");
            value.put("MaxQoS", 0);
            value.put("AllowSysTopic", true);
            value.put("AllowDurable", false);
            value.put("UserIsClientID", false);
            value.put("CheckUser", true);
            value.put("AllowShared", true);
            value.put("RequireSecure", false);
            value.put("AllowRetain", true);
            value.put("RemoveUser", false);
            value.put("CheckTopic", "IoT2");
            value.put("RLACAppEnabled", false);
            value.put("RLACAppDefaultGroup", false);
            value.put("MessageRouting", 0);
			proxy.setTenant("fred", value);
			
            map = proxy.getTenant("fred");
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG02", "Set Tenant");
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG03", "{0}", map); //after changes are made
            
            ret = proxy.getJSON("", "tenant=fred");
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG04", "{0}", ret.getStringValue()); //outputs JSON string
            
            //check validity of parsed json message
            if(parseobj.parse(ret.getStringValue()) == -2){
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "TNTCFG05", "Invalid JSON message");
            	throw new RuntimeException();
            } else {
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG06", "Valid JSON message");
            }
            
            if (!TestConfig.compareMap(map, value, _trWriter)) {
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "TNTCFG07", "Map did not match");
            	throw new RuntimeException();
            }
            
		} catch (Exception e) {
			_trWriter.writeException(e);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "TNTCFG08", "END TestTenantFred -- Test Failure!");
			return false;
		} 
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG08", "END TestTenantFred -- Test Success!");
		return true;
		
	}
	
	public boolean GetandSetTenantFreddy(){
		try {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG10", "BEGIN Test GetandSetTenantFreddy");
            
            HashMap<String,Object>  value = new HashMap<String,Object>();
            value.put("Server", "Server2");
            value.put("MaxConnections", 10);
            value.put("Enabled", false);
            value.put("RequireCertificate", false);
            value.put("MaxQoS", 0);
            value.put("Authenticate",  "Token");
            value.put("AllowSysTopic", true);
            value.put("AllowDurable", false);
            value.put("CheckUser", true);
            value.put("UserIsClientID", false);
            value.put("AllowShared", true);
            value.put("RequireSecure", false);
            value.put("AllowAnonymous", true);
            value.put("AllowRetain", true);
            value.put("RemoveUser", false);
            value.put("RequireUser", true);
            value.put("Port", 16105);
            value.put("CheckTopic", "IoT2");
            value.put("RLACAppEnabled", false);
            value.put("RLACAppDefaultGroup", false);
            value.put("MessageRouting", 0);
            value.put("FairUseMethod", 0);
			proxy.setTenant("freddy", value);
			
            map = proxy.getTenant("freddy");
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG11", "Get Tenant");
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG12", "{0}",map); //after changes are made
            
            ret = proxy.getJSON("", "tenant=freddy");
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG13", "{0}", ret.getStringValue()); //outputs JSON string
            
            //check validity of parsed json message
            if(parseobj.parse(ret.getStringValue()) == -2){
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "TNTCFG14", "Invalid JSON message");
            	throw new RuntimeException();
            } else {
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG15", "Valid JSON message");
            }
            
            if (!TestConfig.compareMap(map, value, _trWriter)) {
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "TNTCFG16", "Map did not match");
            	throw new RuntimeException();
            }
            
		} catch (Exception e) {
			_trWriter.writeException(e);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "TNTCFG17", "END Test GetandSetTenantFreddy -- Test Failure!");
			return false;
		} 
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG17", "END Test GetandSetTenantFreddy -- Test Success!");
		return true;
		
	}
	
	public boolean getTenantList(){
		boolean result = true;
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG20", "BEGIN Test getTenantList");
		String[] matchStrings = { "*", "fred*", "junk*" };
		
		for (String matchString : matchStrings) {
				try {
					// All tenants
					_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG21", "getTenantList(\"{0}\")", matchString);
					String[] tenantList = proxy.getTenantList(matchString);
					_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG22", "{0}", TestConfig.printSA(tenantList));
				} catch (Exception e) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG23", "getTenantList(\"{0}\") failed", matchString);
					_trWriter.writeException(e);
					result = false;
				}
		}
			
		if (result)
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG24", "END Test getTenantList -- Test Success!");
			else 
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG24", "END Test getTenantList -- Test Failure!");
			return result;
	}
	
	public boolean deleteTenantTest() {
		boolean result = true;
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG30", "BEGIN Test DeleteTenant");
		
		String[][] tenants = {
				// name     force    expected result
				{"tenant1", "false", "false"},
				{"tenant2", "true", "false"},
				{"tenant3", "true", "false"}
		};
		
		for (String[] tenant : tenants) { 
			try {
				// Delete with force=false
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG31", "deleteTenant(\"{0}\", {1})", tenant[0], tenant[1]);
				boolean resp =proxy.deleteEndpoint(tenant[0], Boolean.valueOf(tenant[1]));
				try {
					Map<String,Object> map;
					map = proxy.getTenant(tenant[0]);
					if (map != null) {
						// why isn't map null?
					}
				} catch (Exception e) {
					_trWriter.writeException(e);
				}
				if (resp != Boolean.valueOf(tenant[2])) {
					_trWriter.writeTraceLine(logLevel, "TNTCFG32", "deleteTenant("+tenant[0]+" returned "+resp+" expected "+tenant[2]);
					result = false;
				}
			} catch (Exception e) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG33", "deleteTenant(\"{0}\", {1}) failed", tenant, tenant[1]);
				_trWriter.writeException(e);
				result = false;
			}
		}
		
		if (result)
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG34", "END Test DeleteTenant -- Test Success!");
		else 
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "TNTCFG34", "END Test DeleteTenant -- Test Failure!");
		return result;
	}
}
