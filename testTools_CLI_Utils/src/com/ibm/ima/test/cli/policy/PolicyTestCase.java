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

package com.ibm.ima.test.cli.policy;

import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

import com.ibm.ima.test.cli.objects.composite.ConnectionPolicy;
import com.ibm.ima.test.cli.objects.composite.Endpoint;
import com.ibm.ima.test.cli.objects.composite.MessageHub;
import com.ibm.ima.test.cli.objects.composite.MessagingPolicy;
import com.ibm.ima.test.cli.policy.Constants.CLIENTS;
import com.ibm.ima.test.cli.policy.Constants.DESTINATION_TYPE;
import com.ibm.ima.test.cli.policy.Constants.MESSAGING_ACTION;
import com.ibm.ima.test.cli.policy.Constants.PROTOCAL;
import com.ibm.ima.test.cli.policy.Constants.TESTCASE_TYPE;
import com.ibm.ima.test.cli.policy.csv.PolicyTestBean;
import com.ibm.ima.test.cli.testplan.xml.IPolicyValidator;
import com.ibm.ima.test.cli.testplan.xml.JMSReceiver;
import com.ibm.ima.test.cli.testplan.xml.JMSTransmitter;
import com.ibm.ima.test.cli.testplan.xml.MQTTReceiver;
import com.ibm.ima.test.cli.testplan.xml.MQTTTransmitter;

public class PolicyTestCase {

	private static String SEP =  "\n";

	private PolicyTestBean policyTestBean = null;
	private String testCaseIdentifier = null;
	private String testSuiteName = null;  
	private TESTCASE_TYPE type = null;
	private boolean doUpdates = false;

	private int port = 0;

	private MessageHub hub = null;

	private String mPolicyPubName = null;
	private String mPolicySubName = null;
	private String mPolicyPubSubName = null;

	private List<MessagingGroup> groups = new ArrayList<MessagingGroup>();
	private List<MessagingUser> users = new ArrayList<MessagingUser>();

	private String user1String = null;
	private String user2String = null;

	private IPolicyValidator publishValidator = null;
	private IPolicyValidator subscribeValidator = null;
	private List<IPolicyValidator> xmlFiles = new ArrayList<IPolicyValidator>();

	private String realTopicName = null;

	private String subClientMachine = "m1";
	private String pubClientMachine = "m1";



	public PolicyTestCase(PolicyTestBean policyTestBean, String testSuiteName,  String testCaseIdentifier, int port,  TESTCASE_TYPE type) {

		this.policyTestBean = policyTestBean;
		this.testCaseIdentifier = testCaseIdentifier;
		this.testSuiteName = testSuiteName;
		this.port = port;
		this.type = type;

		mPolicyPubName = "PubMsgPol" + testCaseIdentifier;
		mPolicySubName = "SubMsgPol" + testCaseIdentifier;
		mPolicyPubSubName = "PubSubMsgPol" + testCaseIdentifier;
		if (type.equals(TESTCASE_TYPE.GVT_TOPIC)) {
			realTopicName = policyTestBean.getGvtString();
		} else {
			realTopicName = "ENDP" + testCaseIdentifier + "_Topic";
		}

	}

