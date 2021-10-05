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

import java.util.Properties;

import org.w3c.dom.Element;

public class CreateSSLPropertiesAction extends ApiCallAction {
	private final String _props;
	private final String _protocol;
	private final String _contextProvider;
	private final String _keyStore;
	private final String _keyStorePassword;
	private final String _keyStoreType;
	private final String _keyStoreProvider;
	private final String _trustStore;
	private final String _trustStorePassword;
	private final String _trustStoreType;
	private final String _trustStoreProvider;
	private final String _enabledCipherSuites;
	private final String _keyManager;
	private final String _trustManager;
	private final String _sniServer;

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CreateSSLPropertiesAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_props = _actionParams.getProperty("props_id");
		if (_props == null) {
			throw new RuntimeException("props_id is not defined for "
					+ this.toString());
		}
		_protocol = _actionParams.getProperty("com.ibm.ssl.protocol");
		_contextProvider = _actionParams.getProperty("com.ibm.ssl.contextProvider");
		_keyStore = _actionParams.getProperty("com.ibm.ssl.keyStore");
		_keyStorePassword = _actionParams.getProperty("com.ibm.ssl.keyStorePassword");
		_keyStoreType = _actionParams.getProperty("com.ibm.ssl.keyStoreType");
		_keyStoreProvider = _actionParams.getProperty("com.ibm.ssl.keyStoreProvider");
		_trustStore = _actionParams.getProperty("com.ibm.ssl.trustStore");
		_trustStorePassword = _actionParams.getProperty("com.ibm.ssl.trustStorePassword");
		_trustStoreType = _actionParams.getProperty("com.ibm.ssl.trustStoreType");
		_trustStoreProvider = _actionParams.getProperty("com.ibm.ssl.trustStoreProvider");
		_enabledCipherSuites = _actionParams.getProperty("com.ibm.ssl.enabledCipherSuites");
		_keyManager = _actionParams.getProperty("com.ibm.ssl.keyManager");
		_trustManager = _actionParams.getProperty("com.ibm.ssl.trustManager");
		_sniServer = _actionParams.getProperty("com.ibm.ssl.servername");
	}

	protected boolean invokeApi() throws IsmTestException {
		Properties props = new Properties();
		if (null != _protocol) {
			props.setProperty("com.ibm.ssl.protocol", _protocol);
		}
		if (null != _contextProvider) {
			props.setProperty("com.ibm.ssl.contextProvider", _contextProvider);
		}
		if (null != _keyStore) {
			props.setProperty("com.ibm.ssl.keyStore", _keyStore);
		}
		if (null != _keyStorePassword) {
			props.setProperty("com.ibm.ssl.keyStorePassword", _keyStorePassword);
		}
		if (null != _keyStoreType) {
			props.setProperty("com.ibm.ssl.keyStoreType", _keyStoreType);
		}
		if (null != _keyStoreProvider) {
			props.setProperty("com.ibm.ssl.keyStoreProvider", _keyStoreProvider);
		}
		if (null != _trustStore) {
			props.setProperty("com.ibm.ssl.trustStore", _trustStore);
		}
		if (null != _trustStorePassword) {
			props.setProperty("com.ibm.ssl.trustStorePassword", _trustStorePassword);
		}
		if (null != _trustStoreType) {
			props.setProperty("com.ibm.ssl.trustStoreType", _trustStoreType);
		}
		if (null != _trustStoreProvider) {
			props.setProperty("com.ibm.ssl.trustStoreProvider", _trustStoreProvider);
		}
		if (null != _enabledCipherSuites) {
			props.setProperty("com.ibm.ssl.enabledCipherSuites", _enabledCipherSuites);
		}
		if (null != _keyManager) {
			props.setProperty("com.ibm.ssl.keyManager", _keyManager);
		}
		if (null != _trustManager) {
			props.setProperty("com.ibm.ssl.trustManager", _trustManager);
		}
		if (null != _sniServer) {
			props.setProperty("com.ibm.ssl.servername", _sniServer);
		}
		_dataRepository.storeVar(_props, props);
		return true;
	}

}
