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
package com.ibm.ima.plugin.test;

import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaConnectionListener;
import com.ibm.ima.plugin.ImaMessage;
import com.ibm.ima.plugin.ImaPlugin;
import com.ibm.ima.plugin.ImaPluginConfigValidator;
import com.ibm.ima.plugin.ImaPluginException;
import com.ibm.ima.plugin.ImaPluginListener;
import com.ibm.ima.plugin.ImaSubscription;

public class TestPlugin implements ImaPluginListener, ImaPluginConfigValidator {

	public TestPlugin() {
		
	}
	@Override
	public void initialize(ImaPlugin plugin) {
		return;
	}

	@Override
	public boolean onAuthenticate(ImaConnection connect) {
		return false;
	}

	@Override
	public int onProtocolCheck(ImaConnection connect, byte[] data) {
		return 0;
	}

	@Override
	public ImaConnectionListener onConnection(ImaConnection connect, String protocol) {
		return new ImaConnectionListener() {
			
			@Override
			public void onMessage(ImaSubscription sub, ImaMessage msg) {
				return;
			}
			
			@Override
			public boolean onLivenessCheck() {
				return false;
			}
			
			@Override
			public void onHttpData(String op, String path, String query, String content_type, byte[] data) {
				return;
			}
			
			@Override
			public void onGetMessage(Object correlate, int rc, ImaMessage message) {
				return;
			}
			
			@Override
			public int onData(byte[] data, int offset, int length) {
				return 0;
			}
			
			@Override
			public void onConnected(int rc, String reason) {
				return;
			}
			
			@Override
			public void onComplete(Object obj, int rc, String message) {
				return;
			}
			
			@Override
			public void onClose(int rc, String reason) {
				return;
			}
		};
	}

	@Override
	public void onConfigUpdate(String name, Object subname, Object value) {
		return;
	}

	@Override
	public void startMessaging(boolean active) {
		return;
	}

	@Override
	public void terminate(int reason) {
		return;
	}
	@Override
	public void validate(ImaPlugin plugin) throws ImaPluginException {
		System.err.println(plugin.getConfig());
	}

}
