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


/**
 * This is a helper class for manipulating and determining the status of the imaserver.
 * 
 * NOTE: JSCH LIBRARIES ONLY TO BE USED FOR AUTOMATION
 * 
 *
 */
public class ImaServerStatus extends ImaServer {

	private String stopCommand = null;
	private String stopForceCommand = null;
	private String startCommand = null;
	private String showCommand = null;
	private String showServerCommand = null;
	private String statusCommand = null;
	private String isStoppedRegEx = "(Status+)(.*)(Stopped+)";
	private String isRunningRegEx100 = "(Status+)(.*)(Running+)";
	private String isRunningRegEx11 = "(Status+)(.*)(Running+)(.*)(production+)";
	private String isRunningMaintenance = "(Status+)(.*)(Running+)(.*)(maintenance+)";
	private String isStatusUnknown = "(Status+)(.*)(Unknown+)";
	private String getMustGather = null;
	private String getStackTrace = null;
	private String getCoreDump = null;
	private String getFileList=null;
	private String stopMqconnectivity=null;
	private String stopForceMqconnectivity=null;
	private String statusMQConnCommand = null;
	private String startMQConnCommand= null;
	private String versionRegEx = "(.*)(version is 1.0.0)(.*)";
	private int checkVersion = -1;

	
	
	/**
	 * Returns an object that can perform simple stop, start, show etc commands on the imaserver
	 * 
	 * @param ipaddress - the imaserver address
	 * @param username - the imaserver administrators username
	 * @param password - the imaserver administrators password
	 */
	public ImaServerStatus(String ipaddress, String username, String password)
	{		
		super(ipaddress, username, password);
		// Set up all the commands
		stopCommand = ImaCommandReference.STOP_COMMAND;
		stopForceCommand = ImaCommandReference.STOP_FORCE_COMMAND;
		startCommand = ImaCommandReference.START_COMMAND;
		showCommand = ImaCommandReference.SHOW_COMMAND;
		statusCommand = ImaCommandReference.STATUS_COMMAND;
		getMustGather = ImaCommandReference.MUST_GATHER;
		getStackTrace = ImaCommandReference.STACK_TRACE;
		getCoreDump = ImaCommandReference.DUMP_CORE;
		getFileList = ImaCommandReference.GET_FILE_LIST;
		stopMqconnectivity = ImaCommandReference.STOP_MQCONN;
		stopForceMqconnectivity = ImaCommandReference.STOP_FORCE_MQCONN;
		statusMQConnCommand = ImaCommandReference.MQCONN_STATUS_COMMAND;
		startMQConnCommand = ImaCommandReference.START_MQCONN_COMMAND;
		showServerCommand = ImaCommandReference.SHOW_SERVER_COMMAND;
		
	}
	
