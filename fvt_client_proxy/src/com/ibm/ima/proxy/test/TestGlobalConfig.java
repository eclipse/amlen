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

import java.util.Map;

/**
 * Test proxy global configuration.
 * 
 *
 */
public class TestGlobalConfig {
	
	ImaProxyListener proxy;
	String  json;
    ImaJson parseobj = new ImaJson();
    ImaProxyResult ret; 
    Map<String, ?>  map;
    TrWriter _trWriter;
    String logFile;
    LogLevel logLevel;
    
    public String[][] properties = {
    		/* Property name        Expected value */
    		{"Affinity.tcpiop.0",	"0x1"},
    		{"Affinity.tcpiop.1",	"0x2"},
    		{"Affinity.tcpiop.2",	"0x4"},
    		{"Affinity.tcpconnect",	"0x80"},
    		{"Affinity.tcplisten",	"0x40"},
    		/* TODO: These paths will have to be based on test environment. */
    		{"Classpath",			"/niagara/proxy/lib/imaproxy_config.jar:/niagara/proxy/test/clientproxytest.jar"},
    		//{"ConfigPath",			""},
    		//{"ConnectionLogFile",	"connect.log"},
    		{"ConnectionLogLevel",	"Normal"},
    		{"FileLimit",			"1024"},
    		{"IOThreads",			"3"},
    		{"JavaConfig",			"com/ibm/ima/proxy/test/TestConfig"},
    		/* TODO: java home will also depend on the test environment. */
 //   		{"JavaHome",			"/opt/ibm/ibm-java-x86_64-71"},
    		//{"JavaJVM",				""},
    		{"JavaOption",			"-Xrs"},
    		{"KeyStore",			"keystore"},
    		//{"LogFile",				"log.log"},
    		{"LogLevel",			"Normal"},
    		{"LogUnitTest",			"1"},
    		{"MqttProxyProtocol",	"1"},
    		{"TcpMaxCon",			"65535"},
    		{"TraceFile",			"trc.log"},
    		{"TraceLevel",			"6,mqtt=6,tls=9,http=9"},
    		//{"TraceMax",			""},
    		{"TraceMessageData",	"1000"},
    		{"TrustStore",			"truststore"},
    };
    
	public TestGlobalConfig(ImaProxyListener listener) {
		// TODO: get log properties from somewhere
		logFile = "./TestGlobalConfig.log";
		logLevel = LogLevel.valueOf(9);
		_trWriter = new TrWriter(logFile, logLevel);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG", "Beginning Global Configuration Tests");
        proxy = listener;
    }
    
	public boolean runGlobalConfigTests() {
		boolean result = true;
		if (!getSetAllGlobalConfig()) {
			result = false;
		}
		if (!setNonExistantConfig()) {
			result = false;
		}
		if (!TestErrorString()) {
			result = false;
		}
		return result;
	}
	
	public boolean setNonExistantConfig(){
		try {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG", "BEGIN TEST: Set non-existant property 'Fred'");
			proxy.setConfigItem("Fred", 7);  /* Setting unknown is ignored */
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG", "END TEST: Set non-existant property -- Test Success!");
			return true;
		} catch (Exception e) {
			_trWriter.writeException(e);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "GLOBALCFG", "END TEST: Set non-existant property -- Test Failure!");
			return false;
		} 
	}
	
	public boolean getSetAllGlobalConfig() {
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG", "BEGIN TEST: Get then Set all global config items");
		/* id[0] = property name, id[1] = expected output */
		boolean result = true;
		for (String[] id : properties) {
			Object response = null;
			try {
				response = proxy.getConfigItem(id[0]);
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG", "getConfigItem completed successfully! {0}={1}", id[0], response.toString());
				if (!checkResult(id[0], response, id[1])) {
					_trWriter.writeTraceLine(logLevel, "GLOBALCFG", "response: "+response+", expected: "+id[1]);
					_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "GLOBALCFG", "getConfigItem {0} checkResult failed", id[0]);
					result = false;
				}
			} catch (Exception e) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "GLOBALCFG", "getConfigItem failed for property {0}!", id[0]);
				_trWriter.writeException(e);
			}
			if (response != null) {
				try {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG", "setConfigItem {0}={1}", id[0], response.toString());
					proxy.setConfigItem(id[0], response);
					_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG", "setConfigItem completed successfully!");
				} catch (Exception e) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "GLOBALCFG", "setConfigItem failed for property {0}!", id[0]);
					_trWriter.writeException(e);
					result = false;
				}
			}
		}
		proxy.setConfigItem("TrustStore", "truststoreA");
		Object response = proxy.getConfigItem("TrustedCertificateDir");
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG", "getConfigItem TrustedCertificateDir {0}", response.toString());
		proxy.setConfigItem("TrustedCertificateDir", "Atruststore");
		response = proxy.getConfigItem("TrustedCertificateDir");
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG", "getConfigItem TrustedCertificateDir {0}", response.toString());
		proxy.setConfigItem("TrustedCertificateDir", "Atruststore");
		response = proxy.getConfigItem("TrustedCertificateDir");
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG", "getConfigItem TrustedCertificateDir {0}", response.toString());
		if (result) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG", "END TEST: Get then Set all global config items -- Test Success!");
		} else {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "GLOBALCFG", "END TEST: Get then Set all global config items -- Test Failure!");
		}
		return result;
	}
	
	public boolean checkResult(String name, Object id, String expected) {
		if (id instanceof String) {
			if (((String)id).equals(expected)) return true;
			else {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "GLOBALCFG", "For object :"+name+",  expected "+expected+", got: "+id);
				return false;
			}
			//return (((String)id).equals(expected)) ? true : false;
		} else if (id instanceof Integer) {
			int result = (Integer)id;
			if (Integer.toString(result).equals(expected)) return true;
			else {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "GLOBALCFG", "For object :"+name+",  expected "+expected+", got: "+id);
				return false;
			}
		} else if (id instanceof Boolean) {
			if ((Boolean)id) {
				if ("1".equals(expected)) return true;
				else _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "GLOBALCFG", "For object :"+name+",  expected "+expected+", got: "+id);
			} else {
				if ("0".equals(expected)) return true;
				else _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "GLOBALCFG", "For object :"+name+",  expected "+expected+", got: "+id);
			}
		}
		return false;
	}
	
	public boolean TestErrorString(){
    	boolean result = true;
    	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG", "BEGIN TEST: TestErrorString");
    	try {
    		int[] errorcodes = new int[] {0,1,2,3,4,5,6,7,8,9};
            for(int errorcode : errorcodes){
            	String errorstring = proxy.getErrorString(errorcode);
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG","Checking errorcode{0}: {1}", errorcode, errorstring);
            }
    	} catch (Exception e) {
    		_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "GLOBALCFG", "END TEST: TestErrorString failed");
			_trWriter.writeException(e);
			result = false;
    	}
    	
    	if (result) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "GLOBALCFG", "END TEST: TestErrorString -- Test Success!");
		} else {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "GLOBALCFG", "END TEST: TestErrorString -- Test Failure!");
		}
    	return result;
    }

}
