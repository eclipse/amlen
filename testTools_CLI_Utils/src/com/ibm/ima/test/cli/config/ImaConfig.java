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

import java.util.Map;

import com.ibm.ima.test.cli.imaserver.ImaCommandReference;
import com.ibm.ima.test.cli.util.CLICommand;


/**
 * TODO - Add more configuration commands such as creating security profiles.
 *
 * NOTE: JSCH LIBRARIES ONLY TO BE USED FOR AUTOMATION
 *
 * This class contains helper methods to create, delete, interrogate configuration objects in the appliance,
 * i.e. Queues, Hubs, Endpoints, Policies, Users, Groups etc (Note - mqconnectivity configuration is kept 
 * in a different class)
 */
public class ImaConfig extends ImaConfigUtils{
		
	private String createMsgPolicy = null;
	private String createConnPolicy = null;
	private String createEndpoint = null;
	private String createHub = null;
	private String createQueue = null;
	private String createUser = null;
	private String userExists = null;
	private String userNotFound = null;
	private String userPassword = null;
	private String userGroup = null;
	private String userDelete = null;
	private String createGroup = null;
	private String createGroupMember = null;
	private String groupGroupMember = null;
	private String groupNotFound = null;
	private String groupExists = null;
	private String groupDelete = null;
	private String subDelete = null;
	private String mqttDelete = null;
	
	public static enum TYPE {Queue, MessageHub, ConnectionPolicy, MessagingPolicy, JMS, MQTT, JMSMQTT, Endpoint, Topic, Subscription}
	
	public static enum USER_GROUP_TYPE {WebUI, Messaging}
	
	public static enum USER_GROUP {SystemAdministrators, MessagingAdministrators, Users}
	
	/**
	 * This is a helper class for manipulating configuration objects
	 * @param session
	 * @param timeout
	 * @throws Exception
	 */
	public ImaConfig(String ipaddress, String username, String password) throws Exception
	{
		super(ipaddress, username, password);
		
		// Set up all the commands	
		createMsgPolicy = ImaCommandReference.MSGPOLICY_CREATE;
		createConnPolicy = ImaCommandReference.CONNPOLICY_CREATE;
		createEndpoint = ImaCommandReference.ENDPOINT_CREATE;
		createHub = ImaCommandReference.HUB_CREATE;
		createQueue = ImaCommandReference.QUEUE_CREATE;
		createUser = ImaCommandReference.USER_ADD;
		userExists = ImaCommandReference.USER_EXISTS;
		userNotFound = ImaCommandReference.NO_USER_FOUND;
		userPassword = ImaCommandReference.EDIT_USER_PASSWORD;
		userGroup = ImaCommandReference.EDIT_USER_GROUP;
		userDelete = ImaCommandReference.USER_DELETE;
		createGroup = ImaCommandReference.GROUP_ADD;
		createGroupMember = ImaCommandReference.GROUP_ADD_MEMBER;
		groupGroupMember = ImaCommandReference.GROUP_EDIT_MEMBER;
		groupNotFound = ImaCommandReference.NO_GROUP_FOUND;
		groupExists = ImaCommandReference.GROUP_EXISTS;
		groupDelete = ImaCommandReference.GROUP_DELETE;
		subDelete = ImaCommandReference.SUB_DELETE;
		mqttDelete = ImaCommandReference.MQTTCLIENT_DELETE;
		
		
	}
	
	/**
	 * This is a helper class for manipulating configuration objects
	 * @param session
	 * @param timeout
	 * @throws Exception
	 */
	public ImaConfig(String ipaddressA, String ipaddressB, String username, String password) throws Exception
	{
		super(ipaddressA, ipaddressB, username, password);
		
		// Set up all the commands	
		createMsgPolicy = ImaCommandReference.MSGPOLICY_CREATE;
		createConnPolicy = ImaCommandReference.CONNPOLICY_CREATE;
		createEndpoint = ImaCommandReference.ENDPOINT_CREATE;
		createHub = ImaCommandReference.HUB_CREATE;
		createQueue = ImaCommandReference.QUEUE_CREATE;
		createUser = ImaCommandReference.USER_ADD;
		userExists = ImaCommandReference.USER_EXISTS;
		userNotFound = ImaCommandReference.NO_USER_FOUND;
		userPassword = ImaCommandReference.EDIT_USER_PASSWORD;
		userGroup = ImaCommandReference.EDIT_USER_GROUP;
		userDelete = ImaCommandReference.USER_DELETE;
		createGroup = ImaCommandReference.GROUP_ADD;
		createGroupMember = ImaCommandReference.GROUP_ADD_MEMBER;
		groupGroupMember = ImaCommandReference.GROUP_EDIT_MEMBER;
		groupNotFound = ImaCommandReference.NO_GROUP_FOUND;
		groupExists = ImaCommandReference.GROUP_EXISTS;
		groupDelete = ImaCommandReference.GROUP_DELETE;
		subDelete = ImaCommandReference.SUB_DELETE;
		mqttDelete = ImaCommandReference.MQTTCLIENT_DELETE;
		
		
	}
	
