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
package com.ibm.ima.jms.test;

import org.apache.wink.client.ClientConfig;
import org.apache.wink.client.ClientResponse;
import org.apache.wink.client.Resource;
import org.apache.wink.client.RestClient;
import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

/**
 * 
 * 
 * structureID - Store the response object in _dataRepository with this name
 * server - IP:PORT of messagesight container
 * path - /ima/v1/configuration etc.
 * action - POST, GET, etc.
 * payload - payload for POSTs
 * 
 * printResult - true|false to print the response
 * expectedMessageCode - check the message id in the response from messagesight (default "" == don't check)
 * expectedStatusCode - check the HTTP status code (default=-1 == don't check)
 * 
 * TODO: handle file PUT
 * TODO: Handle message code validation
 * TODO: HTTP Auth?
 * 
 * Error codes:
 * ISMTEST3310 - Env variables not set.
 * ISMTEST3311 - Unknown action
 */
public class RestAPIAction extends ApiCallAction {
	// Expected _server value "``A1_HOST``:``A1_PORT``
	private final String _server;
	// Expected path value "/ima/v1/configuration"
	private final String _path;
	// Expected action value "POST"
	private final String _action;
	// Expected payload value "{"ObjectType":{"ObjectName":{"Prop1":"Value1"}}}"
	private final String _payload;
	
	private final boolean _printResult;
	private final String _expectedMessageCode;
	private final int _expectedStatusCode;
	private final String _structureID;
	
	public RestAPIAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		
		_structureID = _actionParams.getProperty("structureID", "");
		_server = _actionParams.getProperty("server");
		if (_server == null || _server.equals("")) {
			throw new RuntimeException("server is not defined for "
					+ this.toString());
		}
		
		_path = _actionParams.getProperty("path");
		if (_path == null || _path.equals("")) {
			throw new RuntimeException("path is not defined for "
					+ this.toString());
		}
		
		_action = _actionParams.getProperty("action");
		if (_action == null || _action.equals("")) {
			throw new RuntimeException("action is not defined for "
					+ this.toString());
		}
		
		_payload = _actionParams.getProperty("payload");
		if (_action.equals("POST") && _payload == null) {
			throw new RuntimeException("action is not defined for "
					+ this.toString());
		}
		
		_expectedStatusCode = Integer.parseInt(_actionParams.getProperty("expectedStatusCode", "-1"));
		_expectedMessageCode = _actionParams.getProperty("expectedMessageCode", "");
		_printResult = Boolean.parseBoolean(_actionParams.getProperty("printResult", "true"));
	}
	
	protected boolean invokeApi() {
		String server = null;
		String payload = null;
		String entity = "";
		String message = "";
		String messageCode = "";
		int statusCode = -1;
		try {
			server = EnvVars.replace(_server);
		} catch (Exception e) {
			_trWriter.writeException(e);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST3310",
					"Environment variable replacement in server failed");
			return false;
		}
		if (_payload != null) {
			try {
				payload = EnvVars.replace(_payload);
			} catch (Exception e) {
				_trWriter.writeException(e);
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST3310",
						"Environment variable replacement in payload failed");
				return false;
			}
		}
		
		// Configure client...
		ClientConfig clientConfig = new ClientConfig();
		
		// Use Basic Authentication
		//BasicAuthSecurityHandler basicAuthSecHandler = new BasicAuthSecurityHandler(); 
		//basicAuthSecHandler.setUserName(_userID); basicAuthSecHandler.setPassword(_password);
		//clientConfig.handlers(basicAuthSecHandler);
				
		// Define resource
		RestClient client = new RestClient(clientConfig);
		Resource resource = client.resource("http://"+server+_path);
		ClientResponse response = null;
		if (_action.equals("POST")) {
			response = resource.contentType("application/json").accept("*/*").post(payload);
		} else if (_action.equals("GET")) {
			response = resource.contentType("application/json").accept("*/*").get();
		} else if (_action.equals("DELETE")) {
			response = resource.contentType("application/json").accept("*/*").delete();
		} else if (_action.equals("PUT")) {
			// TODO:
			response = resource.contentType("application/json").accept("*/*").put(payload);
		} else {
			// Unknown action
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST3311",
					"{0}: Unknown Action type specified: {1}", _id, _action);
			return false;
			//throw new JmsTestException("ISMTEST3311", 
			//		"Unknown Action type specified: " + _action);
		}

		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST3302", 
				"{0}: {1} http://{2}{3} PAYLOAD={4}", _id, _action, server, _path, _payload);
		
		statusCode = response.getStatusCode();
		message = response.getMessage();
		entity = response.getEntity(String.class);
		if (_printResult) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST3301", "{0}: RestAPI response.statusCode: {1}", _id, statusCode);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST3301", "{0}: RestAPI response.message: {1}", _id, message);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST3301", "{0}: RestAPI response.entity: {1}", _id, entity);
		}
		
		// Check return code .. statusCode or message code?
		if (_expectedStatusCode != -1) {
			if (response.getStatusCode() != _expectedStatusCode) {
				return false;
			}
		}
		if (!_expectedMessageCode.equals("")) {
			
		}
		
		if (_structureID != null && !_structureID.equals("")) {
			_dataRepository.storeVar(_structureID, response);
		}
		
		return true;
	}
}
