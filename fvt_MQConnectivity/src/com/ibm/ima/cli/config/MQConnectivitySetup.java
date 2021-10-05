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
/*
package com.ibm.ima.cli.config;

import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.config.ImaMQConnectivity;
import com.ibm.ima.test.cli.config.ImaConfig.TYPE;
*/
/**
 * 
 * @Name		: MQConnectivitySetup 
 * @Author		:
 * @Created On	: 05/06/2013
 * @Category	: FVT
 * 
 * @Description : Creates a MQConnectivity setup rule using JSCH.  
 * 
 * **/
/*
public class MQConnectivitySetup 
{
	private ImaConfig imaConfigHelper;
	private ImaMQConnectivity imaMQConfigHelper;
	
	private static String ip;
	private static String usrname;
	private static String pswd;

	private TYPE protocol;
	private TYPE destinationType;
	
	public static enum ConfigTask
	{
		createCP, 
		deleteCP, 
		createMP, 
		deleteMP, 
		createEP, 
		deleteEP, 
		createMH, 
		deleteMH, 
		createQMC, 
		deleteQMC,
		createDMR,
		deleteDMR,
		createQueue,
		deleteQueue;
	}
	
	public void createCP(String policyname, String description, String protocolString) throws Exception
	{
		imaConfigHelper = new ImaConfig(ip, usrname, pswd);
		if (protocolString.equalsIgnoreCase("jmsmqtt"))
			protocol=TYPE.JMSMQTT;
		else if(protocolString.equalsIgnoreCase("jms"))
			protocol=TYPE.JMS;
		if(imaConfigHelper.connectionPolicyExists(policyname))
			imaConfigHelper.deleteConnectionPolicy(policyname);
		imaConfigHelper.createConnectionPolicy(policyname, description, protocol);
	}
	
	public void deleteCP(String policyname) throws Exception
	{
		imaConfigHelper = new ImaConfig(ip, usrname, pswd);
		imaConfigHelper.deleteConnectionPolicy(policyname);
	}
	
	public void createMP(String policyname, String description, String destinationName, String destinationTypeString, String protocolString) throws Exception
	{
		imaConfigHelper = new ImaConfig(ip, usrname, pswd);
		if (protocolString.equalsIgnoreCase("jmsmqtt"))
			protocol=TYPE.JMSMQTT;
		if (destinationTypeString.equalsIgnoreCase("topic"))
			destinationType=TYPE.Topic;
		else if(destinationTypeString.equalsIgnoreCase("queue"))
			destinationType=TYPE.Queue;
		
		if(imaConfigHelper.messagingPolicyExists(policyname))
			imaConfigHelper.deleteMessagingPolicy(policyname);
		imaConfigHelper.createMessagingPolicy(policyname, description, destinationName, destinationType, protocol);
	}	

	public void deleteMP(String policyname) throws Exception
	{
		imaConfigHelper = new ImaConfig(ip, usrname, pswd);
		imaConfigHelper.deleteMessagingPolicy(policyname);
	}	
	
	public void createEP(String endpointName, String description, String port, String protocolString, String connectionPolicies, String messagingPolicies, String hub) throws Exception
	{
		imaConfigHelper = new ImaConfig(ip, usrname, pswd);
		if (protocolString.equalsIgnoreCase("jmsmqtt"))
			protocol=TYPE.JMSMQTT;
		if(imaConfigHelper.endpointExists(endpointName))
			imaConfigHelper.deleteEndpoint(endpointName);
		imaConfigHelper.createEndpoint(endpointName, description, port, protocol, connectionPolicies, messagingPolicies, hub);
	}	
	
	public void deleteEP(String endpointName) throws Exception
	{
		imaConfigHelper = new ImaConfig(ip, usrname, pswd);
		imaConfigHelper.deleteEndpoint(endpointName);
	}
	
	public void createMH(String msgHubName, String description) throws Exception
	{
		imaConfigHelper = new ImaConfig(ip, usrname, pswd);
		if(imaConfigHelper.endpointExists(msgHubName))
			imaConfigHelper.deleteEndpoint(msgHubName);
		imaConfigHelper.createHub(msgHubName, description);
	}
	
	public void deleteMH(String msgHubName) throws Exception
	{
		imaConfigHelper = new ImaConfig(ip, usrname, pswd);
		imaConfigHelper.deleteMessageHub(msgHubName);
	}	
	
	public void createQueue(String queueName, String discardMessages, String description, String maxMessages) throws Exception
	{
		Boolean discardMsgsFlag = true;
		if (discardMessages.equalsIgnoreCase("false"))
			discardMsgsFlag=false;
		imaConfigHelper = new ImaConfig(ip, usrname, pswd);
		if(imaConfigHelper.queueExists(queueName))
			imaConfigHelper.deleteQueue(queueName, discardMsgsFlag);
		imaConfigHelper.createQueue(queueName, description, Integer.parseInt(maxMessages));
	}
	
	public void deleteQueue(String queueName, String discardMessages) throws Exception
	{		
		Boolean discardMsgsFlag = true;
		if (discardMessages.equalsIgnoreCase("false"))
			discardMsgsFlag=false;
		imaConfigHelper = new ImaConfig(ip, usrname, pswd);
		imaConfigHelper.deleteQueue(queueName, discardMsgsFlag);
	}
	
	public void createQMC(String name, String qmgr, String connName, String chlName) throws Exception
	{
		imaMQConfigHelper = new ImaMQConnectivity(ip, usrname, pswd);
		if(imaMQConfigHelper.queueManagerConnectionExists(name))			
			imaMQConfigHelper.deleteQueueManagerConnection(name);
		imaMQConfigHelper.createQueueManagerConnection(name, qmgr, connName, chlName);
	}	
	
	public void deleteQMC(String name) throws Exception
	{
		imaMQConfigHelper = new ImaMQConnectivity(ip, usrname, pswd);
		imaMQConfigHelper.deleteQueueManagerConnection(name);
	}	
	
	public void createDMR(String name, String qmgrConn, String ruleType, String source, String destination) throws Exception
	{
		imaMQConfigHelper = new ImaMQConnectivity(ip, usrname, pswd);
		if(imaMQConfigHelper.mappingRuleExists(name))
		{
			imaMQConfigHelper.updateMappingRule(name, "Enabled", "False");
			imaMQConfigHelper.deleteMappingRule(name);
		}
		imaMQConfigHelper.createMappingRule(name, qmgrConn, ruleType, source, destination);
	}	
	
	public void deleteDMR(String name) throws Exception
	{
		imaMQConfigHelper = new ImaMQConnectivity(ip, usrname, pswd);
		imaMQConfigHelper.updateMappingRule(name, "Enabled", "False");
		imaMQConfigHelper.deleteQueueManagerConnection(name);
	}	

	public static void main(String args[]) throws Exception
	{
		MQConnectivitySetup mqcs = new MQConnectivitySetup(); 
		ip=args[0];
		usrname=args[1];
		pswd=args[2];

		switch (ConfigTask.valueOf(args[3]))
		{
			case createCP: mqcs.createCP(args[4],args[5],args[6]);;
			case deleteCP: mqcs.deleteCP(args[4]);
			case createMP: mqcs.createMP(args[4],args[5],args[6],args[7],args[8]);
			case deleteMP: mqcs.deleteMP(args[4]);
			case createEP: mqcs.createEP(args[4],args[5],args[6],args[7],args[8],args[9],args[10]);
			case deleteEP: mqcs.deleteEP(args[4]);
			case deleteMH: mqcs.deleteMH(args[4]);
			case createQMC: mqcs.createQMC(args[4], args[5],args[6],args[7]);
			case deleteQMC: mqcs.deleteQMC(args[4]);
			case createDMR: mqcs.createDMR(args[4], args[5],args[6],args[7],args[8]);
			case deleteDMR: mqcs.deleteDMR(args[4]);			
		}
	}
	
}
*/