	public  void buildTestCase()  {

		String hubName = null;
		String hubDesc = null;
		String endpointName = null;
		String endpointDesc = null;
		String connectionPolicyName = null;
		if (type.equals(TESTCASE_TYPE.GVT_TOPIC)) {
			hubName = "HUB" + policyTestBean.getGvtString() + testCaseIdentifier;
			hubDesc = "Message hub for test " + policyTestBean.getGvtString() + testCaseIdentifier;
			endpointName = "ENDP" + policyTestBean.getGvtString() + testCaseIdentifier;
			endpointDesc = "Endpoint for test " + policyTestBean.getGvtString() + testCaseIdentifier;
			connectionPolicyName = "ConnPol" + policyTestBean.getGvtString() + testCaseIdentifier;
		} else {
			hubName = "HUB" + testCaseIdentifier;
			hubDesc = "Message hub for test " + testCaseIdentifier;
			endpointName = "ENDP" + testCaseIdentifier;
			endpointDesc = "Endpoint for test " + testCaseIdentifier;
			connectionPolicyName = "ConnPol" + testCaseIdentifier;
		}
	


		hub = new MessageHub(hubName, hubDesc);
		String protocol = policyTestBean.getCpProtocol();
		Endpoint endpoint = new Endpoint(testSuiteName + "_" + testCaseIdentifier, endpointName, "" + port, protocol, null, null);
		endpoint.setDescription(endpointDesc);
		endpoint.setHubName(hubName);
		endpoint.setHub(hub);
		endpoint.setNetworkInterface(policyTestBean.getEpInterface());
		hub.addEndpoint(endpoint);

		ConnectionPolicy cPolicy = new ConnectionPolicy(connectionPolicyName, endpoint, policyTestBean.getCpProtocol(), policyTestBean.getCpClientAddr());
		endpoint.addConnPolicy(cPolicy);
		cPolicy.setClientId(policyTestBean.getCpClientId());
		cPolicy.setGroup(policyTestBean.getCpGoupId());
		cPolicy.setUserId(policyTestBean.getCpUserId());

		MessagingPolicy mp1 = getMessagePolicy1();

		endpoint.addMsgPolicy(mp1);
		if (type.equals(TESTCASE_TYPE.VALID_SEPARATION)) {
			MessagingPolicy mp2 = getMessagePolicy2();
			endpoint.addMsgPolicy(mp2);
		}

		groupsAndUsers();

		if (type.equals(TESTCASE_TYPE.VALID_SEPARATION)) {
			getValidPublishXMLWithSeparation();
		} else if (type.equals(TESTCASE_TYPE.VALID_NO_SEPARATION) || type.equals(TESTCASE_TYPE.GVT_TOPIC)) {
			getValidPublishXMLNoSeparation();
		}
		getSubscribeXml();

	}

	private MessagingPolicy getMessagePolicy1() {

		MessagingPolicy mp1 = null;

		if (type.equals(TESTCASE_TYPE.VALID_SEPARATION)) {
			mp1 = new MessagingPolicy(mPolicySubName,  policyTestBean.getMp1TopicName(), DESTINATION_TYPE.TOPIC, MESSAGING_ACTION.SUB);
		} else if (type.equals(TESTCASE_TYPE.GVT_TOPIC)) {
			mp1 = new MessagingPolicy(mPolicyPubSubName, policyTestBean.getGvtString(), DESTINATION_TYPE.TOPIC, MESSAGING_ACTION.PUB_SUB);
		} else {
			mp1 = new MessagingPolicy(mPolicyPubSubName,policyTestBean.getMp1TopicName(), DESTINATION_TYPE.TOPIC, MESSAGING_ACTION.PUB_SUB);
		}

		mp1.setId(policyTestBean.getMp1UserId());
		mp1.setGroup(policyTestBean.getMp1GroupId());

		mp1.setClientAddress(policyTestBean.getMp1ClientAddr());
		mp1.setClientID(policyTestBean.getMp1ClientId());

		mp1.setProtocol(policyTestBean.getMp1Protocol());

		return mp1;
	}


	private MessagingPolicy getMessagePolicy2() {

		MessagingPolicy mp2 = null;

		mp2 = new MessagingPolicy(mPolicyPubName, policyTestBean.getMp1TopicName(), DESTINATION_TYPE.TOPIC, MESSAGING_ACTION.PUB);

		mp2.setGroup(policyTestBean.getMp2GroupId());
		mp2.setId(policyTestBean.getMp2UserId());
		mp2.setClientID(policyTestBean.getMp2ClientId());
		mp2.setClientAddress(policyTestBean.getMp2ClientAddr());
		mp2.setProtocol(policyTestBean.getMp2Protocol());

		return mp2;
	}


	private void getValidPublishXMLWithSeparation() {

		CLIENTS pubClient = null;
		CLIENTS cpClient = policyTestBean.getCpClientAddrEnum();
		CLIENTS mp2Client = policyTestBean.getMp2ClientAddrEnum();

		if (cpClient == null || cpClient.equals(CLIENTS.M1_M2)) {
			// mp2Client is the one....
			if (mp2Client != null && !(mp2Client.equals(CLIENTS.M1_M2))) {
				pubClient = mp2Client;
			} else {
				pubClient = cpClient;
			}
		} else {
			pubClient = cpClient;
		}



		if (pubClient != null) {
			if (pubClient.equals(CLIENTS.M1) || pubClient.equals(CLIENTS.M1_M2)) {
				pubClientMachine = "m1";
			} else {
				pubClientMachine = "m2";

			}
		} else {
			pubClientMachine = "m1";
		}

		PROTOCAL protocol = PROTOCAL.fromString(policyTestBean.getMp2Protocol());
		PROTOCAL cpProtocol = PROTOCAL.fromString(policyTestBean.getCpProtocol());

		if (cpProtocol.equals(PROTOCAL.JMS)) {
			publishValidator = new JMSTransmitter(realTopicName, port);
		} else if (cpProtocol.equals(PROTOCAL.MQTT)) {
			publishValidator = new MQTTTransmitter(realTopicName, port);
		} else {
			if (protocol.equals(PROTOCAL.JMS) || protocol.equals(PROTOCAL.JMS_MQTT)) {
				publishValidator = new JMSTransmitter(realTopicName, port);
			} else {
				publishValidator = new MQTTTransmitter(realTopicName, port);
			}
		}


		if ((policyTestBean.getCpClientId() != null) || (policyTestBean.getMp2ClientId() != null)) {
			publishValidator.setClientId("CID" + testCaseIdentifier + "_2");
		}
		if (user2String != null) {
			publishValidator.setUser(user2String);
			publishValidator.setPassword("ismtest");
		} else if (policyTestBean.getCpGoupId() != null || policyTestBean.getCpUserId() != null) {

			publishValidator.setUser(user1String);
			publishValidator.setPassword("ismtest");

		}

		xmlFiles.add(publishValidator);



	}


