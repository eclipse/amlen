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

package com.ibm.ima.test.cli.policy.csv;

import org.supercsv.cellprocessor.ift.CellProcessor;

import com.ibm.ima.test.cli.policy.Constants.CLIENTS;
import com.ibm.ima.test.cli.policy.Constants.EPINTERFACE;
import com.ibm.ima.test.cli.policy.Constants.PROTOCAL;



public class PolicyTestBean {


	private String cpUserId = null;
	private String cpGoupId = null;
	private String mp1UserId = null;
	private String mp1GroupId = null;
	private String mp1TopicName = null;
	private String cpClientId = null;
	private String mp1ClientId = null;
	private String cpProtocol = null;
	private String cpClientAddr = null;
	private String mp1ClientAddr = null;
	private String epInterface = null;
	private String mp2TopicName = null;
	private String mp2ClientId = null;
	private String mp2UserId = null;
	private String mp2GroupId = null;
	private String mp2ClientAddr = null;
	private String mp1Protocol = null;
	private String mp2Protocol = null;
	private String last = null;

	private String testCaseIdentifier = null;
	private boolean createUser1 = false;
	private boolean createGroup1 = false;

	private boolean createUser2 = false;
	private boolean createGroup2 = false;
	
	private String gvtString = null;


	public String getCpUserId() {
		if (cpUserId == null) return null;
		return cpUserId.replaceAll("__TESTCASEID__", testCaseIdentifier);
	}


	public void setCpUserId(String cpUserId) {
		if (cpUserId != null && cpUserId.equalsIgnoreCase("null")) {
			this.cpUserId = null;
		} else {
			createUser1 = true;
			this.cpUserId = cpUserId;
		}
	}


	public String getCpGoupId() {
		if (cpGoupId == null) return null;
		return cpGoupId.replaceAll("__TESTCASEID__", testCaseIdentifier);
	}


	public void setCpGoupId(String cpGoupId) {
		if (cpGoupId != null && cpGoupId.equalsIgnoreCase("null")) {
			this.cpGoupId = null;
		} else {
			createGroup1 = true;
			createGroup2 = true;
			createUser1 = true;
			this.cpGoupId = cpGoupId;
		}
	}


	public String getMp1UserId() {
		if (mp1UserId == null) return null;
		return mp1UserId.replaceAll("__TESTCASEID__", testCaseIdentifier);
	}


	public void setMp1UserId(String mp1UserId) {
		if (mp1UserId != null && mp1UserId.equalsIgnoreCase("null")) {
			this.mp1UserId = null;
		} else {
			createUser1 = true;
			this.mp1UserId = mp1UserId;
		}
	}


	public String getMp1GroupId() {
		if (mp1GroupId == null) return null;
		return mp1GroupId.replaceAll("__TESTCASEID__", testCaseIdentifier);
	}


	public void setMp1GroupId(String mp1GroupId) {
		if (mp1GroupId != null && mp1GroupId.equalsIgnoreCase("null")) {
			this.mp1GroupId = null;
		} else {
			createGroup1 = true;
			createUser1 = true;
			this.mp1GroupId = mp1GroupId;
		}
	}


	public String getMp1TopicName() {
		if (mp1TopicName == null) return null;
		return mp1TopicName.replaceAll("__TESTCASEID__", testCaseIdentifier);
	}


	public void setMp1TopicName(String mp1TopicName) {
		if (mp1TopicName != null && mp1TopicName.equalsIgnoreCase("null")) {
			this.mp1TopicName = null;
		} else {
			this.mp1TopicName = mp1TopicName;
		}
	}

	public String getMp2TopicName() {
		if (mp2TopicName == null) return null;
		return mp2TopicName.replaceAll("__TESTCASEID__", testCaseIdentifier);
	}


	public void setMp2TopicName(String mp2TopicName) {
		if (mp2TopicName != null && mp2TopicName.equalsIgnoreCase("null")) {
			this.mp2TopicName = null;
		} else {
			this.mp2TopicName = mp2TopicName;
		}
	}


	public String getCpClientId() {
		if (cpClientId == null) return null;
		return cpClientId.replaceAll("__TESTCASEID__", testCaseIdentifier);
	}


	public void setCpClientId(String cpClientId) {
		if (cpClientId != null && cpClientId.equalsIgnoreCase("null")) {
			this.cpClientId = null;
		} else {
			this.cpClientId = cpClientId;
		}
	}


	public String getMp1ClientId() {
		if (mp1ClientId == null) return null;
		return mp1ClientId.replaceAll("__TESTCASEID__", testCaseIdentifier);
	}


	public void setMp1ClientId(String mp1ClientId) {
		if (mp1ClientId != null && mp1ClientId.equalsIgnoreCase("null")) {
			this.mp1ClientId = null;
		} else {
			this.mp1ClientId = mp1ClientId;
		}
	}


	public String getCpProtocol() {
		PROTOCAL protocolValue = PROTOCAL.fromString(cpProtocol);
		return protocolValue.getText();
	}


	public void setCpProtocol(String cpProtocol) {
		this.cpProtocol = cpProtocol;
	}


	public String getCpClientAddr() {
		CLIENTS clientAddrVal = CLIENTS.fromString(cpClientAddr);
		if (clientAddrVal != null) {
			return clientAddrVal.getText();
		} else {
			return null;
		}
	}
	