	/**
	 * The basic methods for config manipulation should be added to this class if they are not found.
	 * However, there will be some circumstances where you may need to execute a non-standard command 
	 * which will not be often used. In this case you can use this 'catch' all method by passing in your
	 * own command to run.
	 * @param command - imaserver command to execute
	 * @throws Exception is thrown if the command cannot be executed
	 */
	public String executeOwnCommand(String command) throws Exception
	{
		isConnectedToServer();
		return sshExecutor.exec(command, timeout);		
	}
	
	/**
	 * This common method can be used to execute any command with or without options (args).
	 * @param command The first part of the command, can be any string (e.g. imaserver)
	 * @param options If not null, map of key-value pairs
	 * @return If successful, output of the command
	 * @throws Exception
	 */
	public CLICommand executeCommand(String command, Map<String,String> options) throws Exception {
		isConnectedToServer();
		StringBuilder sb = new StringBuilder(256);
		sb.append(command);
		if (options != null) {
			for (Map.Entry<String, String> e : options.entrySet()) {
				sb.append(" \"" + e.getKey() + "=" + e.getValue() + "\"");
			}
		}
		return sshExecutor.exec(sb.toString());		
	}
	
	/**
	 * This method will check to see if a MessageHub can be found
	 * @param hubName to find
	 * @return true if found; false otherwise
	 * @throws Exception
	 */
	public boolean messageHubExists(String hubName) throws Exception
	{
		isConnectedToServer();
		return doesObjectExist(TYPE.MessageHub.toString(), hubName);
	}
	
	/**
	 * This method will check to see if a connection policy can be found
	 * @param policy to find
	 * @return true if found; false otherwise
	 * @throws Exception
	 */
	public boolean connectionPolicyExists(String name) throws Exception
	{
		isConnectedToServer();
		return doesObjectExist(TYPE.ConnectionPolicy.toString(), name);
	}
	
	/**
	 * This method will check to see if a messaging policy can be found
	 * @param policy to find
	 * @return true if found; false otherwise
	 * @throws Exception
	 */
	public boolean messagingPolicyExists(String name) throws Exception
	{
		isConnectedToServer();
		return doesObjectExist(TYPE.MessagingPolicy.toString(), name);
	}
	
	/**
	 * This method will check to see if an endpoint can be found
	 * @param endpoint to find
	 * @return true if found; false otherwise
	 * @throws Exception
	 */
	public boolean endpointExists(String name) throws Exception
	{
		isConnectedToServer();
		return doesObjectExist(TYPE.Endpoint.toString(), name);
	}
	
	/**
	 * This method will check to see if a queue can be found
	 * @param queueName
	 * @return true if found; false otherwise
	 * @throws Exception
	 */
	public boolean queueExists(String queueName) throws Exception
	{
		isConnectedToServer();
		return doesObjectExist(TYPE.Queue.toString(), queueName);
	}
	
	
	/**
	 * This method will check to see if a topic can be found
	 * @param topicString
	 * @return true if found; false otherwise
	 * @throws Exception
	 */
	public boolean topicStringExists(String topicString) throws Exception
	{
		isConnectedToServer();
		return doesObjectExist(TYPE.Topic.toString(), topicString);
	}
	