	private void getValidPublishXMLNoSeparation() {

		CLIENTS pubClient = null;
		CLIENTS cpClient = policyTestBean.getCpClientAddrEnum();
		CLIENTS mp1Client = policyTestBean.getMp1ClientAddrEnum();

		if (cpClient == null || cpClient.equals(CLIENTS.M1_M2)) {
			// mp1Client is the one....
			if (mp1Client != null && !(mp1Client.equals(CLIENTS.M1_M2))) {
				pubClient = mp1Client;
			} else {
				pubClient = cpClient;
			}
		} else {
			pubClient = cpClient;
		}

		/*	CLIENTS pubClient = policyTestBean.getMp1ClientAddrEnum();
		if (pubClient == null) {
			pubClient = policyTestBean.getCpClientAddrEnum();
		}*/

		if (pubClient != null) {
			if (pubClient.equals(CLIENTS.M1) || pubClient.equals(CLIENTS.M1_M2)) {
				pubClientMachine = "m1";
			} else {
				pubClientMachine = "m2";

			}
		} else {
			pubClientMachine = "m1";
		}


		PROTOCAL protocol = PROTOCAL.fromString(policyTestBean.getMp1Protocol());
		PROTOCAL cpProtocol = PROTOCAL.fromString(policyTestBean.getCpProtocol());


		if (cpProtocol.equals(PROTOCAL.JMS)) {
			publishValidator = new JMSTransmitter(realTopicName, port);
		} else if (cpProtocol.equals(PROTOCAL.MQTT)) {

			publishValidator = new MQTTTransmitter(realTopicName, port);
		} else {
			if (protocol.equals(PROTOCAL.JMS) || protocol.equals(PROTOCAL.JMS_MQTT)) {
				publishValidator = new JMSTransmitter(realTopicName, port);
			} else {
				publishValidator = new MQTTTransmitter(realTopicName, port);
			}
		}

		if ((policyTestBean.getCpClientId() != null) || (policyTestBean.getMp1ClientId() != null)) {
			if (type.equals(TESTCASE_TYPE.GVT_TOPIC)) {
				publishValidator.setClientId(policyTestBean.getGvtString() + "_2");
			} else {
				publishValidator.setClientId("CID" + testCaseIdentifier + "_2");
			}
		}
		if (user1String != null) {
			publishValidator.setUser(user1String);
			publishValidator.setPassword("ismtest");
		}
		publishValidator.setTopic(realTopicName);

		xmlFiles.add(publishValidator);



	}

