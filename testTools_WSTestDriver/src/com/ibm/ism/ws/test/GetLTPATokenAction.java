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
package com.ibm.ism.ws.test;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLSession;

import org.apache.wink.client.ClientConfig;
import org.apache.wink.client.ClientResponse;
import org.apache.wink.client.Resource;
import org.apache.wink.client.RestClient;
import org.apache.wink.client.handlers.BasicAuthSecurityHandler;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class GetLTPATokenAction extends ApiCallAction {
	static {
	    HttpsURLConnection.setDefaultHostnameVerifier(new HostnameVerifier() {
			
			public boolean verify(String hostname, SSLSession session) {
				// TODO Auto-generated method stub
				return true;
			}
		});
	        
	}

	private final 	String 	_WebServer;
	private final	String	_userID;
	private final	String	_password;
	private final	String	_LTPAToken;
	private final	String	_keyStore;
	private final	String	_keyStorePassword;
	private final	String	_keyStoreType;
	private final	String	_trustStore;
	private final	String	_trustStorePassword;
	private final	String	_trustStoreType;

	public GetLTPATokenAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_WebServer = _actionParams.getProperty("WebServer");
		if (_WebServer == null) {
			throw new RuntimeException("WebServer is not defined for "
					+ this.toString());
		}
		_userID = _actionParams.getProperty("userID");
		if (_userID == null) {
			throw new RuntimeException("userID is not defined for "
					+ this.toString());
		}
		_password = _actionParams.getProperty("password");
		if (_password == null) {
			throw new RuntimeException("password is not defined for "
					+ this.toString());
		}
		_LTPAToken = _actionParams.getProperty("LTPAToken");
		_keyStore = _actionParams.getProperty("keyStore");
		_keyStorePassword = _actionParams.getProperty("keyStorePassword");
		_keyStoreType = _actionParams.getProperty("keyStoreType");
		_trustStore = _actionParams.getProperty("trustStore");
		_trustStorePassword = _actionParams.getProperty("trustStorePassword");
		_trustStoreType = _actionParams.getProperty("trustStoreType");
	}
	
	protected boolean invokeApi() throws IsmTestException {
		// See if we have keyStore and keyStorePassword to set
        if (_keyStore != null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "ISMTEST"+(Constants.GETLTPATOKEN+1),
                        "Action {0}: Setting keyStore location.", _id);
            System.setProperty("javax.net.ssl.keyStore", _keyStore);
        }

        if (_keyStorePassword != null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "ISMTEST"+(Constants.GETLTPATOKEN+2),
                        "Action {0}: Setting keyStorePassword.", _id);
            System.setProperty("javax.net.ssl.keyStorePassword", _keyStorePassword);
        }
		// Same for trustStore and trustStorePassword
        if (_trustStore != null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "ISMTEST"+(Constants.GETLTPATOKEN+3),
                        "Action {0}: Setting trustStore location.", _id);
            System.setProperty("javax.net.ssl.trustStore", _trustStore);
        }

        if (_trustStorePassword != null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "ISMTEST"+(Constants.GETLTPATOKEN+4),
                        "Action {0}: Setting trustStorePassword.", _id);
            System.setProperty("javax.net.ssl.trustStorePassword", _trustStorePassword);
        }
        if (null != _keyStoreType) System.setProperty("javax.net.ssl.keyStoreType", _keyStoreType);
        if (null != _trustStoreType) System.setProperty("javax.net.ssl.trustStoreType", _trustStoreType);

		// Configure client...
		ClientConfig clientConfig = new ClientConfig();
		
		// Use Basic Authentication
		BasicAuthSecurityHandler basicAuthSecHandler = new BasicAuthSecurityHandler(); 
		basicAuthSecHandler.setUserName(_userID); basicAuthSecHandler.setPassword(_password);
		clientConfig.handlers(basicAuthSecHandler);
		
		// Define resource
		RestClient client = new RestClient(clientConfig);
		Resource resource = client.resource("https://"+_WebServer+"/ltpawebsample/rest/ltpa/token");
		ClientResponse response = resource.contentType("application/json").accept("*/*").get();
		String LTPAToken = response.getEntity(String.class);
		_dataRepository.storeVar(_LTPAToken, LTPAToken);
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.GETLTPATOKEN),
				"Action {0}: Got LTPA Token {1}.",
				_id, LTPAToken);

		// If we set key and trust Store and Password then we now need to unset
        if (_keyStore != null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "ISMTEST"+(Constants.GETLTPATOKEN+5),
                        "Action {0}: Clearing keyStore location.", _id);
            System.clearProperty("javax.net.ssl.keyStore");
        }

        if (_keyStorePassword != null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "ISMTEST"+(Constants.GETLTPATOKEN+6),
                        "Action {0}: Clearing keyStorePassword.", _id);
            System.clearProperty("javax.net.ssl.keyStorePassword");
        }
        if (_trustStore != null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "ISMTEST"+(Constants.GETLTPATOKEN+7),
                        "Action {0}: Clearing trustStore location.", _id);
            System.clearProperty("javax.net.ssl.trustStore");
        }

        if (_trustStorePassword != null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "ISMTEST"+(Constants.GETLTPATOKEN+8),
                        "Action {0}: Clearing trustStorePassword.", _id);
            System.clearProperty("javax.net.ssl.trustStorePassword");
        }
        if (null != _keyStoreType) System.clearProperty("javax.net.ssl.keyStoreType");
        if (null != _trustStoreType) System.clearProperty("javax.net.ssl.trustStoreType");
		return true;
	}
	
}