	public CLIENTS getCpClientAddrEnum() {
		CLIENTS clientAddrVal = CLIENTS.fromString(cpClientAddr);
		return clientAddrVal;
	}


	public void setCpClientAddr(String cpClientAddr) {
		this.cpClientAddr = cpClientAddr;
	}


	public String getMp1ClientAddr() {
		CLIENTS clientAddrVal = CLIENTS.fromString(mp1ClientAddr);
		if (clientAddrVal != null) {
			return clientAddrVal.getText();
		} else {
			return null;
		}
	}
	
	public CLIENTS getMp1ClientAddrEnum() {
		CLIENTS clientAddrVal = CLIENTS.fromString(mp1ClientAddr);
		return clientAddrVal;
	}


	public void setMp1ClientAddr(String mp1ClientAddr) {
		this.mp1ClientAddr = mp1ClientAddr;
	}


	public String getEpInterface() {
		EPINTERFACE epInterfaceValue = EPINTERFACE.fromString(epInterface);
		return epInterfaceValue.getText();
	}


	public void setEpInterface(String epInterface) {
		this.epInterface = epInterface;
	}


	public String getMp2ClientId() {
		if (mp2ClientId == null) return null;
		return mp2ClientId.replaceAll("__TESTCASEID__", testCaseIdentifier);
	}


	public void setMp2ClientId(String mp2ClientId) {
		if (mp2ClientId != null && mp2ClientId.equalsIgnoreCase("null")) {
			this.mp2ClientId = null;
		} else {
			this.mp2ClientId = mp2ClientId;
		}
	}


	public String getMp2UserId() {
		if (mp2UserId == null) return null;
		return mp2UserId.replaceAll("__TESTCASEID__", testCaseIdentifier);
	}


	public void setMp2UserId(String mp2UserId) {
		if (mp2UserId != null && mp2UserId.equalsIgnoreCase("null")) {
			this.mp2UserId = null;
		} else {
			createUser2 = true;
			this.mp2UserId = mp2UserId;
		}
	}


	public String getMp2GroupId() {
		if (mp2GroupId == null) return null;
		return mp2GroupId.replaceAll("__TESTCASEID__", testCaseIdentifier);
	}


	public void setMp2GroupId(String mp2GroupId) {
		if (mp2GroupId != null && mp2GroupId.equalsIgnoreCase("null")) {
			this.mp2GroupId = null;
		} else {

			createUser2 = true;
			createGroup2 = true;

			this.mp2GroupId = mp2GroupId;
		}
	}


	public String getMp2ClientAddr() {
		CLIENTS clientAddrVal = CLIENTS.fromString(mp2ClientAddr);
		if (clientAddrVal != null) {
			return clientAddrVal.getText();
		} else {
			return null;
		}
	}
	
	public CLIENTS getMp2ClientAddrEnum() {
		CLIENTS clientAddrVal = CLIENTS.fromString(mp2ClientAddr);
		return clientAddrVal;
	}


	public void setMp2ClientAddr(String mp2ClientAddr) {
		this.mp2ClientAddr = mp2ClientAddr;
	}


	public String getMp1Protocol() {
		PROTOCAL protocolValue = PROTOCAL.fromString(mp1Protocol);
		return protocolValue.getText();
	}


	public void setMp1Protocol(String mp1Protocol) {
		this.mp1Protocol = mp1Protocol;
	}


	public String getMp2Protocol() {
		PROTOCAL protocolValue = PROTOCAL.fromString(mp2Protocol);
		return protocolValue.getText();
	}


	public void setMp2Protocol(String mp2Protocol) {
		this.mp2Protocol = mp2Protocol;
	}



	public void setTestCaseIdentifier(String testCaseIdentifier) {
		this.testCaseIdentifier = testCaseIdentifier;
	}


	public void setLast(String last) {
		this.last = last;
	}
	
	public String getLast() {
		return last;
	}


	public boolean createUser1() {

		return createUser1;	
	}

	public boolean createGroup1() {

		if (cpUserId != null && mp2GroupId !=null) {
			if (cpUserId.endsWith("_1")) {
				return true;
			}
		}
		return createGroup1;

	}

	public boolean createGroup2() {

		if (cpUserId != null) {
			if (cpUserId.endsWith("_1")) {
				return false;
			}
		}

		if (cpGoupId != null) {
			if (cpGoupId.endsWith("_1")) {
				return false;
			}
		}



		return createGroup2;

	}

	public boolean createUser2() {

		if (cpUserId != null) {
			if (cpUserId.endsWith("_1")) {
				return false;
			}
		}

		if (cpGoupId != null) {
			if (cpGoupId.endsWith("_1")) {
				return false;
			}
		}

		return createUser2;

	}
	

	public String getGvtString() {
		return gvtString;
	}


	public void setGvtString(String gvtString) {
		this.gvtString = gvtString;
	}



	public static final String[] header = new String[] { 
		"epInterface",
		"cpClientAddr",
		"cpProtocol",
		"cpClientId",
		"cpUserId",
		"cpGoupId",
		"mp1ClientAddr",
		"mp1Protocol",
		"mp1ClientId",
		"mp1UserId",
		"mp1GroupId",
		"mp1TopicName",
		"mp2ClientAddr",
		"mp2Protocol",
		"mp2ClientId",
		"mp2UserId",
		"mp2GroupId",
		"mp2TopicName",
		"last"
	};

	public static final CellProcessor[] policyTestProcessor = new CellProcessor[] {
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
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null
	};



}