	private void getSubscribeXml() {


		PROTOCAL cpProtocol = PROTOCAL.fromString(policyTestBean.getCpProtocol());
		PROTOCAL mp1Protocol = PROTOCAL.fromString(policyTestBean.getMp1Protocol());

		CLIENTS subClient = null;
		CLIENTS cpClient = policyTestBean.getCpClientAddrEnum();
		CLIENTS mp1Client = policyTestBean.getMp1ClientAddrEnum();

		if (cpClient == null || cpClient.equals(CLIENTS.M1_M2)) {
			// mp1Client is the one....
			if (mp1Client != null && !(mp1Client.equals(CLIENTS.M1_M2))) {
				subClient = mp1Client;
			} else {
				subClient = cpClient;
			}
		} else {
			subClient = cpClient;
		}





		if (subClient != null) {
			if (subClient.equals(CLIENTS.M1) || subClient.equals(CLIENTS.M1_M2)) {
				subClientMachine = "m1";
			} else {
				subClientMachine = "m2";

			}
		} else {
			subClientMachine = "m1";
		}

		if (cpProtocol.equals(PROTOCAL.JMS)) {
			subscribeValidator = new JMSReceiver(realTopicName, port);
		} else if (cpProtocol.equals(PROTOCAL.MQTT)) {
			subscribeValidator = new MQTTReceiver(realTopicName, port);
		} else {
			if (mp1Protocol.equals(PROTOCAL.JMS) || mp1Protocol.equals(PROTOCAL.JMS_MQTT)) {
				subscribeValidator = new JMSReceiver(realTopicName, port);
			} else {
				subscribeValidator = new MQTTReceiver(realTopicName, port);
			}
		}


		/*	if ((cpProtocol.equals(PROTOCAL.JMS) || cpProtocol.equals(PROTOCAL.JMS_MQTT)) && 
					(mp1Protocol.equals(PROTOCAL.JMS) || mp1Protocol.equals(PROTOCAL.JMS_MQTT))) {
				subscribeValidator = new JMSReceiver(realTopicName, testCaseIdentifier);
			} else {
				subscribeValidator = new MQTTReceiver(realTopicName, testCaseIdentifier);
			}
		 */


		if ((policyTestBean.getCpClientId() != null) || (policyTestBean.getMp1ClientId() != null)) {
			if (type.equals(TESTCASE_TYPE.GVT_TOPIC)) {
				subscribeValidator.setClientId(policyTestBean.getGvtString() + "_1");
			} else {
				subscribeValidator.setClientId("CID" + testCaseIdentifier + "_1");
			}
		}
		if (user1String != null) {
			subscribeValidator.setUser(user1String);
			subscribeValidator.setPassword("ismtest");
		}
		subscribeValidator.setTopic(realTopicName);

		xmlFiles.add(subscribeValidator);
	}



	private void groupsAndUsers() {


		if (policyTestBean.createUser1()) {
			user1String = "USR" + testCaseIdentifier + "_1";
			MessagingUser user1 = new MessagingUser(user1String, "ismtest");
			if (policyTestBean.createGroup1()) {
				String group1String = "GRP" + testCaseIdentifier + "_1";
				MessagingGroup group1 = new MessagingGroup(group1String);
				user1.addGroup(group1);
				groups.add(group1);
			}
			users.add(user1);

		}

		if (policyTestBean.createUser2()) {
			user2String = "USR" + testCaseIdentifier + "_2";
			MessagingUser user2 = new MessagingUser(user2String, "ismtest");
			if (policyTestBean.createGroup2()) {
				String group2String = "GRP" + testCaseIdentifier + "_2";
				MessagingGroup group2 = new MessagingGroup(group2String);
				user2.addGroup(group2);
				groups.add(group2);
			}
			users.add(user2);

		}		

	}

	public void writeCommands(OutputStream out) throws IOException {

		for (MessagingGroup group : groups) {
			group.writeCliAddCommand(testSuiteName, testCaseIdentifier, out);
		}

		for (MessagingUser user : users) {
			user.writeAddCommand(testSuiteName, testCaseIdentifier, out);
		}

		List<Endpoint> endpoints = hub.getEndpoints();
		for (Endpoint endpoint : endpoints) {
			List<ConnectionPolicy> connPolicies = endpoint.getConnPolicyList();
			for (ConnectionPolicy connPolicy : connPolicies) {
				connPolicy.writeConnectionPolicySet(testSuiteName, testCaseIdentifier, out);
			}
			List<MessagingPolicy> mPolocies = endpoint.getMsgPolicyList();
			for (MessagingPolicy mPolicy : mPolocies) {
				mPolicy.writeMessagingPoliciesSet(testSuiteName, testCaseIdentifier, out);
			}

		}

		hub.writeHubSet(testSuiteName, testCaseIdentifier, out);

		for (Endpoint endpoint : endpoints) {
			endpoint.writeCliCreateCommand(out);
		}

		if (doUpdates) {

			endpoints = hub.getEndpoints();
			for (Endpoint endpoint : endpoints) {
				List<ConnectionPolicy> connPolicies = endpoint.getConnPolicyList();
				for (ConnectionPolicy connPolicy : connPolicies) {
					connPolicy.writeConnectionPolicyUpdateSet(testSuiteName, testCaseIdentifier, out);
				}
				List<MessagingPolicy> mPolocies = endpoint.getMsgPolicyList();
				for (MessagingPolicy mPolicy : mPolocies) {
					mPolicy.writeMessagingPoliciesUpdateSet(testSuiteName, testCaseIdentifier, out);
				}

			}

			hub.writeHubUpdateSet(testSuiteName, testCaseIdentifier, out);
			for (Endpoint endpoint : endpoints) {
				endpoint.writeCliUpdateCommand(out);
			}
		}

		for (Endpoint endpoint : endpoints) {
			endpoint.writeCliDeleteCommand(out);
		}


		for (MessagingGroup group : groups) {
			group.writeCliDeleteCommand(testSuiteName, testCaseIdentifier, out);
		}


		for (MessagingUser user : users) {
			user.writeDeleteCommand(testSuiteName, testCaseIdentifier, out);
		}

		for (Endpoint endpoint : endpoints) {
			List<ConnectionPolicy> connPolicies = endpoint.getConnPolicyList();
			for (ConnectionPolicy connPolicy : connPolicies) {
				connPolicy.writeConnectionPolicyDelete(testSuiteName, testCaseIdentifier, out);
			}
			List<MessagingPolicy> mPolocies = endpoint.getMsgPolicyList();
			for (MessagingPolicy mPolicy : mPolocies) {
				mPolicy.writeMessagingPoliciesDelete(testSuiteName, testCaseIdentifier, out);
			}

		}

		hub.writeHubDelete(testSuiteName, testCaseIdentifier, out);

	}

