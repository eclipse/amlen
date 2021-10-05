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

import java.io.IOException;

import org.apache.wink.client.ClientConfig;
import org.apache.wink.client.ClientResponse;
import org.apache.wink.client.Resource;
import org.apache.wink.client.RestClient;
import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;
import com.ibm.json.java.JSONObject;

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
public class GetServerUIDAction extends ApiCallAction {
	// Expected _server value "``A1_HOST``:``A1_PORT``
	private final String _server;
	
	private final boolean _printResult;
	private final String _structureID;
	
	public GetServerUIDAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		
		_structureID = _actionParams.getProperty("structureID", "");
		_server = _actionParams.getProperty("server");
		if (_server == null || _server.equals("")) {
			throw new RuntimeException("server is not defined for "
					+ this.toString());
		}

		_printResult = Boolean.parseBoolean(_actionParams.getProperty("printResult", "true"));
	}
	
	protected boolean invokeApi() {
		String server = null;
		String entity = "";
		String message = "";
		String messageCode = "";
		String serverUID = "";
		int statusCode = -1;
		try {
			server = EnvVars.replace(_server);
		} catch (Exception e) {
			_trWriter.writeException(e);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST3310",
					"Environment variable replacement in server failed");
			return false;
		}
		
		// Configure client...
		ClientConfig clientConfig = new ClientConfig();
		
		// Use Basic Authentication
		//BasicAuthSecurityHandler basicAuthSecHandler = new BasicAuthSecurityHandler(); 
		//basicAuthSecHandler.setUserName(_userID); basicAuthSecHandler.setPassword(_password);
		//clientConfig.handlers(basicAuthSecHandler);
				
		// Define resource
		RestClient client = new RestClient(clientConfig);
		Resource resource = client.resource("http://"+server+"/ima/v1/configuration/ServerUID");
		ClientResponse response = null;

		response = resource.contentType("application/json").accept("*/*").get();


		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST3302", 
				"{0}: GET http://{1}/ima/v1/configuration/ServerUID ", _id, server);
		
		statusCode = response.getStatusCode();
		message = response.getMessage();
		entity = response.getEntity(String.class);
		if (_printResult) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST3301", "{0}: RestAPI response.statusCode: {1}", _id, statusCode);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST3301", "{0}: RestAPI response.message: {1}", _id, message);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST3301", "{0}: RestAPI response.entity: {1}", _id, entity);
		}
		
		if (response.getStatusCode() != 200) {
			return false;
		}
		
		try {
			JSONObject json = JSONObject.parse(entity);
			serverUID = (String) json.get("ServerUID");
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		if (_structureID != null && !_structureID.equals("")) {
			_dataRepository.storeVar(_structureID, serverUID);
		}
		
		return true;
	}
}
