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
package com.ibm.ims.svt.client.tests.users;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ims.svt.clients.Trace;
import com.ibm.ima.test.cli.config.ImaConfig.USER_GROUP_TYPE;
import com.ibm.ima.test.cli.imaserver.ImaServerStatus;

/* This class creates a new WebUI user, then stops and starts the IMA server as 
 * the new user.
*/

public class CreateUserStopStartServer {
	
	protected String prefix = "TestQueueConnections"; // default but each class can use a different one
	protected String port = "16300"; // default but each class can use a different one
	protected boolean recoverable = false; // default but each class can use a different one
	protected long timeToRun = 1800000; // default but each class can override this one
	protected int interval = 2000; // default but each class can override this one
	protected int numMessages = 1000000000; // default but each class can override this one
	protected long catchUpTime = 300000;
	
	private String username = "admin";
	private String password = "admin";
	private String newUsername = "admin2";
	private String newPassword = "admin2";
	private String newUserType = "WebUI";
	private String newUserGroupMembership = "SystemAdministrators";
	
	private USER_GROUP_TYPE userGroupType = ImaConfig.USER_GROUP_TYPE.WebUI;
	private String ipaddress = null;
	private ImaConfig imaConfig = null;
	private ImaServerStatus imaServerStatus = null;
	
	
	@Before
	public void setUp() throws Exception {
		
		ipaddress = System.getProperty("ipaddress");
		
		if(ipaddress == null)
		{
			System.out.println("Please pass in as system parameters the 'ipaddress' of the server. For example: -Dipaddress=9.3.177.15");
			System.exit(0);
		}
		
		//Username and password (default admin)
		String temp = System.getProperty("username");
		if(temp != null)
		{
			username = temp;
		}
		temp = System.getProperty("password");
		if(temp != null)
		{
			password = temp;
		}
		
		//Username and password for the new user
		temp = System.getProperty("newUsername");
		if(temp != null)
		{
			newUsername = temp;
		}
		temp = System.getProperty("newPassword");
		if(temp != null)
		{
			newPassword = temp;
		}
		
		//Get user type from text input (needs more validation?)
		if (System.getProperty("newUserType") != null){
			if (newUserType.equals("WebUI")){
				System.out.println("WEBUI");
				userGroupType = ImaConfig.USER_GROUP_TYPE.WebUI;
			} else if (newUserType.equals("Messaging")){
				userGroupType = ImaConfig.USER_GROUP_TYPE.Messaging;
			}
		}
		
		temp = System.getProperty("newUserGroupMembership");
		if(temp != null)
		{
			newUserGroupMembership = temp;
		}
		
		new Trace("stdout", true); //debugging
		
		//If the user already exists, delete it.
		imaConfig = new ImaConfig(ipaddress, username, password);
		imaConfig.connectToServer();
		if (imaConfig.userExists(newUsername, userGroupType)){
			imaConfig.deleteUser(newUsername, userGroupType);
		}
	}
	
	
	@Test
	public void testQueueConnections() throws Exception
	{
		//Create the new WebUI user and disconnect as 'admin'
		imaConfig.createUser(newUsername, newPassword, userGroupType, newUserGroupMembership, "Testing2");
		imaConfig.disconnectServer();
		
		//Establish new connection with new user
		imaServerStatus = new ImaServerStatus(ipaddress, newUsername, newPassword);
		imaServerStatus.connectToServer();
		
		//Stop and start the server
		imaServerStatus.stopServer();
		imaServerStatus.startServer();
		
		imaServerStatus.disconnectServer();
	}
	
	@After
	public void tearDown() throws Exception {
		
	}

}