	public void writeTestScript(String zipBaseDir, OutputStream out) throws IOException {

		if (subscribeValidator == null || publishValidator == null) return;

		String test = testSuiteName + "_" + testCaseIdentifier;
		String cleanTest = testSuiteName + "_" + testCaseIdentifier + "_CLEAN";
		String testRoot = zipBaseDir + testCaseIdentifier + "/"; 

		String cliFile = testRoot + testSuiteName + "_" + testCaseIdentifier + ".cli";

		String subscriberXml = testRoot + subscribeValidator.getXmlName();
		String publisherXml = testRoot + publishValidator.getXmlName();

		StringBuffer buf = new StringBuffer();

		buf.append("# Test number: " + testCaseIdentifier + SEP);
		buf.append("xml[${n}]=\"" + testSuiteName + "_"  + testCaseIdentifier + "\"" + SEP);
		buf.append("timeouts[${n}]=50" + SEP);
		buf.append("scenario[${n}]=\"${xml[${n}]} - policy test " + testCaseIdentifier + "\"" + SEP);

		buf.append("component[1]=cAppDriverLogWait,m1,\"-e|${M1_TESTROOT}/scripts/run-cli.sh\",\"-o|-c|" 
				+ "" + cliFile + "|-s|" + test + "\"" + SEP);
		buf.append("component[2]=sync,m1" + SEP);

		buf.append("component[3]=" + subscribeValidator.getDriverName() + "," + subClientMachine + "," + subscriberXml +",rmdr" + SEP);

		buf.append("component[4]=" + publishValidator.getDriverName() + "," + pubClientMachine + "," + publisherXml + ",rmdt" + SEP);

		buf.append("components[${n}]=\"${component[1]} ${component[2]} ${component[3]} ${component[4]}\"" + SEP);
		buf.append("((n+=1))" + SEP);
		buf.append(SEP);

		buf.append("# Clean-up test number: " + testCaseIdentifier + SEP);
		buf.append("xml[${n}]=\"" + testSuiteName + "_"  + testCaseIdentifier + "_CLEAN\"" + SEP);
		buf.append("timeouts[${n}]=20" + SEP);
		buf.append("scenario[${n}]=\"${xml[${n}]} - clean policy test " + testCaseIdentifier + "\"" + SEP);

		buf.append("component[1]=cAppDriverLogWait,m1,\"-e|${M1_TESTROOT}/scripts/run-cli.sh\",\"-o|-c|" 
				+ "" + cliFile + "|-s|" + cleanTest + "\"" + SEP);
		buf.append("components[${n}]=\"${component[1]}\"" + SEP);
		buf.append("((n+=1))" + SEP);
		buf.append(SEP);
		byte[] byteArray = buf.toString().getBytes();
		out.write(byteArray);

	}


	public List<MessagingUser> getUsers() {
		return users;
	}



	public List<MessagingGroup> getGroups() {
		return groups;
	}



	public String getTestCaseIdentifier() {
		return testCaseIdentifier;
	}


	public MessageHub getHub() {
		return hub;
	}



	public List<IPolicyValidator> getXmlFiles() {
		return xmlFiles;
	}

	public void setDoUpdates(boolean doUpdates) {
		this.doUpdates = doUpdates;
	}




}
