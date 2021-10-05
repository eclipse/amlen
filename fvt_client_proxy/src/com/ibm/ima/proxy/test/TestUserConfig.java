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
 * Test proxy user configuration.
 * 
 *
 */
public class TestUserConfig {
	
	ImaProxyListener proxy;
	String  json;
    ImaJson parseobj = new ImaJson();
    ImaProxyResult ret; 
    Map<String, ?>  map;
    TrWriter _trWriter;
    String logFile;
    LogLevel logLevel;
    
    public String[][] properties = {
    		{"Password",""},
    };
    
	public TestUserConfig(ImaProxyListener listener) {
		// TODO: get log properties from somewhere
		logFile = "./TestUserConfig.log";
		logLevel = LogLevel.valueOf(9);
		_trWriter = new TrWriter(logFile, logLevel);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "Beginning User Configuration Tests");
        proxy = listener;
    }
    
	public boolean runUserTests() {
		boolean result = true;
		if (!TestUserA()) {
			result = false;
		}
		if (!SetandGetUser()) {
			result = false;
		}		
		if (!ListUserTest()) {
			result = false;
		}	
		if (!DeleteUserTest()) {
			result = false;
		}
		return result;
	}
	
	public boolean TestUserA(){
		try {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "BEGIN TestUserA");
			map = proxy.getUser("kwb", "");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "{0}", map);
			
			ret = proxy.getJSON("", "user=,kwb");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "{0}", ret.getStringValue());
			parseobj.parse(ret.getStringValue());
			
		} catch (Exception e) {
			_trWriter.writeException(e);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "USRCFG", "END TestUserA -- Test Failure!");
			return false;
		} 
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "END TestUserA -- Test Success!");
		return true;
		
	}
	
	public boolean SetandGetUser(){
		try {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "BEGIN Test SetandGetUser");
			
			HashMap<String,Object>  value = new HashMap<String,Object>();
            value.put("Password", "TestPassword");
            proxy.setUser("TestUser", "sam", value);
			proxy.setUser("TestUser2", "sam", value); //create additional to expand list
            
			map = proxy.getUser("TestUser", "sam");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "{0}", map);
			
			ret = proxy.getJSON("", "user=sam,TestUser");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "{0}", ret.getStringValue());
			//check validity of parsed json message
            if(parseobj.parse(ret.getStringValue()) == -2){
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "USRCFG", "Invalid JSON message");
            	throw new RuntimeException();
            } else {
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "Valid JSON message");
            }
			
		} catch (Exception e) {
			_trWriter.writeException(e);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "USRCFG", "END SetandGetUser -- Test Failure!");
			return false;
		} 
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "END SetandGetUser -- Test Success!");
		return true;
		
	}
	
	public boolean ListUserTest(){
		try {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "BEGIN ListUserTest");
			String[] userList = proxy.getUserList("*", "sam");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "{0}", TestConfig.printSA(userList));
		} catch (Exception e) {
			_trWriter.writeException(e);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "USRCFG", "END ListUserTest -- Test Failure!");
			return false;
		} 
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "END ListUserTest -- Test Success!");
		return true;
		
	}
	
	public boolean DeleteUserTest(){
		boolean result = true;
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "BEGIN DeleteUserTest");
		
		String[][] users = {
				{"TestUser", "sam", "false"},
				{"TestUser2", "sam", "true"}
		};
		
		for (String[] user : users) {
			try {
				String[] userList = proxy.getUserList("*", "sam");
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "UserList before deletion: {0}", TestConfig.printSA(userList));
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "deleteUser(\"{0}\", \"{1}\", {2})", user[0], user[1], user[2]);
				proxy.deleteUser(user[0], user[1], Boolean.valueOf(user[2]));
				try {
					map = proxy.getUser(user[0], user[1]);
					if (map != null) {
						// why isn't map null?
					}
				} catch (Exception e) {
					_trWriter.writeException(e);
				}
			} catch (Exception e) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "USRCFG", "END DeleteUserTest -- Deletion of user failed!");
				_trWriter.writeException(e);
				result = false;
			} 
		}
			
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "USRCFG", "END DeleteUserTest -- Test Success!");
		return result;
		
	}
}
