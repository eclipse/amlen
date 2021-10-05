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
 * Test proxy endpoint configuration.
 * 
 *
 */
public class TestEndpointConfig {
	
	ImaProxyListener proxy;
	String  json;
    ImaJson parseobj = new ImaJson();
    ImaProxyResult ret; 
    TrWriter _trWriter;
    String logFile;
    LogLevel logLevel;
    
    public String[][] properties = {
    		{"Authentication","Basic"},
    		{"Certificate",""},
    		{"Ciphers","Fast"},
    		{"DomainSeparator",":"},
    		{"Interface","*"},
    		{"Key",""},
    		{"MaxMessageSize","4194304"},
    		{"Method","TLSv1"},
    		{"Port","1883"},
    		{"Protocol","mqtt"},
    		{"Secure","false"},
    		{"Transport","tcp"},
    		{"UseClientCertificate","false"},
    		{"UseClientCipher","false"},
    };
    
	public TestEndpointConfig(ImaProxyListener listener) {
		// TODO: get log properties from somewhere
		logFile = "./TestEndpointConfig.log";
		logLevel = LogLevel.valueOf(9);
		_trWriter = new TrWriter(logFile, logLevel);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "Beginning Endpoint Configuration Tests");
        proxy = listener;
    }
	
	public boolean runEndpointTests() {
		boolean result = true;
		if (!TestEndpointBasic()) {
			result = false;
		}
		if (!getEndpointTest()) {
			result = false;
		}
		if (!setEndpointTest()) {
			result = false;
		}
		if (!listEndpointTest()) {
			result = false;
		}
		if (!updateEndpointTest()) {
			result = false;
		}
		if (!deleteEndpointTest()) {
			result = false;
		}
		return result;
	}
    
	public boolean getEndpointTest() {
		boolean result = true;
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "BEGIN Get Endpoint Test");

		String[] endpoints = { "mqtt", "mqtts" };
		
		for (String endpoint : endpoints) {
			Map<String, Object> map = null;
			try {
				// Get an endpoint with a subset of properties defined
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "getEndpoint(\"{0}\")", endpoint);
				map = proxy.getEndpoint(endpoint);
				if (map == null) {
					result = false;
				} else {
					for (Object id : map.keySet()) {
						_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "Endpoint mqtt property: {0}={1}", id.toString(), map.get(id));
					}
				}
			} catch (Exception e) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "getEndpoint(\"{0}\") failed", endpoint);
				_trWriter.writeException(e);
				result = false;
			}
		}
		
		if (result)
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "END Get Endpoint Test -- Test Success!");
		else 
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "END Get Endpoint Test -- Test Failure!");
		return result;
	}
	
	public boolean setEndpointTest() {
		boolean result = true;
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "BEGIN Set Endpoint Test");
		
		Map<String,Object> endpoint1 = new HashMap<String,Object>();
		// Set all properties
		/*
		{"Authentication","Basic"},
		{"Certificate",""},
		{"Ciphers","Fast"},
		{"DomainSeparator",":"},
		{"Interface","*"},
		{"Key",""},
		{"MaxMessageSize","4194304"},
		{"Method","TLSv1"},
		{"Port","1883"},
		{"Protocol","mqtt"},
		{"Secure","false"},
		{"Transport","tcp"},
		{"UseClientCertificate","false"},
		{"UseClientCipher","false"},
		*/
		endpoint1.put("Port", 1884);
		endpoint1.put("UseClientCipher", false);
		endpoint1.put("UseClientCertificate", false);
		endpoint1.put("Secure", false);
		endpoint1.put("Method", "TLSv1");
		endpoint1.put("Interface", "0.0.0.0");
		endpoint1.put("MaxMessageSize", 4194304);

		
		Map<String,Object> endpoint2 = new HashMap<String,Object>();
		// Set a subset of properties
		endpoint2.put("Interface", "");
		endpoint2.put("Port", "1885");
		endpoint2.put("Authentication", "None");
		
		Map<String,Object> endpoint3 = new HashMap<String,Object>();
		// Set all properties to ""
		for (String[] id : properties) {
			endpoint3.put(id[0], "");
		}
		
		Map<String,Map<String,Object>> endpoints = new HashMap<String,Map<String,Object>>();
		endpoints.put("testendpoint1", endpoint1); // succeeds
		endpoints.put("testendpoint2", endpoint2); // fails
		endpoints.put("testendpoint3", endpoint3); // fails
		
		for (String endpoint : endpoints.keySet()) {
			try {
				Map<String,Object> map = endpoints.get(endpoint);
				
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "setEndpoint(\"{0}\", map)", endpoint);
				proxy.postJSON(null, "endpoint="+endpoint, "{\"Port\":"+map.get("Port")+"}");
				// need to post JSON before you can do setEndpoint
				proxy.setEndpoint(endpoint, map);
				try {
					map = proxy.getEndpoint(endpoint);
				} catch (Exception e) {
					// if settings were invalid, we most likely will end up here
					if (endpoint.equals("testendpoint1")) {
						_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "setEndpoint(\"{0}\", map) failed", endpoint);
						result = false;
					}
					_trWriter.writeException(e);
				}
			} catch (Exception e) {
				if (endpoint.equals("testendpoint1")) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "setEndpoint(\"{0}\", map) failed", endpoint);
					result = false;
				}
				_trWriter.writeException(e);
			}
		}
		
		if (result)
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "END Set Endpoint Test -- Test Success!");
		else 
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "END Set Endpoint Test -- Test Failure!");
		return result;
	}
	
	public boolean listEndpointTest() {
		boolean result = true;
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "BEGIN List Endpoint Test");
		
		String[] matchStrings = { "*", "test*", "garbage*" };
		
		for (String matchString : matchStrings) {
			try {
				// All endpoints
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "getEndpointList(\"{0}\")", matchString);
				String[] endpointList = proxy.getEndpointList(matchString);
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "{0}", TestConfig.printSA(endpointList));
			} catch (Exception e) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "getEndpointList(\"{0}\") failed", matchString);
				_trWriter.writeException(e);
				result = false;
			}
		}
		
		if (result)
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "END List Endpoint Test -- Test Success!");
		else 
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "END List Endpoint Test -- Test Failure!");
		return result;
	}
	
	public boolean updateEndpointTest() {
		boolean result = true;
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "BEGIN Update Endpoint Test");
		
		Map<String,Object> endpoint1 = new HashMap<String,Object>();
		endpoint1.put("Port", 1887);
		endpoint1.put("UseClientCipher", false);
		endpoint1.put("UseClientCertificate", false);
		endpoint1.put("Secure", false);
		endpoint1.put("Method", "TLSv1.2");
		endpoint1.put("Interface", "0.0.0.0");
		endpoint1.put("MaxMessageSize", 4096);
		
		Map<String,Map<String,Object>> endpoints = new HashMap<String,Map<String,Object>>();
		endpoints.put("testendpoint1", endpoint1);

		for (String endpoint : endpoints.keySet()) {
			try {
				Map<String,Object> map = endpoints.get(endpoint);
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "setEndpoint(\"{0}\", map)", endpoint);
				proxy.setEndpoint(endpoint, map);
				try {
					map = proxy.getEndpoint(endpoint);
				} catch (Exception e) {
					_trWriter.writeException(e);
				}
			} catch (Exception e) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "setEndpoint(\"{0}\", map) failed", endpoint);
				_trWriter.writeException(e);
				result = false;
			}
		}
		
		if (result)
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "END Update Endpoint Test -- Test Success!");
		else 
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "END Update Endpoint Test -- Test Failure!");
		return result;
	}
	
	public boolean deleteEndpointTest() {
		boolean result = true;
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "BEGIN Delete Endpoint Test");
		
		String[][] endpoints = {
				{"testendpoint1", "false"},
				{"testendpoint2", "true"},
				{"testendpoint3", "true"}
		};
		
		for (String[] endpoint : endpoints) { 
			try {
				// Delete with force=false
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "deleteEndpoint(\"{0}\", {1})", endpoint[0], endpoint[1]);
				proxy.deleteEndpoint(endpoint[0], Boolean.valueOf(endpoint[1]));
				try {
					Map<String,Object> map;
					map = proxy.getEndpoint(endpoint[0]);
					if (map != null) {
						// why isn't map null?
					}
				} catch (Exception e) {
					_trWriter.writeException(e);
				}
			} catch (Exception e) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "deleteEndpoint(\"{0}\", {1}) failed", endpoint, endpoint[1]);
				_trWriter.writeException(e);
				result = false;
			}
		}
		
		if (result)
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "END Delete Endpoint Test -- Test Success!");
		else 
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "END Delete Endpoint Test -- Test Failure!");
		return result;
	}
	
	public boolean TestEndpointBasic(){
		try {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "Endpoint");
			Map<String, Object> map = proxy.getEndpoint("mqtts");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "{0}", map);

			ret = proxy.getJSON("", "endpoint=mqtts");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "{0}", ret.getStringValue());
			parseobj.parse(ret.getStringValue());

			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "getEndpointList");
			String[] endpointList = proxy.getEndpointList("*");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "{0}", TestConfig.printSA(endpointList));

			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "setEndpoint");
			HashMap<String,Object> emap = new HashMap<String,Object>();
			emap.put("Ciphers", "Best");
			emap.put("Port", 8890);
			emap.put("UseClientCipher", true);
			proxy.setEndpoint("mqtts", emap);
			map = proxy.getEndpoint("mqtts");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "{0}", map);

			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "setEndpoint json");
			proxy.postJSON(null, "endpoint=mqtts", "{\"Port\":9999,\"Ciphers\":\"Medium\"}");
			map = proxy.getEndpoint("mqtts");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "{0}", map);
		} catch (Exception e) {
			_trWriter.writeException(e);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "EPCFG", "Test Failure!");
			return false;
		} 
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "EPCFG", "Test Success!");
		return true;
		
	}
}
