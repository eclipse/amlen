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

public class JsonMessageAck extends MyJsonMsg {
	String ID;

	public JsonMessageAck(String action, ImaJson json, String original) {
		super(action, json, original);
		ID = json.getString("ID");
	}

	public String getID() {
		return ID;
	}

	public void setID(String iD) {
		ID = iD;
	}


}