	/**
	 * This method will create a Messaging Hub
	 * 
	 * @param name
	 * @param description
	 * @throws Exception if the hub could not be created
	 */
	public void createHub(String name, String description) throws Exception
	{
		isConnectedToServer();
		String command = createHub.replace("{NAME}", name);
		command = command.replace("{DESC}", description);
		
		sshExecutor.exec(command, timeout);
		
	}
	
	
	/**
	 * This method will create an Endpoint - note if you want to add more selection criteria then 
	 * perform an updateEndpoint(...) to add the extra information.
	 * 
	 * @param name
	 * @param description
	 * @param port
	 * @param protocol
	 * @param connectionPolicies
	 * @param messagingPolicies
	 * @param hub
	 * @throws Exception if the endpoint could not be created
	 */
	public void createEndpoint(String name, String description, String port, TYPE protocol, String connectionPolicies, String messagingPolicies, String hub) throws Exception
	{
		isConnectedToServer();
		String command = createEndpoint.replace("{NAME}", name);
		command = command.replace("{DESC}", description);
		command = command.replace("{PORT}", port);
		command = command.replace("{CONNPOL}", connectionPolicies);
		command = command.replace("{MSGPOL}", messagingPolicies);
		command = command.replace("{HUB}", hub);
		if(protocol.equals(TYPE.JMSMQTT))
		{
			command = command.replace("{PROTYPE}", "JMS,MQTT");
		}
		else
		{
			command = command.replace("{PROTYPE}", protocol.toString());
		}
		
		sshExecutor.exec(command, timeout);
	
	}
	
	/**
	 * This method can be used to update an existing Endpoint. An exception will
	 * be thrown if the endpoint does not exist
	 * 
	 * You can find the parameters in the properties file under the section
	 * #Commands used for additional selection parameters
	 * 
	 * @param name - of the endpoint to update
	 * @param parameter - the parameter to update
	 * @param value - the value of the parameter
	 * @throws Exception will be thrown if the object cannot be updated
	 */
	public void updateEndpoint(String name, String parameter, String value) throws Exception
	{
		isConnectedToServer();
		updateObject(TYPE.Endpoint.toString(), name, parameter, value);
	}

	/**
	 * This method will create a connection policy - note if you want to add more selection criteria then 
	 * perform an updateConnectionPolicy(...) to add the extra information.
	 * 
	 * @param name
	 * @param description
	 * @param protocol
	 * @throws Exception if the policy could not be created
	 */
	public void createConnectionPolicy(String policyname, String description, TYPE protocol) throws Exception
	{
		isConnectedToServer();
		String command = createConnPolicy.replace("{NAME}", policyname);
		command = command.replace("{DESC}", description);
		if(protocol.equals(TYPE.JMSMQTT))
		{
			command = command.replace("{PROTYPE}", "JMS,MQTT");
		}
		else
		{
			command = command.replace("{PROTYPE}", protocol.toString());
		}
			
		sshExecutor.exec(command, timeout);
		
	}
	
	/**
	 * This method can be used to update an existing connection policy. An exception will
	 * be thrown if the policy does not exist
	 * 
	 * You can find the parameters in the properties file under the section
	 * #Commands used for additional selection parameters
	 * 
	 * @param name - of the policy to update
	 * @param parameter - the parameter to update
	 * @param value - the value of the parameter
	 * @throws Exception will be thrown if the object cannot be updated
	 */
	public void updateConnectionPolicy(String policyName, String parameter, String value) throws Exception
	{
		isConnectedToServer();
		updateObject(TYPE.ConnectionPolicy.toString(), policyName, parameter, value);
	}
	
	
	/**
	 * This method will create a messaging policy - note if you want to add more selection criteria then 
	 * perform an updateMessagingPolicy(...) to add the extra information.
	 * 
	 * @param name
	 * @param description
	 * @param destinationName
	 * @param destinationType
	 * @param protocol
	 * @throws Exception if the policy could not be created
	 */
	public void createMessagingPolicy(String policyname, String description, String destinationName, TYPE destinationType,  TYPE protocol) throws Exception
	{
		isConnectedToServer();
		String command = createMsgPolicy.replace("{NAME}", policyname);
		command = command.replace("{DESC}", description);
		command = command.replace("{DEST}", destinationName);
		command = command.replace("{DESTTYPE}", destinationType.toString());
		if(protocol.equals(TYPE.JMSMQTT))
		{
			command = command.replace("{PROTYPE}", "JMS,MQTT");
		}
		else
		{
			command = command.replace("{PROTYPE}", protocol.toString());
		}
		if(destinationType.equals(TYPE.Queue))
		{
			command = command.replace("{ACTION}", ImaCommandReference.MSGPOLICY_QUEUEACTION);
		}
		else if(destinationType.equals(TYPE.Topic))
		{
			command = command.replace("{ACTION}", ImaCommandReference.MSGPOLICY_TOPICACTION);
		}
		else if(destinationType.equals(TYPE.Subscription))
		{
			command = command.replace("{ACTION}", ImaCommandReference.MSGPOLICY_SUBSCRIPTIONACTION);
		}
		
		sshExecutor.exec(command, timeout);
		
	}
	
