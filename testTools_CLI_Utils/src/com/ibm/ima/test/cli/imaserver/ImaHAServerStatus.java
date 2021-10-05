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
 * This is a helper class for manipulating and determining the status of 
 * a HA pair of imaservers.
 * 
 * NOTE: JSCH LIBRARIES ONLY TO BE USED FOR AUTOMATION
 * 
 *
 */
public class ImaHAServerStatus extends ImaServer {
	
	protected long timeout = 30000;
	private String ipaddressHAA = null;
	private String ipaddressHAB = null;
	private String username = null;
	private String password = null;
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
	private String isPrimary = "(HARole+)(.*)(PRIMARY+)";
	private String isStandby = "(HARole+)(.*)(STANDBY+)";
	private String isStatusUnknown = "(Status+)(.*)(Unknown+)";
	private String isStatusFailover = "(Status+)(.*)(Failover+)";
	private String getMustGather = null;
	private String getStackTrace = null;
	private String getCoreDump = null;
	private String getFileList=null;
	private String stopMqconnectivity=null;
	private String stopForceMqconnectivity=null;
	private String statusMQConnCommand = null;
	private String startMQConnCommand= null;
	private String harole = null;
	private String versionRegEx = "(.*)(version is 1.0.0)(.*)";
	private int checkVersion = -1;
	public boolean setup = true;
	
	/**
	 * Returns an object that can perform simple stop, start, show etc commands on the imaserver
	 * 
	 * @param ipaddressA - the imaserver address for appliance A
	 * @param ipaddressB - the imaserver address for appliance B
	 * @param username - the imaserver administrators username
	 * @param password - the imaserver administrators password
	 */
	public ImaHAServerStatus(String ipA, String ipB, String user, String passwd)
	{	
		super(ipA, ipB, user, passwd);
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
		harole = ImaCommandReference.HAROLE;
		this.username = user;
		this.password = passwd;
		this.ipaddressHAA = ipA;
		this.ipaddressHAB = ipB;
		trace = new Audit("stdout", true);
		
	}
		
	/**
	 * This method attempts to stop the primary imaserver and then once completed start the standby node. 
	 * If the failover completed successfully within the timeout period it will return true otherwise it 
	 * will return false
	 * @param timeout - in milliseconds the length of time we will wait for the 
	 * server to have stopped
	 * @param force - if true the server will be forced to stop
	 * @return true if the server was stopped; false otherwise.
	 */
	public boolean stopServer(long timeout, boolean force) throws Exception
	{
		return stopServer(timeout, force, false, 0);
		
	}
	
	
	/**
	 * This method attempts to stop the primary imaserver and then once completed start the standby node. 
	 * If the failover completed successfully within the timeout period it will return true otherwise it 
	 * will return false
	 * @param timeout - in milliseconds the length of time we will wait for the 
	 * server to have stopped
	 * @param force - if true the server will be forced to stop
	 * @return true if the server was stopped; false otherwise.
	 */
	public boolean stopServer(long timeout, boolean force, boolean retry) throws Exception
	{
		return stopServer(timeout, force, retry, 0);
	}
	
	
	
