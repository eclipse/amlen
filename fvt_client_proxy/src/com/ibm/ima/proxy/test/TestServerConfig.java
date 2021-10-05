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
 * Test proxy server configuration.
 * 
 *
 */
public class TestServerConfig {
	
	ImaProxyListener proxy;
	String  json;
    ImaJson parseobj = new ImaJson();
    ImaProxyResult ret; 
    Map<String, ?>  map;
    TrWriter _trWriter;
    String logFile;
    LogLevel logLevel;
    
	public TestServerConfig(ImaProxyListener listener) {
		// TODO: get log properties from somewhere
		logFile = "./TestServerConfig.log";
		logLevel = LogLevel.valueOf(9);
		_trWriter = new TrWriter(logFile, logLevel);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG", "Beginning Server Configuration Tests");
        proxy = listener;
    }
    
	public boolean runServerTests() {
		boolean result = true;
		if (!TestServerA()) {
			result = false;
		}
		if (!TestGetIotServer()) {
			result = false;
		}
		if (!SetandGetServer()) {
			result = false;
		}
		if (!GetServerList()) {
			result = false;
		}
		if (!deleteServerTest()) {
			result = false;
		}
		return result;
	}
	
	public boolean TestServerA(){
		try {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG40", "Server");
			map = proxy.getServer("IoTServer");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG41", "{0}", map);
			
			ret = proxy.getJSON("", "server=IoTServer");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG42", "{0}", ret.getStringValue());
			parseobj.parse(ret.getStringValue());
			
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG53", "getServerList");
			String[] serverList = proxy.getServerList("*");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG44", "{0}", TestConfig.printSA(serverList));
		} catch (Exception e) {
			_trWriter.writeException(e);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "SRVCFG5", "Test Failure!");
			return false;
		} 
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG45", "Test Success!");
		return true;
	}

	public boolean TestGetIotServer(){
		try {

			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG00", "BEGIN TestGetIoTServer");
            
            HashMap<String,Object>  value = new HashMap<String,Object>();
            
            String[] addresses;
            addresses = new String[] {"14.13.12.1","11.12.11.1"};
            value.put("Address", addresses);
            value.put("TLS",  "None");
            value.put("Monitor", true);
            value.put("Port", 16102);
            value.put("MonitorTopic", "IotServerTopic");
            value.put("MonitorRetain", 1);
            value.put("MqttProxyProtocol", false);
            proxy.setServer("IoTServerTest", value);
            map = proxy.getServer("IoTServerTest");
            
            ret = proxy.getJSON("", "server=IoTServerTest");
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG01", "{0}", ret.getStringValue());
            	
            //check validity of parsed json message
            if(parseobj.parse(ret.getStringValue()) == -2){
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "SRVCFG02", "Invalid JSON message");
            	throw new RuntimeException();
            } else {
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG", "Valid JSON message");
            }
            
            if (!TestConfig.compareMap(map, value, _trWriter)) {
            	_trWriter.writeTraceLine(logLevel, "SRVCFG03", "value: "+value);
                _trWriter.writeTraceLine(logLevel, "SRVCFG04", "map: "+map);
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "SRVCFG05", "Map did not match");
            	throw new RuntimeException();
            }
            
		} catch (Exception e) {
			_trWriter.writeException(e);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "SRVCFG06", "END TestGetIoTServer -- Test Failure!");
			return false;
		} 
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG06", "END TestGetIoTServer -- Test Success!");
		return true;
	}
	
	public boolean SetandGetServer(){
		try {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG10", "BEGIN SetandGetServer");
			           
            HashMap<String,Object>  value = new HashMap<String,Object>();
            
            String[] addresses;
            addresses = new String[] {"16.11.22.1", "15.13.21.2"};
            
            value.put("Address", addresses);
            value.put("TLS", "None");
            value.put("Monitor", true);
            value.put("MonitorRetain", 1);
            value.put("Port", 16102);
            proxy.setServer("Server7", value);
            map = proxy.getServer("Server7");
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG11", "{0}", map);
            
            ret = proxy.getJSON("", "server=Server7");
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG12", "{0}", ret.getStringValue());
            	
            //check validity of parsed json message
            if(parseobj.parse(ret.getStringValue()) == -2){
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "SRVCFG13", "Invalid JSON message");
            	throw new RuntimeException();
            } else {
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG", "Valid JSON message");
            }
            
            if (!TestConfig.compareMap(map, value, _trWriter)) {
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "SRVCFG14", "Map did not match");
            	_trWriter.writeTraceLine(logLevel, "SRVCFG15", "value: "+value);
                _trWriter.writeTraceLine(logLevel, "SRVCFG16", "map: "+map);
            	_trWriter.writeTraceLine(logLevel, "SRVCFG17", "Original map keyset=", value.keySet());
            	_trWriter.writeTraceLine(logLevel, "SRVCFG18", "Returned map keyset=", map.keySet());
            	throw new RuntimeException();
            }
            
		} catch (Exception e) {
			_trWriter.writeException(e);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "SRVCFG19", "END SetandGetServer -- Test Failure!");
			return false;
		} 
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG19", "END SetandGetServer -- Test Success!");
		return true;
	}
	
	public boolean GetServerList(){
		try {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG20", "BEGIN getServerList");          
			String[] serverList = proxy.getServerList("*");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG21", "{0}", TestConfig.printSA(serverList));
		   	
            //check validity of parsed json message
            if(parseobj.parse(ret.getStringValue()) == -2){
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "SRVCFG22", "Invalid JSON message");
            	throw new RuntimeException();
            } else {
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG23", "Valid JSON message");
            }
                        
		} catch (Exception e) {
			_trWriter.writeException(e);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "SRVCFG24", "END getServerList -- Test Failure!");
			return false;
		} 
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG24", "END getServerList -- Test Success!");
		return true;
	}
	
	public boolean deleteServerTest() {
		boolean result = true;
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG30", "BEGIN Test DeleteServer");
		
		String[][] servers = {
				{"server1", "false"},
				{"server2", "true"},
				{"server3", "true"}
		};
		
		for (String[] server : servers) { 
			try {
				// Delete with force=false
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG31", "deleteTenant(\"{0}\", {1})", server[0], server[1]);
				proxy.deleteEndpoint(server[0], Boolean.valueOf(server[1]));
				try {
					Map<String,Object> map;
					map = proxy.getServer(server[0]);
					if (map != null) {
						// why isn't map null?
					}
				} catch (Exception e) {
					_trWriter.writeException(e);
				}
			} catch (Exception e) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG32", "deleteTenant(\"{0}\", {1}) failed", server, server[1]);
				_trWriter.writeException(e);
				result = false;
			}
		}
		
		if (result)
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG33", "END Test DeleteServer -- Test Success!");
		else 
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "SRVCFG33", "END Test DeleteServer -- Test Failure!");
		return result;
	}
}