	/**
	 * This method can be used to update an existing messaging policy. An exception will
	 * be thrown if the policy does not exist
	 * 
	 * You can find the parameters in the properties file under the section
	 * #Commands used for additional selection parameters
	 * 
	 * @param name - of the policy to update
	 * @param parameter - the parameter to update
	 * @param value - the value of the parameter
	 * @throws Exception will be thrown if the object cannot be updated
	 */
	public void updateMessagingPolicy(String policyName, String parameter, String value) throws Exception
	{
		isConnectedToServer();
		updateObject(TYPE.MessagingPolicy.toString(), policyName, parameter, value);
	}
	
	
	
	/**
	 * This method will create a queue - note if you want to add more selection criteria then 
	 * perform an updateQueue(...) to add the extra information.
	 * 
	 * @param name
	 * @param description
	 * @throws Exception if the queue could not be created
	 */
	public void createQueue(String name, String description, int maxMessages) throws Exception
	{
		String command = createQueue.replace("{NAME}", name);
		command = command.replace("{DESC}", description);
		command = command.replace("{MSGS}",Integer.toString(maxMessages));

		sshExecutor.exec(command, timeout);
		
	}
	
	/**
	 * This method can be used to update an existing queue. An exception will
	 * be thrown if the queue does not exist
	 * 
	 * You can find the parameters in the properties file under the section
	 * #Commands used for additional selection parameters
	 * 
	 * @param name - of the queue to update
	 * @param parameter - the parameter to update
	 * @param value - the value of the parameter
	 * @throws Exception will be thrown if the object cannot be updated
	 */
	public void updateQueue(String name, String parameter, String value) throws Exception
	{
		isConnectedToServer();
		updateObject(TYPE.Queue.toString(), name, parameter, value);
	}
	
	/**
	 * This method will delete a messaging policy if not already in use
	 * @param policyname
	 * @throws Exception will be thrown if the object is in use and cannot be deleted
	 */
	public void deleteMessagingPolicy(String policyname) throws Exception
	{
		isConnectedToServer();
		deleteObject(TYPE.MessagingPolicy.toString(), policyname);
	}
	
	/**
	 * This method will delete an endpoint if not already in use
	 * @param endpointName
	 * @throws Exception will be thrown if the object is in use and cannot be deleted
	 */
	public void deleteEndpoint(String endpointName) throws Exception
	{
		isConnectedToServer();
		deleteObject(TYPE.Endpoint.toString(), endpointName);
	}
	
	/**
	 * This method will delete an messageHub if not already in use
	 * @param hubName
	 * @throws Exception will be thrown if the object is in use and cannot be deleted
	 */
	public void deleteMessageHub(String hubName) throws Exception
	{
		isConnectedToServer();
		deleteObject(TYPE.MessageHub.toString(), hubName);
	}
	
	
	/**
	 * This method will delete a connection policy if not already in use
	 * @param policyName
	 * @throws Exception will be thrown if the object is in use and cannot be deleted
	 */
	public void deleteConnectionPolicy(String policyName) throws Exception
	{
		isConnectedToServer();
		deleteObject(TYPE.ConnectionPolicy.toString(), policyName);
	}
	
	/**
	 * This method will delete the queue if it not in use
	 * @param queue
	 * @throws Exception will be thrown if the object is in use and cannot be deleted
	 */
	public void deleteQueue(String queue, boolean discardMessages) throws Exception
	{
		isConnectedToServer();
		if(discardMessages)
		{
			String deleteQueue = ImaCommandReference.QUEUE_DELETE;
			String command = deleteQueue.replace("{NAME}", queue);
			command = command.replace("{DISCARD}", Boolean.toString((discardMessages)));
			
			sshExecutor.exec(command, timeout);
			
		}
		else
		{
			deleteObject(TYPE.Queue.toString(), queue);
		}
	}
	