	/**
	 * This method attempts to stop the primary imaserver and then once completed start the standby node. 
	 * If the failover completed successfully within the timeout period it will return true otherwise it 
	 * will return false
	 * @param timeout - in milliseconds the length of time we will wait for the 
	 * server to have stopped
	 * @param force - if true the server will be forced to stop
	 * @return true if the server was stopped; false otherwise.
	 */
	public boolean stopServer(long timeout, boolean force, boolean retry, long resynchTime) throws Exception
	{
		isConnectedToServer();
		if(setup)
		{
			try
			{
				isVersion100();
				setup=false;
			}
			catch(Exception e)
			{
				throw new RuntimeException("Unable to determine version");
			}
		}

		boolean firstIsPrimary = true;
		// determine which appliance is the primary server
		if(isPrimary(true))
		{
			// do nothing
		}
		else if(isPrimary(false))
		{
			firstIsPrimary = false;
		}
		else
		{
			throw new Exception("None of the appliances were the primary server");
		}
		if(firstIsPrimary)
		{
			if(trace!= null)
			{
				trace.trace("We are going to stop the first machine");
			}
			try
			{
				if(force)
				{
					sshExecutorA.exec(stopForceCommand, timeout);
				}
				else
				{
					sshExecutorA.exec(stopCommand, timeout);
				}
				// 	check if stopped
				String result = sshExecutorA.exec(statusCommand, timeout);
				Pattern p = Pattern.compile(isStoppedRegEx);
				Matcher m = p.matcher(result); // get a matcher object
				if(m.find())
				{
		    	// 	all ok
					if(trace!= null)
					{
						trace.trace("The first machine was stopped correctly");
					}
				}
				else
				{
					if(retry)
					{
						int i=0;					
						do
						{
							Thread.sleep(10000);
							result = sshExecutorA.exec(statusCommand, timeout);
							p = Pattern.compile(isStoppedRegEx);
							m = p.matcher(result); // get a matcher object
							if(trace!= null)
							{
								trace.trace("Has server A stopped="+m.find());
							}
							m.reset();
							i++;
							
						}while((!m.find()) && i<30);
						
						m.reset();
						if(m.find())
						{
							if(trace!= null)
							{
								trace.trace("The server was correctly stopped ");
							}
						}
						else
						{
							if(trace!= null)
							{
								trace.trace("We were unable to stop the appliance A correctly " + result);
							}
							throw new Exception("We were unable to stop the appliance A correctly " + result);
						}
					}
					else
					{
						if(trace!= null)
						{
							trace.trace("We were unable to stop the appliance A correctly " + result);
						}
						throw new Exception("We were unable to stop the appliance A correctly " + result);
					}
				}
			}
			catch(Exception e)
			{
				if(trace!= null)
				{
					trace.trace("An exception occurred when we attempted to stop the first appliance");
				}
				if(retry)
				{
					// 	ok lets retry
					Thread.sleep(10000);
					String result = sshExecutorA.exec(statusCommand, timeout);
					Pattern p = null;
					if(checkVersion == 100)
					{
						p = Pattern.compile(isRunningRegEx100);
					}
					else
					{
						p = Pattern.compile(isRunningRegEx11);
					}
					Matcher m = p.matcher(result); // get a matcher object
					if(m.find())
					{
						if(trace!= null)
						{
							trace.trace("Attempting to stop the server again as it appears to be running");
						}
						if(force)
						{
							sshExecutorA.exec(stopForceCommand, timeout);
						}
						else
						{
							sshExecutorA.exec(stopCommand, timeout);
						}
					}
					
					int i=0;					
					do
					{
						Thread.sleep(10000);
						result = sshExecutorA.exec(statusCommand, timeout);
						p = Pattern.compile(isStoppedRegEx);
						m = p.matcher(result); // get a matcher object
						if(trace!= null)
						{
							trace.trace("Has server A stopped="+m.find());
						}
						m.reset();
						i++;
						
					}while((!m.find()) && i<30);
					
					m.reset();
					if(m.find())
					{
						if(trace!= null)
						{
							trace.trace("The server was correctly stopped ");
						}
					}
					else
					{
						if(trace!= null)
						{
							trace.trace("We were unable to stop the appliance A correctly and this is our final attempt " + result);
						}
						throw new Exception("We were unable to stop the appliance A correctly and this is our final attempt " + result);
					}
				}
				else
				{
					if(trace!= null)
					{
						trace.trace("We were unable to stop the appliance A correctly");
					}
					throw new Exception("We were unable to stop the appliance A correctly");
				}
			}
		    
		    boolean success = false;
		    
		    long endHA = System.currentTimeMillis() + timeout;
		    do
		    {
		    	if(trace!= null)
			   	{
			   		trace.trace("Sleeping before checking to see if standby is up as primary");
			   	}
		    	Thread.sleep(30000);
		    	if(trace!= null)
			   	{
			   		trace.trace("The standby is running as primary=" +  isRunning(false));
			   	}
		    	
		    }while((!isRunning(false)) && (System.currentTimeMillis() < endHA));
		    	
		    if(isRunning(false))
		    {
		    	if(trace!= null)
			   	{
		    		trace.trace("The standby is successfully running as primary");
			   	}
		    	success = true;
		    	
		    }
		    else
		    {
		    	if(trace!= null)
			   	{
		    		trace.trace("The standby did not failover and start to primary in a reasonable time");
			   	}
		    	throw new Exception("The standby did not failover and start to primary in a reasonable time");
		    }
		    				    
		    if(success)
		    {
		    	success = false;
		    	// now lets start the standby back up
		    	if(trace!= null)
		    	{
    				trace.trace("Sleeping for " +resynchTime+" millseconds before resynching");
		    	}
		    	Thread.sleep(resynchTime);
		    	if(trace!= null)
		    	{
    				trace.trace("Attempting to start the standby");
		    	}
		    	sshExecutorA.exec(startCommand, timeout);
		    	long endStart = System.currentTimeMillis() + timeout;
			    do
			    {
			    	if(trace!= null)
			    	{
	    				trace.trace("Standby is back up="+isStandby(true));
			    	}
			    	//  now check the standby is correctly up
			    	if(isStandby(true))
			    	{
			    		if(trace!= null)
				    	{
		    				trace.trace("The standby was started");
				    	}
			    		success = true;
			    		break;
			    		
			    	}
			    	else
			    	{
			    		if(trace!= null)
				    	{
		    				trace.trace("Sleeping before checking to see if the standby is back up");
				    	}
			    		Thread.sleep(30000);
			    		
			    	}
			    	
			    }while((!success) && (System.currentTimeMillis() < endStart));
			    
			    if(success)
			    {
			    	if(trace!= null)
			    	{
	    				trace.trace("Both systems are back up correctly");
			    	}
			    	// all done
			    	return true;
			    }
			    else
			    {
			    	if(trace!= null)
			    	{
	    				trace.trace("The appliance A did not start as standby is a reasonable time");
			    	}
			    	throw new Exception("The appliance A did not start as standby is a reasonable time");
			    }
		    	
		    }
		    else
		    {
		    	if(trace!= null)
		    	{
    				trace.trace("The standby did not failover to primary and or start in a reasonable timee");
		    	}
		    	throw new Exception("The standby did not failover to primary and or start in a reasonable time");
		    }
		}
		else
		{
			try
			{
				if(trace!= null)
				{
					trace.trace("We are going to stop the second machine");
				}
				if(force)
				{
					sshExecutorB.exec(stopForceCommand, timeout);
				}
				else
				{
					sshExecutorB.exec(stopCommand, timeout);
				}
				// check if stopped
				String result = sshExecutorB.exec(statusCommand, timeout);
				Pattern p = Pattern.compile(isStoppedRegEx);
				Matcher m = p.matcher(result); // get a matcher object
				if(m.find())
				{
					// 	all ok
					if(trace!= null)
					{
						trace.trace("The second machine was stopped correctly");
					}
				}
				else
				{
					if(retry)
					{
						int i=0;					
						do
						{
							Thread.sleep(10000);
							result = sshExecutorB.exec(statusCommand, timeout);
							p = Pattern.compile(isStoppedRegEx);
							m = p.matcher(result); // get a matcher object
							if(trace!= null)
							{
								trace.trace("Has server B stopped=" +m.find());
							}
							m.reset();
							i++;
							
						}while((!m.find()) && i<30);
						
						m.reset();
						if(m.find())
						{
							if(trace!= null)
							{
								trace.trace("The server was correctly stopped");
							}
						}
						else
						{
							if(trace!= null)
							{
								trace.trace("We were unable to stop the appliance B correctly " + result);
							}
							throw new Exception("We were unable to stop the appliance B correctly " + result);
						}
					}
					else
					{
						if(trace!= null)
						{
							trace.trace("We were unable to stop the appliance B correctly " + result);
						}
						throw new Exception("We were unable to stop the appliance B correctly " + result);	
					}
				}
			}
			catch(Exception e)
			{
				if(trace!= null)
				{
					trace.trace("An exception occurred when we attempted to stop the second appliance");
				}
				if(retry)
				{
					// 	ok lets retry
					Thread.sleep(10000);
					String result = sshExecutorB.exec(statusCommand, timeout);
					Pattern p = null;
					if(checkVersion == 100)
					{
						p = Pattern.compile(isRunningRegEx100);
					}
					else
					{
						p = Pattern.compile(isRunningRegEx11);
					}
					Matcher m = p.matcher(result); // get a matcher object
					if(m.find())
					{
						if(trace!= null)
						{
							trace.trace("Attempting to stop the server again as it appears to be running");
						}
						if(force)
						{
							sshExecutorB.exec(stopForceCommand, timeout);
						}
						else
						{
							sshExecutorB.exec(stopCommand, timeout);
						}
					}
					
					int i=0;					
					do
					{
						
						Thread.sleep(10000);
						result = sshExecutorB.exec(statusCommand, timeout);
						p = Pattern.compile(isStoppedRegEx);
						m = p.matcher(result); // get a matcher object
						if(trace!= null)
						{
							trace.trace("Has server B stopped="+m.find());
						}
						m.reset();
						i++;
						
					}while((!m.find()) && i<30);
					
					m.reset();
					if(m.find())
					{
						if(trace!= null)
						{
							trace.trace("The server was correctly stopped");
						}
					}
					else
					{
						if(trace!= null)
						{
							trace.trace("We were unable to stop the appliance B correctly and this is our final attempt " + result);
						}
						throw new Exception("We were unable to stop the appliance B correctly and this is our final attempt " + result);
					}
				}
			}
		
		    boolean success = false;
		    long endHA = System.currentTimeMillis() + timeout;
		    do
		    {
		    	if(trace!= null)
			   	{
			   		trace.trace("Sleeping before checking to see if standby is up as primary");
			   	}
		    	Thread.sleep(30000);
		    	if(trace!= null)
			   	{
			   		trace.trace("The standby is running as primary=" +  isRunning(true));
			   	}
		    			
		    }while((!isRunning(true)) && (System.currentTimeMillis() < endHA));
		    		
			if(isRunning(true))
			{
			  	if(trace!= null)
			   	{
			   		trace.trace("The standby is successfully running as primary");
			   	}
			  	success = true;
			
			}
			else
		  	{
			   	if(trace!= null)
			   	{
			   		trace.trace("The standby did not failover and start to primary in a reasonable time");
				}
			   	throw new Exception("The standby did not failover and start to primary in a reasonable time");
			}
		    	
		   
		   
		    if(success)
		    {
		    	success = false;
		    	// now lets start the standby back up
		    	if(trace!= null)
		    	{
    				trace.trace("Sleeping for " +resynchTime+" millseconds before resynching");
		    	}
		    	Thread.sleep(resynchTime);
		    	if(trace!= null)
		    	{
    				trace.trace("Attempting to start the standby");
		    	}
		    	sshExecutorB.exec(startCommand, timeout);
		    	long endStart = System.currentTimeMillis() + timeout;
			    do
			    {
			    	if(trace!= null)
			    	{
	    				trace.trace("Standby is back up="+isStandby(false));
			    	}
			    	//  now check the standby is correctly up
			    	if(isStandby(false))
			    	{
			    		if(trace!= null)
				    	{
		    				trace.trace("The standby was started");
				    	}
			    		success = true;
			    		break;
			    		
			    	}
			    	else
			    	{
			    		if(trace!= null)
				    	{
		    				trace.trace("Sleeping before checking to see if the standby is back up");
				    	}
			    		Thread.sleep(30000);
			    	}
			    	
			    }while((!success) && (System.currentTimeMillis() < endStart));
			    
			    if(success)
			    {
			    	if(trace!= null)
			    	{
	    				trace.trace("Both systems are back up correctly");
			    	}
			    	// all done
			    	return true;
			    }
			    else
			    {
			    	if(trace!= null)
			    	{
	    				trace.trace("The appliance B did not start as standby is a reasonable time");
			    	}
			    	throw new Exception("The appliance B did not start as standby is a reasonable time");
			    }
		    	
		    }
		    else
		    {
		    	if(trace!= null)
		    	{
    				trace.trace("The standby did not failover to primary and or start in a reasonable time");
		    	}
		    	throw new Exception("The standby did not failover to primary and or start in a reasonable time");
		    }
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
	 * 
	 * @returns true if mqconnectivity is stopped; false otherwise
	 * @throws Exception
	 */
	public boolean isMQConnectivityStopped() throws Exception
	{
		isConnectedToServer();
		return isMQConnStopped();
	}
	
	
	/*
	 * This method will return true if the imaserver is stopped.
	 * @return true if stopped; false otherwise
	 */
	private boolean isMQConnStopped() throws Exception
	{
		String result = null;
		if(isPrimary(true))
		{
			result = sshExecutorA.exec(statusMQConnCommand, timeout);
		}
		else
		{
			result = sshExecutorB.exec(statusMQConnCommand, timeout);
		}
		Pattern p = Pattern.compile(isStoppedRegEx);
	    Matcher m = p.matcher(result); // get a matcher object
	    return m.find();
	}
	
	private void stopMQConnCommand(boolean force) throws Exception
	{
		if(isPrimary(true))
		{
			if(force)
			{
				sshExecutorA.exec(stopForceMqconnectivity, timeout);
			}
			else
			{
				sshExecutorA.exec(stopMqconnectivity, timeout);
			}
		}
		else
		{
			if(force)
			{
				sshExecutorB.exec(stopForceMqconnectivity, timeout);
			}
			else
			{
				sshExecutorB.exec(stopMqconnectivity, timeout);
			}
		}
		
		
	}
	
	private boolean isRunning(boolean firstAppliance) throws Exception
	{
		if(firstAppliance)
		{
			String result = sshExecutorA.exec(statusCommand, timeout);
			Pattern p = null;
			if(checkVersion == 100)
			{
				p = Pattern.compile(isRunningRegEx100);
			}
			else
			{
				p = Pattern.compile(isRunningRegEx11);
			}
    		Matcher m = p.matcher(result);
		    return m.find();
		}
		else
		{
			String result = sshExecutorB.exec(statusCommand, timeout);
			Pattern p = null;
			if(checkVersion == 100)
			{
				p = Pattern.compile(isRunningRegEx100);
			}
			else
			{
				p = Pattern.compile(isRunningRegEx11);
			}
    		Matcher m = p.matcher(result);
		    return m.find();
		}
	}
	
	
	private void isVersion100() throws Exception
	{
		String result = null;
		if(isPrimary(true))
		{
			result = sshExecutorA.exec(showServerCommand, timeout);
		}
		else
		{
			result = sshExecutorB.exec(showServerCommand, timeout);
		}
		Pattern p = Pattern.compile(versionRegEx);
	    Matcher m = p.matcher(result); // get a matcher object
	    if(m.find())
	    {
	    	checkVersion = 100;
	    }
	    else
	    {
	    	checkVersion = 100;
	    }
	   
	}
	
	
	private boolean isPrimary(boolean firstAppliance) throws Exception
	{
		if(firstAppliance)
		{
			String result = sshExecutorA.exec(statusCommand, timeout);
			Pattern p = Pattern.compile(isPrimary);
		    Matcher m = p.matcher(result); // get a matcher object
		    return m.find();
		}
		else
		{
			String result = sshExecutorB.exec(statusCommand, timeout);
			Pattern p = Pattern.compile(isPrimary);
		    Matcher m = p.matcher(result); // get a matcher object
		    return m.find();
		}
	}
	
	
	private boolean isStandby(boolean firstAppliance) throws Exception
	{
		if(firstAppliance)
		{
			String result = sshExecutorA.exec(statusCommand, timeout);
			Pattern p = Pattern.compile(isStandby);
		    Matcher m = p.matcher(result); // get a matcher object
		    return m.find();
		}
		else
		{
			String result = sshExecutorB.exec(statusCommand, timeout);
			Pattern p = Pattern.compile(isStandby);
		    Matcher m = p.matcher(result); // get a matcher object
		    return m.find();
		}
	}


	public String gatherCoreDump(String component) throws Exception {
		// TODO Auto-generated method stub
		return null;
	}


	public void gatherDebug(String fileName) throws Exception {
		// TODO Auto-generated method stub
		
	}


	public String gatherStackTrace(String component) throws Exception {
		// TODO Auto-generated method stub
		return null;
	}


	public String[] getFileNames() throws Exception {
		// TODO Auto-generated method stub
		return null;
	}


	public boolean isMQConnectivityRunning() throws Exception {
		// TODO Auto-generated method stub
		return false;
	}


	public boolean isServerRunning() throws Exception {
		
		return false;
		
	}


	public boolean isServerStopped() throws Exception {
		
		return false;
	}


	public String showDetails() throws Exception {
		// TODO Auto-generated method stub
		return null;
	}


	public String showStatus() throws Exception {
		// TODO Auto-generated method stub
		return null;
	}


	public void startMQConnectivity() throws Exception {
		// TODO Auto-generated method stub
		
	}


	public boolean startServer(long timeout) throws Exception {
		// TODO Auto-generated method stub
		return false;
	}


	public void startServer() throws Exception {
		// TODO Auto-generated method stub
		
	}


	public void stopForceServer() throws Exception {
		// TODO Auto-generated method stub
		
	}


	public void stopServer() throws Exception {
		// TODO Auto-generated method stub


	}
	
	/*
	 * This method will return true if the imaserver is in maintenance
	 */
	public boolean isServerInMaintenance() throws Exception {
		
		String result = sshExecutorA.exec(statusCommand, timeout);
		Pattern p = Pattern.compile(isRunningMaintenance);
		Matcher m = p.matcher(result); // get a matcher object
		
		if(m.find())
		{
			return true;
		}
		else
		{
			result = sshExecutorB.exec(statusCommand, timeout);
			p = Pattern.compile(isRunningMaintenance);
			m = p.matcher(result); 
			if(m.find())
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	
	
	/*
	 * This method will return true if the imaserver is in an unknown state
	 */
	public boolean isStatusUnknown() throws Exception {
		
		if(isStandby(true))
		{
			String result = sshExecutorB.exec(statusCommand, timeout);
			Pattern p = Pattern.compile(isStatusUnknown);
			Matcher m = p.matcher(result); // get a matcher object
			return m.find();
		}
		else
		{
			String result = sshExecutorA.exec(statusCommand, timeout);
			Pattern p = Pattern.compile(isStatusUnknown);
			Matcher m = p.matcher(result); // get a matcher object
			return m.find();
		}
	}
	
	/*
	 * This method will return true if one server is stopped but the other is failing over
	 */
	public boolean isStatusFailover() throws Exception {
		
		String result = sshExecutorA.exec(statusCommand, timeout);
		Pattern p = Pattern.compile(isStatusFailover);
		Matcher m = p.matcher(result); // get a matcher object
		if(m.find())
		{
			return true;
		}
		else
		{
			result = sshExecutorB.exec(statusCommand, timeout);
			p = Pattern.compile(isStatusFailover);
			m = p.matcher(result); // get a matcher object
			return m.find();
		}
	}

}
