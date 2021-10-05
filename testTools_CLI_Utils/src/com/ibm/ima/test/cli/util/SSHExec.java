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

package com.ibm.ima.test.cli.util;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

import com.jcraft.jsch.ChannelExec;
import com.jcraft.jsch.JSch;
import com.jcraft.jsch.JSchException;
import com.jcraft.jsch.Session;

/**
 * Utility class that may be used to execute SSH commands. The instance
 * will keep an SSH session active until explicitly closed. A specific
 * command may be executed or a set of commands within a Properties 
 * instance identified by a prefix may be executed.
 *
 */
public class SSHExec {

	// Known automation variables....
	private static final String M1_IPv4_1 = System.getProperty("M1_IPv4_1");
	private static final String M2_IPv4_1 = System.getProperty("M2_IPv4_1");
	private static final String A1_IPv4_1 = System.getProperty("A1_IPv4_INTERNAL_1");
	
	private String host = null;
	private String user = null;
	private String password = null;
	private Session session = null;


	/**
	 * Create a new SSHExec instance.
	 * 
	 * @param host      Host to connect to
	 * @param user      User to authenticate with
	 * @param password  Password used for authentication
	 */
	public SSHExec(String host, String user, String password) {
		this.host = host;
		this.user = user;
		this.password = password;
	}

	/**
	 * Connect to the remote host.
	 * 
	 * @throws JSchException  If an error occurs
	 */
	public void connect() throws JSchException {
		JSch jsch=new JSch();
		session=jsch.getSession(user, host, 22);
		session.setConfig("StrictHostKeyChecking", "no");
		session.setPassword(password);
		session.connect();
	}
	
	/**
	 * Connect to the remote host.
	 * 
	 * @throws JSchException  If an error occurs
	 */
	public void connectPasswordless() throws JSchException {

		JSch jsch=new JSch();
		jsch.setKnownHosts("/root/.ssh/known_hosts");	
		jsch.addIdentity("/root/.ssh/id_sample_harness_sshkey");
		session=jsch.getSession(user, host, 22);
		session.setConfig("StrictHostKeyChecking", "no");		
		session.connect();

	}
	
	/**
	 * Disconnect the current active session
	 */
	public void disconnect()  {
		
		try {
			
			if (session != null) {
				session.disconnect();
			}
		} catch (Throwable t) {
			
		}
		
	}
	
	/**
	 * Execute a specific command on the remote host via
	 * SSH
	 * 
	 * @param cmd  The command to execute
	 * @return     A CLICommand instance with the results of the command
	 */ 
	public CLICommand exec(String cmd)  {
		ChannelExec channel = null;
		InputStream stdout = null;
		InputStream stderr = null;
		CLICommand cmdResult = new CLICommand(cmd);
		String line = null;
		
		try {
			channel =  (ChannelExec) session.openChannel("exec");
			channel.setCommand(cmd);
			channel.setInputStream(null);
			stdout = channel.getInputStream();
			stderr = channel.getErrStream();
			channel.connect();
			waitForChannelClosed(channel, 30000);
			
			// set the return code
			cmdResult.setRetCode(channel.getExitStatus());
			
			// stdout
			BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(stdout));
			ArrayList<String> stdoutList = new ArrayList<String>();
			while ((line = bufferedReader.readLine()) != null) {
				stdoutList.add(line.trim());
			}
			cmdResult.setOutput(stdoutList);
			
			// stderr
			bufferedReader = new BufferedReader(new InputStreamReader(stderr));
			ArrayList<String> stderrList = new ArrayList<String>();
			while ((line = bufferedReader.readLine()) != null) {
				stderrList.add(line.trim());
			}
			cmdResult.setStderr(stderrList);
		} catch (Throwable t) {
			// something went wrong - update result
			cmdResult.setError(t);
			cmdResult.setRetCode(-1);
		} finally {
			try {
				channel.disconnect();
				channel = null;
			} catch (Exception e) {
				// do nothing
			}
		}
	    if (channel != null) 
	    {
	        channel.disconnect();
	    }
	    if (stdout != null)
	    {
	    	try
	    	{
	    		stdout.close();
	    	}
	    	catch(Exception e)
	    	{
	    		// do nothing
	    	}
	    }
	    if (stderr != null)
	    {
	    	try
	    	{
	    		stderr.close();
	    	}
	    	catch(Exception e)
	    	{
	    		// do nothing
	    	}
	    }
		
