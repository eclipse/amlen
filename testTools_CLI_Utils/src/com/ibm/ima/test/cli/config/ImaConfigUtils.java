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

import com.ibm.ima.test.cli.imaserver.ImaCliHelper;
import com.ibm.ima.test.cli.imaserver.ImaCommandReference;


/**
 * Base class for helper methods for ims server commands
 * 
 * NOTE: JSCH LIBRARIES ONLY TO BE USED FOR AUTOMATION
 *
 */
public class ImaConfigUtils extends ImaCliHelper{
		
	protected String showCommand = null;
	protected String notFound = null;
	protected String deleteCommand = null;
	protected String updateCommand = null;
	
	/**
	 * Basic object for imaserver commands
	 * @param ipaddress - the imaserver address
	 * @param username - the imaserver administrators username
	 * @param password - the imaserver administrators password
	 * @throws Exception if the properties file for the command list cannot be found
	 */
	public ImaConfigUtils(String ipaddress, String username, String password) throws Exception
	{
		super(ipaddress, username, password);
			
		showCommand = ImaCommandReference.SHOW_COMMAND;
		deleteCommand = ImaCommandReference.DELETE_COMMAND;
		updateCommand = ImaCommandReference.UPDATE_COMMAND;
		notFound = ImaCommandReference.NOT_FOUND;
	}
	
	/**
	 * Basic object for imaserver commands
	 * @param ipaddressA - the imaserver address for the first HA appliance
	 * @param ipaddressB - the imaserver address for the second HA appliance
	 * @param username - the imaserver administrators username
	 * @param password - the imaserver administrators password
	 * @throws Exception if the properties file for the command list cannot be found
	 */
	public ImaConfigUtils(String ipaddressA, String ipaddressB, String username, String password) throws Exception
	{
		super(ipaddressA, ipaddressB, username, password);
			
		showCommand = ImaCommandReference.SHOW_COMMAND;
		deleteCommand = ImaCommandReference.DELETE_COMMAND;
		updateCommand = ImaCommandReference.UPDATE_COMMAND;
		notFound = ImaCommandReference.NOT_FOUND;
	}
	
	
	
	/*
	 * @param type of config object to delete
	 * @param name of object
	 * @return true if found; false otherwise
	 * @throws Exception will be thrown if the object is in use and cannot be deleted
	 */
	protected void deleteObject(String type, String objectname) throws Exception
	{
		String command = deleteCommand.replace("{NAME}", objectname);
		command = command.replace("{TYPE}", type);
		
		sshExecutor.exec(command, timeout);
		
		
	}
	
	
	/*
	 * @param type of config object to update
	 * @param name of parameter which will be updated
	 * @param value of the parameter
	 * @throws Exception will be thrown if the object cannot be found
	 */
	protected void updateObject(String type, String name, String parameter, String value) throws Exception
	{
		String command = updateCommand.replace("{TYPE}", type);
		command = command.replace("{NAME}", name);
		command = command.replace("{PARAM}", parameter);
		command = command.replace("{VALUE}", value);
		
		sshExecutor.exec(command, timeout);
		
	}
	

	
	/*
	 * 
	 * @param type of config object to find
	 * @param name of object
	 * @return true if found; false otherwise
	 * @throws Exception
	 */
	protected boolean doesObjectExist(String type, String objectname) throws Exception
	{
		String command = showCommand.replace("{NAME}", objectname);
		command = command.replace("{TYPE}", type);
	
		try
		{
			sshExecutor.exec(command, timeout);
			return true;
		}
		catch(Exception e)
		{
			if(e.getMessage().contains(objectname) && e.getMessage().contains(notFound) )
			{
				return false;
			}
			else
			{
				throw e;
			}
		}
	
	}

}
