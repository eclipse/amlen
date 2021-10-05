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
package com.ibm.ism.ws.test.jsonmsg;

import com.ibm.ism.mqtt.ImaJson;
import com.ibm.ism.ws.test.MyJsonMsg;

public class JsonMessageConnect extends MyJsonMsg {
	private String ClientID;
	private String User;
	private String Password;
	
	public JsonMessageConnect(String action, ImaJson json, String original) {
		super(action, json, original);
		ClientID	= json.getString("ClientID");
		User		= json.getString("User");
		Password	= json.getString("Password");
	}

	public String getClientID() {
		return ClientID;
	}

	public void setClientID(String clientID) {
		ClientID = clientID;
	}

	public String getUser() {
		return User;
	}

	public void setUser(String user) {
		User = user;
	}

	public String getPassword() {
		return Password;
	}

	public void setPassword(String password) {
		Password = password;
	}

}
