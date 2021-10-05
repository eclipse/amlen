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
package com.ibm.ima.test.cli.config;

import com.ibm.ima.test.cli.imaserver.ImaCommandReference;

/**
 * TODO - add more mqconnectivity commands such as ssl key repository etc
 * NOTE: JSCH LIBRARIES ONLY TO BE USED FOR AUTOMATION
 * 
 * 
 * This class contains helper methods to create, delete, interrogate mqconnectivity configuration objects in the appliance
 *
 */
public class ImaMQConnectivity extends ImaConfigUtils{
	
	
	private String createQmgrConn = null;
	private String createMappingRule = null;
	
	public static enum MQTYPE {QueueManagerConnection, DestinationMappingRule}
	
	
	/**
	 * Returns an object that can perform mqconnectivity commands
	 * 
	 * @param ipaddress - the imaserver address
	 * @param username - the imaserver administrators username
	 * @param password - the imaserver administrators password
	 * 
	 * @throws Exception if the properties file for the command list cannot be found
	 */
	public ImaMQConnectivity(String ipaddress, String username, String password) throws Exception
	{
		super(ipaddress, username, password);
		
		createQmgrConn = ImaCommandReference.QMGRCONN_CREATE;	
		createMappingRule = ImaCommandReference.MAPPING_CREATE;	

	}
	
	
	/**
	 * Returns an object that can perform mqconnectivity commands
	 * 
	 * @param ipaddressA - the imaserver address for the first HA appliance
	 * @param ipaddressB - the imaserver address for the second HA appliance
	 * @param username - the imaserver administrators username
	 * @param password - the imaserver administrators password
	 * 
	 * @throws Exception if the properties file for the command list cannot be found
	 */
	public ImaMQConnectivity(String ipaddressA, String ipaddressB, String username, String password) throws Exception
	{
		super(ipaddressA, ipaddressB, username, password);
		
		createQmgrConn = ImaCommandReference.QMGRCONN_CREATE;	
		createMappingRule = ImaCommandReference.MAPPING_CREATE;	

	}
	
	
	/**
	 * This method create a queue manager connection - note if you wish to add additional properties such
	 * as SSL cipher create the qmgr connection and then use the update command
	 * @param name - of the queue manager connection
	 * @param qmgr - the name of the queue manager
	 * @param connName - connection name
	 * @param chlName - channel name
	 * 
	 * @throws Exception if the queue manager connection could not be created
	 */
	public void createQueueManagerConnection(String name, String qmgr, String connName, String chlName) throws Exception
	{
		isConnectedToServer();
		String command = createQmgrConn.replace("{NAME}", name);
		command = command.replace("{QMGRNAME}", qmgr);
		command = command.replace("{CONNAME}", connName);
		command = command.replace("{CHLNAME}", chlName);
		
		sshExecutor.exec(command, timeout);
		
	}
	
	
	/**
	 * This method can be used to update an existing QueueManagerConnection. An exception will
	 * be thrown if the QueueManagerConnection does not exist
	 * 
	 * You can find the parameters in the properties file under the section
	 * #Commands used for additional mq parameters
	 * 
	 * @param name - of the QueueManagerConnection to update
	 * @param parameter - the parameter to update
	 * @param value - the value of the parameter
	 * @throws Exception
	 */
	public void updateQueueManagerConnection(String name, String parameter, String value) throws Exception
	{
		isConnectedToServer();
		updateObject(MQTYPE.QueueManagerConnection.toString(), name, parameter, value);
	}
	
	
	
	/**
	 * This method creates a mapping rule - note if you wish to add additional properties such
	 * as max messages or change the enabled property use the updateMappingRule() method once created.
	 * @param name - of the mapping rule
	 * @param qmgrConn - the name of the queue manager connection
	 * @param ruleType - type 1-14
	 * @param source - source
	 * @param destination - destination
	 * 
	 * @throws Exception if the mapping rule could not be created
	 */
	public void createMappingRule(String name, String qmgrConn, String ruleType, String source, String destination) throws Exception
	{
		isConnectedToServer();
		String command = createMappingRule.replace("{NAME}", name);
		command = command.replace("{QMGRCONN}", qmgrConn);
		command = command.replace("{RULETYPE}", ruleType);
		command = command.replace("{SOURCE}", source);
		command = command.replace("{DEST}", destination);
		
		sshExecutor.exec(command, timeout);
		
	}
	
	
	/**
	 * This method can be used to update an existing mapping rule. An exception will
	 * be thrown if the mapping rule does not exist
	 * 
	 * You can find the parameters in the properties file under the section
	 * #Commands used for additional mq parameters
	 * 
	 * @param name - of the mapping rule to update
	 * @param parameter - the parameter to update
	 * @param value - the value of the parameter
	 * @throws Exception
	 */
	public void updateMappingRule(String name, String parameter, String value) throws Exception
	{
		isConnectedToServer();
		updateObject(MQTYPE.DestinationMappingRule.toString(), name, parameter, value);
	}
	
	/**
	 * This method will check to see if a QueueManagerConnection can be found
	 * @param name to find
	 * @return true if found; false otherwise
	 * @throws Exception
	 */
	public boolean queueManagerConnectionExists(String name) throws Exception
	{
		isConnectedToServer();
		return doesObjectExist(MQTYPE.QueueManagerConnection.toString(), name);
	}
	
	
	/**
	 * This method will check to see if a DestinationMappingRule can be found
	 * @param name to find
	 * @return true if found; false otherwise
	 * @throws Exception
	 */
	public boolean mappingRuleExists(String name) throws Exception
	{
		isConnectedToServer();
		return doesObjectExist(MQTYPE.DestinationMappingRule.toString(), name);
	}
	
	
	
	/**
	 * This method will delete an QueueManagerConnection if not already in use
	 * @param QueueManagerConnection name
	 * @throws Exception will be thrown if the object is in use and cannot be deleted
	 */
	public void deleteQueueManagerConnection(String name) throws Exception
	{
		isConnectedToServer();
		deleteObject(MQTYPE.QueueManagerConnection.toString(), name);
	}
	
	/**
	 * This method will delete an MappingRule if not already in use
	 * @param MappingRule name
	 * @throws Exception will be thrown if the object is in use and cannot be deleted
	 */
	public void deleteMappingRule(String name) throws Exception
	{
		isConnectedToServer();
		deleteObject(MQTYPE.DestinationMappingRule.toString(), name);
	}
	

	
}
