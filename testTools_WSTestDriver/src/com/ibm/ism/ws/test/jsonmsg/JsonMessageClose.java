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

public class JsonMessageClose extends MyJsonMsg {
	private int		Code;
	private String	Reason;

	public JsonMessageClose(String action, ImaJson json, String original) {
		super(action, json, original);
		Code	= json.getInt("Code", -9999);
		Reason	= json.getString("Reason");
	}

	public int getCode() {
		return Code;
	}

	public void setCode(int code) {
		Code = code;
	}

	public String getReason() {
		return Reason;
	}

	public void setReason(String reason) {
		Reason = reason;
	}

}
