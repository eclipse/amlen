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
package com.ibm.ima.test.cli.imaserver;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.ibm.ima.test.cli.util.SSHExec;
import com.jcraft.jsch.JSchException;

/**
 * This is the base class for the CLI Helper classes.
 * 
 *
 */
public abstract class ImaCliHelper {
	
	protected SSHExec sshExecutorA = null;
	protected SSHExec sshExecutorB = null;
	protected SSHExec sshExecutor = null;
	protected boolean hAConfiguration = false;
	protected long timeout = 30000;
	private boolean passwordless = false;
	protected Audit trace = null;
	

	public ImaCliHelper(String ipaddress, String username, String password)
	{		
		sshExecutorA = new SSHExec(ipaddress, username, password);
		if(password == null)
		{
			passwordless = true;
		}
		
	}
	
	
	public ImaCliHelper(String ipaddressA, String ipaddressB, String username, String password)
	{		
		sshExecutorA = new SSHExec(ipaddressA, username, password);
		sshExecutorB = new SSHExec(ipaddressB, username, password);
		hAConfiguration = true;
		if(password == null)
		{
			passwordless = true;
		}
		
	}
	
	/**
	 * This method must be called before any commands can be executed
	 * @throws Exception if we cannot connect to the server
	 */
	public void connectToServer() throws Exception
	{
		try
		{
			if(passwordless)
			{
				sshExecutorA.connectPasswordless();
			}
			else
			{
				sshExecutorA.connect();
			}
			if(sshExecutorB != null)
			{
				if(passwordless)
				{
					sshExecutorB.connectPasswordless();
				}
				else
				{
					sshExecutorB.connect();
				}
			}
		}
		catch(JSchException e)
		{
			throw new Exception("We were unable to connect to the server: Check that the ipaddress, username and password is correct" + e);
		}
	}
	
	/**
	 * This method must be called to clean up and close the connection to the server
	 */
	public void disconnectServer()
	{
		try
		{
			sshExecutorA.disconnect();
		}
		catch(Exception e)
		{
			// do nothing
		}
		if(sshExecutorB != null)
		{
			try
			{
				sshExecutorB.disconnect();
			}
			catch(Exception e)
			{
				// do nothing
			}
		}
		
	}
	
	/**
	 * Returns true if connected to the server; false otherwise
	 * @return
	 */
	public boolean isConnected()
	{
		if(sshExecutorB != null)
		{
			return (sshExecutorA.isConnected() && sshExecutorB.isConnected());
		}
		else
		{
			return sshExecutorA.isConnected();
		}
	}
	
	
	public long getTimeout() {
		return timeout;
	}

	public void setTimeout(long timeout) {
		this.timeout = timeout;
	}
	
	
	protected void isConnectedToServer() throws Exception
	{
		if(sshExecutorB != null)
		{
			if(sshExecutorA.isConnected() && sshExecutorB.isConnected())
			{
				// do nothing
				validateHA();
			}
			else
			{
				throw new Exception("No connection made to server - please call connectToServer() to open a connection before executing config methods");
			}
		}
		else
		{
			if(sshExecutorA.isConnected())
			{
				sshExecutor = sshExecutorA;
			}
			else
			{
				throw new Exception("No connection made to server - please call connectToServer() to open a connection before executing config methods");
			}
		}
		
	}
	
	protected void validateHA() throws Exception
	{
		if(! hAConfiguration)
		{
			// do nothing 
			
		}
		else
		{
			String result = sshExecutorA.exec(ImaCommandReference.STATUS_COMMAND, timeout);
			String isPrimary = "(HARole+)(.*)(PRIMARY+)";
			Pattern p = Pattern.compile(isPrimary);
			Matcher m = p.matcher(result);
			if(m.find())
			{
				sshExecutor = sshExecutorA;
			}
			else
			{
				result = sshExecutorB.exec(ImaCommandReference.STATUS_COMMAND, timeout);
				p = Pattern.compile(isPrimary);
				m = p.matcher(result);
				if(m.find())
				{
					sshExecutor = sshExecutorB;
				}
				else
				{
					throw new Exception("In a HA configuration but neither system is up and running as the primary server");
				}
			}
		}
	}

}