	/**
	 * This method creates a new user
	 * @param name - the name of the user which must be unique on the system
	 * @param password - password
	 * @param type - this must be either IMSUtils.USER_TYPE.WebUI or IMSUtils.USER_TYPE.Messaging
	 * @param member - the name of the group
	 * @param description
	 * @throws Exception if the user cannot be created (user already exists, group does not exist etc)
	 */
	public void createUser(String name, String password, USER_GROUP_TYPE type, String group, String description) throws Exception
	{
		isConnectedToServer();
		String command = createUser.replace("{ID}", name);
		command = command.replace("{PASSWORD}", password);
		command = command.replace("{TYPE}", type.toString());
		command = command.replace("{MEMBER}", group);
		command = command.replace("{DESC}", description);

		sshExecutor.exec(command, timeout);
		
	}
	
	
	/**
	 * This method checks to see if a particular user exists on the system
	 * @param name of the user
	 * @param type of the user
	 * @return true if the user exists; false otherwise
	 * @throws Exception
	 */
	public boolean userExists(String name, USER_GROUP_TYPE type) throws Exception
	{
		isConnectedToServer();
		String command = userExists.replace("{ID}", name);
		command = command.replace("{TYPE}", type.toString());
		
		try
		{
			sshExecutor.exec(command, timeout);
			return true;
		}
		catch(Exception e)
		{
			String objNotFound = userNotFound.replace("{USERID}", name);
			if(e.getMessage().contains(objNotFound))
			{
				
				return false;
			}
			else
			{
				throw e;
			}
		}
		
	}

	/**
	 * This method changes the user password
	 * @param name of the user
	 * @param type of the user
	 * @param newPassword
	 * @throws Exception if the user does not exist etc
	 */
	public void changeUserPassword(String name, USER_GROUP_TYPE type, String newPassword) throws Exception
	{
		isConnectedToServer();
		String command = userPassword.replace("{ID}",  name);
		command = command.replace("{TYPE}",  type.toString());
		command = command.replace("{PASSWORD}",  newPassword);
		
		sshExecutor.exec(command, timeout);
		
		
	}
	
	/**
	 * This method can be used to change the users group membership
	 * @param name of the user
	 * @param type of the user
	 * @param remove if true the group will be removed, if false the group will be added
	 * @param group to modify
	 * @throws Exception if the group cannot be modified
	 */
	public void changeUserGroup(String name, USER_GROUP_TYPE type, boolean remove, String group) throws Exception
	{
		isConnectedToServer();
		String command = userGroup.replace("{ID}",  name);
		command = command.replace("{TYPE}",  type.toString());
		if(remove)
		{
			command = command.replace("{ACTION}",  "RemoveGroupMembership");
		}
		else
		{
			command = command.replace("{ACTION}",  "AddGroupMembership");
		}
		command = command.replace("{GROUP}", group);
		sshExecutor.exec(command, timeout);
		
		
	}
	
	/**
	 * This method delete a user
	 * @param name of the user to delete
	 * @param type of user
	 * @throws Exception if the user cannot be deleted i.e. cannot be found
	 */
	public void deleteUser(String name, USER_GROUP_TYPE type) throws Exception
	{
		isConnectedToServer();
		String command = userDelete.replace("{ID}",  name);
		command = command.replace("{TYPE}",  type.toString());
		sshExecutor.exec(command, timeout);
		
	}
	
	/**
	 * This method creates a group
	 * @param name of the group which must be unique within the groups
	 * @param description
	 * @param groupMember if a not null value is passed into this method the new group will be created as a member of this group. 
	 * If this value is not null this groupMember must already exist on the system.
	 * @throws Exception if the group cannot be created
	 */
	public void createGroup(String name, String description, String groupMember)throws Exception
	{
		isConnectedToServer();
		String command = null;
		if(groupMember == null)
		{
			command = createGroup;
		}
		else
		{
			command = createGroupMember.replace("{MEMBER}",  groupMember);
		}
		command = command.replace("{ID}",  name);
		command = command.replace("{DESC}",  description);
	
		sshExecutor.exec(command, timeout);
		
		
	}
	
