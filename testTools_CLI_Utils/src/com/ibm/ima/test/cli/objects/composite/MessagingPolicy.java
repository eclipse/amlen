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

package com.ibm.ima.test.cli.objects.composite;

import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;

import com.ibm.ima.test.cli.policy.Constants.DESTINATION_TYPE;
import com.ibm.ima.test.cli.policy.Constants.MESSAGING_ACTION;

public class MessagingPolicy {

	private static String SEP =  "\n";

	private String name;
	private String description;
	private String clientID;
	private String clientAddress;
	private String userId;
	private String group;
	private String commonNames;
	private String destination;
	private String protocol;
	private MESSAGING_ACTION actions;
	private DESTINATION_TYPE destinationType;
	private int maxMessages = -1;


	public MessagingPolicy(String name, String destination, DESTINATION_TYPE destinationType, MESSAGING_ACTION actions) {

		this.name = name;
		this.actions = actions;
		this.destination = destination;
		this.destinationType = destinationType;

	}


	public String getName() { return name; }

	/**
	 * Return the cli set command:
	 * set MessagingPolicy "Name=PubSubMsgPol19540" 
	 *   "PolicyType=Messaging" 
	 *   "Description=Pub/Sub for ENDP19540" 
	 *   "Topic=ENDP19540_Topi*" 
	 *   "Protocol=JMS,MQTT" 
	 *   "MaxMessages=5000" 
	 *   "ID=GRP19540_1,USR19540_1" 
	 *   "ActionList=Publish,Subscribe" 
	 *   "ClientID=CID19540_1" 
	 *   "ClientAddress=${M1_IPv4_1}"
	 * @return
	 */
	public String getCliSetCommand() {
		String cmd = "create MessagingPolicy \"Name=" + name + "\""
				+ " \"Destination=" + destination + "\""
				+ " \"DestinationType=" + destinationType.getText() + "\""
				+ " \"ActionList=" + actions.getText() + "\"";


		// Protocol is optional
		if (protocol != null) {
			cmd += " \"Protocol=" + protocol + "\"";
		}
		if (description != null) {
			cmd += " \"Description=" + description + "\"";
		}
		if (clientID != null) {
			cmd += " \"ClientID=" + clientID + "\"";
		}

		if (maxMessages >= 0) {
			cmd += " \"MaxMessages=" + maxMessages + "\"";
		}
		if (clientAddress != null) {
			cmd += " \"ClientAddress=" + clientAddress + "\"";
		}
		if (userId != null) {
			cmd += " \"UserID=" + userId + "\"";
		}
		if (group != null) {
			cmd += " \"GroupID=" + group + "\"";
		}
		if (commonNames != null) {
			cmd += " \"CommonNames=" + commonNames + "\"";
		}

		return cmd;
	}

	public String getCliUpdateCommand() {

		String cmd = "update MessagingPolicy \"Name=" + name + "\""
				+ " \"Protocol=" + protocol + "\"";

		return cmd;

	}

	public String getCliDeleteCommand() {
		return "delete MessagingPolicy \"Name=" + name + "\"";
	}


	public void writeMessagingPoliciesSet(String testSuiteName, String testCaseIdentifier, OutputStream zOut) throws IOException {
		
		String testCaseName = testSuiteName + "_" + testCaseIdentifier;
		String mPolicyCommand = testCaseName + " 0 " +  getCliSetCommand() + SEP;
		OutputStreamWriter out = new OutputStreamWriter(zOut,"utf-8");

		try {
			byte[] utf8Bytes = mPolicyCommand.getBytes("UTF8");
			mPolicyCommand = new String(utf8Bytes, "UTF8");
		} catch (Exception e) {
			e.printStackTrace();
		}

		out.write(mPolicyCommand);
		out.flush();

	}

	public  void writeMessagingPoliciesUpdateSet(String testSuiteName, String testCaseIdentifier, OutputStream zOut) throws IOException {

		String testCaseName = testSuiteName + "_" + testCaseIdentifier;
		String mPolicyCommand = testCaseName + " 0 " + getCliUpdateCommand() + SEP;
		
		OutputStreamWriter out = new OutputStreamWriter(zOut,"utf-8");

		try {
			byte[] utf8Bytes = mPolicyCommand.getBytes("UTF8");
			mPolicyCommand = new String(utf8Bytes, "UTF8");
		} catch (Exception e) {
			e.printStackTrace();
		}

		out.write(mPolicyCommand);
		out.flush();
		
	}

	public void writeMessagingPoliciesDelete(String testSuiteName, String testCaseIdentifier, OutputStream zOut) throws IOException {
		
		String testCaseName = testSuiteName + "_" + testCaseIdentifier;
		String mPolicyCommand = testCaseName + "_CLEAN 0 " +  getCliDeleteCommand() + SEP;
		OutputStreamWriter out = new OutputStreamWriter(zOut,"utf-8");

		try {
			byte[] utf8Bytes = mPolicyCommand.getBytes("UTF8");
			mPolicyCommand = new String(utf8Bytes, "UTF8");
		} catch (Exception e) {
			e.printStackTrace();
		}

		out.write(mPolicyCommand);
		out.flush();
		
	}


	public void setDescription(String description) {
		this.description = description;
	}

	public void setClientID(String clientID) {
		this.clientID = clientID;
	}

	public void setClientAddress(String clientAddress) {
		this.clientAddress = clientAddress;
	}

	public void setId(String id) {
		this.userId = id;
	}

	public void setGroup(String group) {
		this.group = group;
	}

	public void setCommonNames(String commonNames) {
		this.commonNames = commonNames;
	}

	public void setProtocol(String protocol) {
		this.protocol = protocol;
	}

}
