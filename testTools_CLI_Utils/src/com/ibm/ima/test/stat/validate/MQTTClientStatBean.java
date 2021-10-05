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

package com.ibm.ima.test.stat.validate;

import org.supercsv.cellprocessor.ift.CellProcessor;



public class MQTTClientStatBean implements Comparable<MQTTClientStatBean> {

	private String clientId = "";
	private String isConnected = "";
	private String lastConnectedTime = "";

/*
	"ClientId",
	"IsConnected",
	"LastConnectedTime"
*/
	public static final CellProcessor[] mqttclientStatProcessor = new CellProcessor[] {
		null,
		null,
		null
	};
	
	public boolean equals(Object expected) {
		
		if (expected == null) {
			System.out.println("Entire stat entry line was empty.");
			return false;
		}
		
		
		if ( this == expected ) return true;
		
		 if ( !(expected instanceof MQTTClientStatBean) ) return false;
		
		 MQTTClientStatBean expectedBean = (MQTTClientStatBean) expected;

		
		boolean areEqual = true;
		
		if (clientId.equals(expectedBean.getClientId()) == false) {
			System.out.println("clientId not equal. Expected <" +expectedBean.getClientId() + "> " + "received <" + clientId +">");
			areEqual = false;
		}
		
		if (isConnected.equals(expectedBean.getIsConnected()) == false) {
			System.out.println("isConnected not equal. Expected <" +expectedBean.getIsConnected() + "> " + "received <" + isConnected +">");
			areEqual = false;
		}
		
		//if (lastConnectedTime.equals(expectedBean.getLastConnectedTime()) == false) {
		//	System.out.println("lastConnectedTime not equal. Expected <" +expectedBean.getLastConnectedTime() + "> " + "received <" + lastConnectedTime +">");
		//	areEqual = false;
		//}
		
		
		return areEqual;
		
	}

	public String getClientId() {
		return clientId;
	}

	public void setClientId(String topicString) {
		this.clientId = clientId;
	}

	public String getIsConnected() {
		return isConnected;
	}

	public void setIsConnected(String isConnected) {
		this.isConnected = isConnected;
	}

	public String getLastConnectedTime() {
		return lastConnectedTime;
	}

	public void setLastConnectedTime(String lastConnectedTime) {
		this.lastConnectedTime = lastConnectedTime;
	}

	@Override
	public int compareTo(MQTTClientStatBean o) {
		return 0;
	}




}