	/**
	 * This method can be used to change the groups group membership
	 * @param name of the group
	 * @param remove - if true the group will be removed, if false the group will be added
	 * @param group membership to modify
	 * @throws Exception if the group cannot be modified
	 */
	public void changeGroupMember(String name, boolean remove, String group) throws Exception
	{
		isConnectedToServer();
		String command = groupGroupMember.replace("{ID}",  name);
		if(remove)
		{
			command = command.replace("{ACTION}",  "RemoveGroupMembership");
		}
		else
		{
			command = command.replace("{ACTION}",  "AddGroupMembership");
		}
		command = command.replace("{GROUP}", group);
		
		sshExecutor.exec(command, timeout);
		
		
	}
	
	
	/**
	 * This method checks to see if a particular group exists on the system
	 * @param name of the group
	 * @param type of the group
	 * @return true if the group exists; false otherwise
	 * @throws Exception
	 */
	public boolean groupExists(String name, USER_GROUP_TYPE type) throws Exception
	{
		isConnectedToServer();
		String command = groupExists.replace("{ID}", name);
		command = command.replace("{TYPE}", type.toString());
		
		
		try
		{
			sshExecutor.exec(command, timeout);
			return true;
		}
		catch(Exception e)
		{
			String objNotFound = groupNotFound.replace("{GROUPID}", name);
			if(e.getMessage().contains(objNotFound))
			{
				return false;
			}
			else
			{
				throw e;
			}
		}
		
	}
	
	/**
	 * This method delete a group
	 * @param name of the group to delete
	 * @param type of group
	 * @throws Exception if the group cannot be deleted i.e. cannot be found
	 */
	public void deleteGroup(String name) throws Exception
	{
		isConnectedToServer();
		String command = groupDelete.replace("{ID}",  name);
		sshExecutor.exec(command, timeout);
		
	}
	
	/**
	 * This methods deletes a subscription
	 * @param name - the name of the subscription
	 * @param clientId - the client id
	 * @throws Exception is thrown if the subscription name cannot be deleted i.e. cannot be found
	 */
	public void deleteSubscription(String name, String clientId) throws Exception
	{
		isConnectedToServer();
		String command = subDelete.replace("{NAME}",  name);
		command = command.replace("{ID}",  clientId);
		sshExecutor.exec(command, timeout);
	}
	
	/**
	 * This method delete a mqttclient
	 * @param clientId
	 * @throws Exception is thrown if the clientid cannot be found
	 */
	public void deleteMqttClient(String clientId) throws Exception
	{
		isConnectedToServer();
		String command = mqttDelete.replace("{ID}",  clientId);
		sshExecutor.exec(command, timeout);
	}
	
	/**
	 * This method creates an external LDAP configuration
	 * @param options to imaserver create LDAP command
	 * @throws Exception if failed
	 * @return If successful, output of the command
	 */
	public CLICommand createLDAP(Map<String,String> options) throws Exception {
		return executeCommand(ImaCommandReference.CREATE_LDAP, options);
	}

	/**
	 * This method deletes the LDAP configuration object
	 * @param options
	 * @return If successful, output of the command
	 * @throws Exception if failed
	 */
	public CLICommand deleteLDAP(Map<String,String> options) throws Exception {
		return executeCommand(ImaCommandReference.DELETE_LDAP, options);
	}
	
	/**
	 * This method updates the LDAP configuration object
	 * @param options
	 * @return If successful, output of the command
	 * @throws Exception if failed
	 */
	public CLICommand updateLDAP(Map<String,String> options) throws Exception {
		return executeCommand(ImaCommandReference.UPDATE_LDAP, options);
	}
	
	/**
	 * This method shows the LDAP configuration object
	 * @param options
	 * @return If successful, output of the command
	 * @throws Exception if failed
	 */
	public CLICommand showLDAP(Map<String,String> options) throws Exception {
		return executeCommand(ImaCommandReference.SHOW_LDAP, options);
	}

	/**
	 * This method tests the LDAP connection
	 * @param options
	 * @return If successful, output of the command
	 * @throws Exception if failed
	 */
	public CLICommand testLDAP(Map<String,String> options) throws Exception {
		return executeCommand(ImaCommandReference.TEST_LDAP, options);
	}
	
}