		return cmdResult;
	}
	
	/**
	 * Run all commands for a specific cmdPrefix. The commands will be obtained
	 * from a properties object. The commands will be executed in order
	 * starting from cmdPrefix.1 .. cmdPrefix.n
	 * 
	 * @param cmdPrefix  The cmdPrefix used to lookup commands.
	 * @param cmdProps   The Properties instance containing the commands. 
	 * @return           A List of CLICommand instances corresponding to each
	 *                   command executed.
	 * @throws CLICommandException Thrown if at least one of the commands fails
	 */
	public List<CLICommand> runCommands(String cmdPrefix, Properties cmdProps) throws CLICommandException {

		List<CLICommand> resultList = new ArrayList<CLICommand>();
		String cmdString = null;
		int cmdPtr = 1;
		boolean failureDetected = false;


		while ((cmdString = cmdProps.getProperty(cmdPrefix + cmdPtr)) != null) {
			try {
				cmdString = checkCommand(cmdString);
				resultList.add(exec(cmdString));

			} catch (Exception e) {

			} finally {
				cmdPtr++;
			}

		}
		
		for (CLICommand result : resultList) {
			if (result.getRetCode() != 0) {
				failureDetected = true;
			}
			System.out.println(result.getCommand());
			System.out.println("[" + result.getRetCode() + "]: " + result.getOutput());
		}
		
		if (failureDetected) {
			throw new CLICommandException(resultList);
		}

		return resultList;
	}
	
	/**
	 * Check a command string for the existence of a known automation variable. 
	 * If the variable exists replace it with the system value specified.  
	 * 
	 * @param cmd  The command to be checked
	 * @return     The command after any replacement.
	 */
	private String checkCommand(String cmd) {

		cmd = cmd.replaceAll("M1_IPv4_1", M1_IPv4_1);
		cmd = cmd.replaceAll("M2_IPv4_1", M2_IPv4_1);
		cmd = cmd.replaceAll("A1_IPv4_1", A1_IPv4_1);

		return cmd;

	}
	
	
	
	
	/**
	 * Execute a specific command on the remote host via
	 * SSH with a timeout. Certain cli commands can take many minutes
	 * to execute if running on a real server i.e. imaserver start with a full 
	 * store. Specify a timeout property as a maximum time to wait for the 
	 * command to complete.
	 * 
	 * @param cmd  The command to execute
	 * @return     A CLICommand instance with the results of the command
	 */ 
	public String exec(String command, long timeout) throws Exception  {
		
		ChannelExec channel =  (ChannelExec) session.openChannel("exec");
		channel.setCommand(command);
		channel.setInputStream(null);
		InputStream stdout = channel.getInputStream();
		InputStream stderr = channel.getErrStream();
		channel.connect();
		
		waitForChannelClosed(channel, timeout);
		String response;

		if (channel.getExitStatus() == 1) 
		{
			BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(stdout));
			response = readAll(bufferedReader);
			if(response!=null && !response.equals(""))
			{
				throw new Exception("IMA appliance command '"+command+"' failed. Response so far: " +response.toString());
			}
			else
			{
				bufferedReader = new BufferedReader(new InputStreamReader(stderr));
				response = readAll(bufferedReader);
				
				throw new Exception("IMA appliance command '"+command+"' failed. Response so far: " +response.toString());
			}
			
	        
	    } 
		else 
	    {
	        BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(stdout));
	        response = readAll(bufferedReader);
	    }

	    if (channel != null) 
	    {
	        channel.disconnect();
	    }
	    if (stdout != null)
	    {
	    	try
	    	{
	    		stdout.close();
	    	}
	    	catch(Exception e)
	    	{
	    		// do nothing
	    	}
	    }
	    if (stderr != null)
	    {
	    	try
	    	{
	    		stderr.close();
	    	}
	    	catch(Exception e)
	    	{
	    		// do nothing
	    	}
	    }
	    
		return response.toString();
		
	}
	

	/*
	 * This waits for the channel to be closed successfully
	 * Some commands such as starting the server on a system with a large
	 * store can take a long time to execute. This method allows us to
	 * wait a specified time before we effectively give up
	 */
	private static void waitForChannelClosed(ChannelExec ce, long maxwaitMs) 
	{    
	    final long until = System.currentTimeMillis() + maxwaitMs;
	    try 
	    {
	        while (!ce.isClosed() && System.currentTimeMillis() < until)
	        { 
	        	if(maxwaitMs > 60000)
	        	{
	        		Thread.sleep(30000);
	        	}
	        	else
	        	{
	        		Thread.sleep(maxwaitMs/40);
	        	}
	        	
	        }

	    } 
	    catch (InterruptedException e) 
	    {
	        // do nothing
	    }
	    if (!ce.isClosed()) 
	    {
	        throw new RuntimeException("ssh command did not execute fully in the specified time!");
	    }
	}
	
	
	/*
	 * Returns a string with the response from the command
	 */
	private static String readAll(BufferedReader bufferedReader) throws IOException 
	{
		StringBuilder builder = new StringBuilder();
		String s = "";
		
		s = bufferedReader.readLine();
		if(s!= null)
		{
			builder.append(s);
		}

		while (((s = bufferedReader.readLine()) != null))
		{
			if(s.equals(""))
			{
				
			}
			else
			{
				builder.append(',');
				builder.append(s);
			}
		   
		}

		
		
		return builder.toString();
	}

	public boolean isConnected()
	{
		if(session != null)
		{
			return session.isConnected();
		}
		else
		{
			return false;
		}
	}
}
