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



public class ConnectionStatBean implements Comparable<ConnectionStatBean> {

	private String name = "";
	private String protocol = "";
	private String clientAddr = "";
	private String userId = "";
	private String endpoint = "";
	private String port = "";
	private String connectionTime = "";
	private String duration = "";
	private String readBytes = "";
	private String readMsg = "";
	private String writeBytes = "";
	private String writeMsg = "";
	
/*
	"Name",
	"Protocol",
	"ClientAddr",
	"UserId",
	"Endpoint",
	"Port",
	"ConnectionTime",
	"Duration",
	"ReadBytes",
	"ReadMsg",
	"WriteBytes",
	"WriteMsg"
*/
	public static final CellProcessor[] connectionStatProcessor = new CellProcessor[] {
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
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
		
		 if ( !(expected instanceof ConnectionStatBean) ) return false;
		
		 ConnectionStatBean expectedBean = (ConnectionStatBean) expected;

		
		boolean areEqual = true;
		
		if (name.equals(expectedBean.getName()) == false) {
			System.out.println("name not equal. Expected <" +expectedBean.getName() + "> " + "received <" + name +">");
			areEqual = false;
		}
		
		if (protocol.equals(expectedBean.getProtocol()) == false) {
			System.out.println("protocol not equal. Expected <" +expectedBean.getProtocol() + "> " + "received <" + protocol +">");
			areEqual = false;
		}
		
		// TODO: Check of client IP address which is different for each test machine
		//if (clientAddr.equals(expectedBean.getClientAddr()) == false) {
		//	System.out.println("clientAddr not equal. Expected <" +expectedBean.getClientAddr() + "> " + "received <" + clientAddr +">");
		//	areEqual = false;
		//}
		
		if (userId.equals(expectedBean.getUserId()) == false) {
			System.out.println("userId not equal. Expected <" +expectedBean.getUserId() + "> " + "received <" + userId +">");
			areEqual = false;
		}
		
		if (endpoint.equals(expectedBean.getEndpoint()) == false) {
			System.out.println("endpoint not equal. Expected <" +expectedBean.getEndpoint() + "> " + "received <" + endpoint +">");
			areEqual = false;
		}
		
		if (port.equals(expectedBean.getPort()) == false) {
			System.out.println("port not equal. Expected <" +expectedBean.getPort() + "> " + "received <" + port +">");
			areEqual = false;
		}
		
		// All of the remaining values are no deterministic
		//if (connectionTime.equals(expectedBean.getConnectionTime()) == false) {
		//	System.out.println("connectionTime not equal. Expected <" +expectedBean.getConnectionTime() + "> " + "received <" + connectionTime +">");
		//	areEqual = false;
		//}
		
		//if (duration.equals(expectedBean.getDuration()) == false) {
		//	System.out.println("duration not equal. Expected <" +expectedBean.getDuration() + "> " + "received <" + duration +">");
		//	areEqual = false;
		//}

		//if (readBytes.equals(expectedBean.getReadBytes()) == false) {
		//	System.out.println("readBytes not equal. Expected <" +expectedBean.getReadBytes() + "> " + "received <" + readBytes +">");
		//	areEqual = false;
		//}
		
		//if (readMsg.equals(expectedBean.getReadMsg()) == false) {
		//	System.out.println("readMsg not equal. Expected <" +expectedBean.getReadMsg() + "> " + "received <" + readMsg +">");
		//	areEqual = false;
		//}
		
		//if (writeBytes.equals(expectedBean.getWriteBytes()) == false) {
		//	System.out.println("writeBytes not equal. Expected <" +expectedBean.getWriteBytes() + "> " + "received <" + writeBytes +">");
		//	areEqual = false;
		//}
		
		//if (writeMsg.equals(expectedBean.getWriteMsg()) == false) {
		//	System.out.println("writeMsg not equal. Expected <" +expectedBean.getWriteMsg() + "> " + "received <" + writeMsg +">");
		//	areEqual = false;
		//}

		return areEqual;
		
	}

	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;	
		}

	public String getProtocol() {
		return protocol;
	}

	public void setProtocol(String protocol) {
		this.protocol = protocol;
	}

	public String getClientAddr() {
		return clientAddr;
	}

	public void setClientAddr(String clientAddr) {
		this.clientAddr = clientAddr;
	}
	
	public String getUserId() {
		return userId;
	}

	public void setUserId(String userId) {
		this.userId = userId;
	}

	public String getEndpoint() {
		return endpoint;
	}

	public void setEndpoint(String endpoint) {
		this.endpoint = endpoint;
	}
	
	public String getPort() {
		return port;
	}

	public void setPort(String port) {
		this.port = port;
	}
	
	public String getConnectionTime() {
		return connectionTime;
	}

	public void setConnectionTime(String connectionTime) {
		this.connectionTime = connectionTime;
	}
	
	public String getDuration() {
		return duration;
	}

	public void setDuration(String duration) {
		this.duration = duration;
	}
	
	public String getReadBytes() {
		return readBytes;
	}

	public void setReadBytes(String readBytes) {
		this.readBytes = readBytes;
	}
	
	public String getReadMsg() {
		return readMsg;
	}
	
	public void setReadMsg(String readMsg) {
		this.readMsg = readMsg;
	}
	
	public String getWriteBytes() {
		return writeBytes;
	}

	public void setWriteBytes(String writeBytes) {
		this.writeBytes = writeBytes;
	}
	
	public String getWriteMsg() {
		return writeMsg;
	}

	public void setWriteMsg(String writeMsg) {
		this.writeMsg = writeMsg;
	}
	
	
	@Override
	public int compareTo(ConnectionStatBean o) {
		return 0;
	}




}
