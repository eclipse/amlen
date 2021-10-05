/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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

package com.ibm.ima.proxy.impl;

import java.util.Map;

import com.ibm.ima.proxy.ImaDeviceStatus;
import com.ibm.ima.proxy.ImaJson;

public class ImaDeviceStatusImpl implements ImaDeviceStatus {

	String clientID; 
	
	String clientAddress;

	String protocol;

	String action;

	String timestamp;

	boolean secureConnection;
	
	public ImaDeviceStatusImpl(String clientID, String clientAddress, String protocol, boolean secureConnection, String action, String timestamp) {
		this.clientID = clientID;
		this.clientAddress = clientAddress;
		this.protocol = protocol;
		this.action = action;
		this.timestamp = timestamp;
		this.secureConnection=secureConnection;
	}
	
	public ImaDeviceStatusImpl(String deviceStatusJson) {
		/*Todo Parse out the values from JSON*/
		 if (deviceStatusJson != null && deviceStatusJson.length() > 0 && deviceStatusJson.charAt(0)=='{') {
	            ImaJson jparse = new ImaJson();
	            jparse.parse(deviceStatusJson);
	            Map<String,Object> statusObjMap = jparse.toMap(0);
	            //System.out.println("JSON Map: "+ statusObjMap.toString());
	            if(statusObjMap!=null){
	            	this.clientID = (String)statusObjMap.get("ClientID");
	            	this.clientAddress = (String)statusObjMap.get("ClientAddr");
	            	this.protocol = (String)statusObjMap.get("Protocol");
	            	this.action = (String)statusObjMap.get("Action");
	            	this.timestamp = (String)statusObjMap.get("Time");
	            	Boolean secureConnBool = (Boolean)statusObjMap.get("SecureConnection");
	            	this.secureConnection = secureConnBool.booleanValue();
	            }
	            
	            
	     }
		 
	}
	
	
	@Override
	public String getClientID() {
		return clientID;
	}
	
	@Override
	public String getProtocol() {
		return protocol;
	}

	@Override
	public String getAction() {
		return action;
	}

	@Override
	public String getTimestamp() {
		return timestamp;
	}

	@Override
	public String getClientAddress() {
		return clientAddress;
	}

	@Override
	public boolean getSecureConnection() {
		return secureConnection;
	}

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