	/**
	 * This method attempts to stop the imaserver. If the server is stopped
	 * within the timeout period it will return true otherwise it will return
	 * false
	 * @param timeout - in milliseconds the length of time we will wait for the 
	 * server to have stopped
	 * @param force - if true the server will be forced to stop
	 * @return true if the server was stopped; false otherwise.
	 */
	public boolean stopServer(long timeout, boolean force) throws Exception
	{
		isConnectedToServer();
		if(isStopped())
		{
			return true;
		}
		
		stopServerCommand(force);
		
		final long until = System.currentTimeMillis() + timeout;
	    
		while (!isStopped() && System.currentTimeMillis() < until)
		{
			Thread.sleep(1000);
		}
		if(!isStopped())
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	
	/**
	 * This method will call the stop server command.
	 */
	public void stopServer() throws Exception
	{
		isConnectedToServer();
		if(isStopped())
		{
			return;
		}
		else
		{
			stopServerCommand(false);
		}
	}
	
	/**
	 * This method will call the forced stop server command.
	 */
	public void stopForceServer() throws Exception
	{
		isConnectedToServer();
		if(isStopped())
		{
			return;
		}
		else
		{
			stopServerCommand(true);
		}
	}
	
	/**
	 * This method will call the stop mqconnectivity command.
	 */
	public void stopMQConnectivity() throws Exception
	{
		isConnectedToServer();
		if(isMQConnStopped())
		{
			return;
		}
		else
		{
			stopMQConnCommand(false);
		}
	}
	
	
	
	/**
	 * This method will call the forced stop mqconnectivity command.
	 */
	public void stopForceMQConnectivity() throws Exception
	{
		isConnectedToServer();
		if(isMQConnStopped())
		{
			return;
		}
		else
		{
			stopMQConnCommand(true);
		}
	}
	
	
	
	
	/**
	 * This method attempts to start the imaserver. If the server is started
	 * within the timeout period it will return true otherwise it will return
	 * false
	 * @param timeout;
	 * @return true if the server was started; false otherwise.
	 * @throws Exception if the server cannot be started in the specified time
	 */
	public boolean startServer(long timeout) throws Exception
	{
		isConnectedToServer();
		if(isRunning())
		{
			return true;
		}
		
		startServerCommand();
		
		final long until = System.currentTimeMillis() + timeout;
	    
		while (!isRunning() && System.currentTimeMillis() < until)
		{
			Thread.sleep(1000);
		}
		if(!isRunning())
		{
			throw new Exception("The server could not be started in the specified time");
		}
		else
		{
			return true;
		}
	}
	
	/**
	 * This method will call the start server command.
	 */
	public void startServer() throws Exception
	{
		isConnectedToServer();
		if(isRunning())
		{
			return;
		}
		
		startServerCommand();
	}
	
	
	/**
	 * This method will call the start mqconnectivity command.
	 */
	public void startMQConnectivity() throws Exception
	{
		isConnectedToServer();
		if(isMQConnRunning())
		{
			return;
		}
		else
		{
			startMQConnCommand();
		}
	}
	
	/**
	 * This method returns the status of the imaserver
	 * @return the imaserver status
	 * @throws Exception
	 */
	public String showStatus() throws Exception
	{
		isConnectedToServer();
		return statusCommand();
	}
	
	
	/**
	 * This method returns the details of the imaserver build
	 * @return the build details
	 * @throws Exception
	 */
	public String showDetails() throws Exception
	{
		isConnectedToServer();
		return showCommand();
	}
	
	/**
	 * 
	 * @returns true if the server is running; false otherwise
	 * @throws Exception
	 */
	public boolean isServerRunning() throws Exception
	{
		isConnectedToServer();
		return isRunning();
	}
	
	/**
	 * 
	 * @returns true if the server is stopped; false otherwise
	 * @throws Exception
	 */
	public boolean isServerStopped() throws Exception
	{
		isConnectedToServer();
		return isStopped();
	}
	
	/**
	 * 
	 * @returns true if mqconnectivity is stopped; false otherwise
	 * @throws Exception
	 */
	public boolean isMQConnectivityStopped() throws Exception
	{
		isConnectedToServer();
		return isMQConnStopped();
	}
	
	
	/**
	 * 
	 * @returns true if mqconnectivity is running; false otherwise
	 * @throws Exception
	 */
	public boolean isMQConnectivityRunning() throws Exception
	{
		isConnectedToServer();
		return isMQConnRunning();
	}
	
	
	/**
	 * This method is used to execute the must-gather
	 * on the server
	 * 
	 * @param fileName - the name of the must-gather file i.e. my-mustgather.tgz
	 */
	public void gatherDebug(String fileName) throws Exception
	{
		isConnectedToServer();
		String command = getMustGather.replace("{MUST_GATHER}", fileName);
		sshExecutor.exec(command, timeout);
		
	}
	
	
	/**
	 * This method is used to execute the dumpStack command
	 * on the server
	 * 
	 * @param component - the name of the component you wish to get - if null imaserver will be used
	 */
	public String gatherStackTrace(String component) throws Exception
	{
		isConnectedToServer();
		String command = null;
		if(component == null)
		{
			command = getStackTrace.replace("{COMPONENT}", "imaserver");
		}
		else
		{
			command = getStackTrace.replace("{COMPONENT}", component);
		}
		return sshExecutor.exec(command, timeout);
			
	}
	
	/**
	 * This method is used to execute the dumpcore command
	 * on the server
	 * 
	 * @param component - the name of the component you wish to get - if null imaserver will be used
	 */
	public String gatherCoreDump(String component) throws Exception
	{
		isConnectedToServer();
		String command = null;
		if(component == null)
		{
			command = getCoreDump.replace("{COMPONENT}", "imaserver");
		}
		else
		{
			command = getCoreDump.replace("{COMPONENT}", component);
		}
		return sshExecutor.exec(command, timeout);
		
	}
	
	
	/**
	 * This method is used to return all names of the debug files on the system
	 * 
	 */
	public String[] getFileNames() throws Exception
	{
		
		isConnectedToServer();
		String result = sshExecutor.exec(getFileList, timeout);
		String[] elements = result.split(",");
		
		return elements;
		
	}
	
	
	private void stopServerCommand(boolean force) throws Exception
	{
		if(force)
		{
			sshExecutor.exec(stopForceCommand, timeout);
		}
		else
		{
			sshExecutor.exec(stopCommand, timeout);
		}
		
	}
	
	
	private void stopMQConnCommand(boolean force) throws Exception
	{
		if(force)
		{
			sshExecutor.exec(stopForceMqconnectivity, timeout);
		}
		else
		{
			sshExecutor.exec(stopMqconnectivity, timeout);
		}
		
	}
	
	private void startServerCommand() throws Exception
	{
		
		sshExecutor.exec(startCommand, timeout);
		
	}
	
	
	private void startMQConnCommand() throws Exception
	{
		sshExecutor.exec(startMQConnCommand, timeout);
		
	}
	
	private String statusCommand() throws Exception
	{
		return sshExecutor.exec(statusCommand, timeout);
		
	}
	
	private String statusMQConnCommand() throws Exception
	{
		return sshExecutor.exec(statusMQConnCommand, timeout);
	}
	
	private String showCommand() throws Exception
	{
		return sshExecutor.exec(showCommand, timeout);
		
	}


	/*
	 * This method will return true if the imaserver is stopped.
	 * @return true if stopped; false otherwise
	 */
	private boolean isStopped() throws Exception
	{
		String result = statusCommand();
		Pattern p = Pattern.compile(isStoppedRegEx);
	    Matcher m = p.matcher(result); // get a matcher object
	    return m.find();
	}
	

	/*
	 * This method will return true if the imaserver is running.
	 * @return true if started; false otherwise
	 */
	private boolean isRunning() throws Exception
	{
		String result = statusCommand();
		Pattern p = null;
		if(checkVersion == -1)
		{
			checkVersion = this.isVersion100();
		}
		if(checkVersion == 100)
		{
			p = Pattern.compile(isRunningRegEx100);
		}
		else
		{
			p = Pattern.compile(isRunningRegEx11);
		}
		
	    Matcher m = p.matcher(result); // get a matcher object
	    return m.find();
	}
	

	/*
	 * This method will return true if the imaserver is in maintenance
	 */
	public boolean isServerInMaintenance() throws Exception {
		
		String result = statusCommand();
		Pattern p = Pattern.compile(isRunningMaintenance);
	    Matcher m = p.matcher(result); // get a matcher object
	    return m.find();
	}
	
	/*
	 * This is not implemented for standalone servers
	 */
	public boolean isStatusFailover() throws Exception {
		
		return false;
	}
	
	/*
	 * This method will return true if the imaserver is in an unknown state
	 */
	public boolean isStatusUnknown() throws Exception {
		
		String result = statusCommand();
		Pattern p = Pattern.compile(isStatusUnknown);
	    Matcher m = p.matcher(result); // get a matcher object
	    return m.find();
	}
	
	
	/*
	 * This method will return true if the imaserver is stopped.
	 * @return true if stopped; false otherwise
	 */
	private boolean isMQConnStopped() throws Exception
	{
		String result = statusMQConnCommand();
		Pattern p = Pattern.compile(isStoppedRegEx);
	    Matcher m = p.matcher(result); // get a matcher object
	    return m.find();
	}
	

	/*
	 * This method will return true if the imaserver is running.
	 * @return true if started; false otherwise
	 */
	private boolean isMQConnRunning() throws Exception
	{
		String result = statusMQConnCommand();
		Pattern p = Pattern.compile(isRunningRegEx100);
	    Matcher m = p.matcher(result); // get a matcher object
	    return m.find();
	}

	
	public boolean stopServer(long timeout, boolean force, boolean retry)
			throws Exception {
		
		isConnectedToServer();
		if(isStopped())
		{
			return true;
		}
		else
		{
			try
			{
				stopServerCommand(force);
				return true;
			}
			catch(Exception e)
			{
				Thread.sleep(10000);
				if(isStopped())
				{
					return true;
				}
				try
				{
					stopServerCommand(force);
					return true;
				}
				catch(Exception ef)
				{
					return false;
					
				}
				
			}
			
		}
	}
	
	/**
	 * The recoveryTime is not valid for single server so ignore
	 */
	public boolean stopServer(long timeout, boolean force, boolean retry, long recoveryTime)
	throws Exception 
	{
		return stopServer(timeout, force, retry);
	}
		
	
	
	private int isVersion100() throws Exception
	{
		isConnectedToServer();
		String result = sshExecutor.exec(showServerCommand, timeout);
		Pattern p = Pattern.compile(versionRegEx);
	    Matcher m = p.matcher(result); // get a matcher object
	    if(m.find())
	    {
	    	return 100;
	    }
	    else
	    {
	    	return 11;
	    }
	   
	}

	

	

	

}
