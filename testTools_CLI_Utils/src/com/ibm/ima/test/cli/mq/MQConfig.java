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
package com.ibm.ima.test.cli.mq;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.ibm.ima.test.cli.imaserver.ImaCliHelper;

public class MQConfig extends ImaCliHelper {

	private String mqlocation = "/opt/mqm/bin";	
		
	private String isRunningRegEx = "(.*)(Running+)";
	private String isStoppedRegEx = "(.*)(Ended normally+)";
	private String topicCreateRegEx = "(.*)(WebSphere MQ topic created+)(.*)";
	private String channelCreateRegEx = "(.*)(WebSphere MQ channel created+)(.*)";
	private String alterQMGRRegEx = "(.*)(WebSphere MQ queue manager changed+)(.*)";
	private String listenerCreateRegEx =  "(.*)(WebSphere MQ listener created+)(.*)";
	private String objectRunning = "(.*)(RUNNING+)(.*)";
	private String objectNotFound = "(.*)(WebSphere MQ object {NAME} not found+)(.*)";
	private String authorityCreateRegEx="(.*)(WebSphere MQ authority record set+)(.*)";
	private String authenticationRegEx="(.*)(WebSphere MQ channel authentication record set+)(.*)";
	private String queueCreatedRegEx="(.*)(WebSphere MQ queue created+)(.*)";
	private String qmgrChangedRegEx="(.*)(WebSphere MQ queue manager changed+)(.*)";
	private String serviceCreatedRegEx="(.*)(WebSphere MQ service created+)(.*)";
	private String serviceStartRequestRegEx="(.*)(Request to start Service accepted+)(.*)";
	private String listenerStartRequestRegEx="(.*)(Request to start WebSphere MQ listener accepted+)(.*)";
	private String channelStopped="(.*)(STOPPED+)(.*)";
	private String retainedRegEx="(.*)(RETAINED\\(YES\\))(.*)";
	private String notRetainedRegEx="(.*)(RETAINED\\(NO\\))(.*)";
	
	
	private String listQMGRS = null;
	private String stopQMGR = null;
	private String startQMGR = null;
	private String createQMGR = null;
	private String createTOPIC = null;
	private String createIMSChannel = null;
	private String disableChannelAuth = null;
	private String createListener = null;
	private String startListener = null;
	private String isListenerRunning = null;
	private String listenerExists = null;
	private String deleteQMGR = null;
	private String createQUEUE = null;
	private String stopChannel = null;
	private String startChannel = null;
	private String displayChannel = null;
	private String channelStatus = null;
	private String displayQueue = null;
	private String deleteQueue = null;
	private String queueDepth = null;
	private String retainedMsgs =  null;
	private String clearRetained = null;
	private String clearQueue = null;
	

	/**
	 * Creates an MQConfiguration helper class
	 * @param session - which must be using a user that has mq authroity to create objects etc
	 * @param mqlocation - this is the location to run the mq libraries. If null the default of /opt/mqm/bin will be used
	 * @param timeout - the time to wait for the command to execute successully
	 * 
	 */
	public MQConfig(String ipaddress, String username, String password, String mqlocation)
	{
		
		super(ipaddress.trim(), username.trim(), password.trim());
		if(mqlocation != null)
		{
			this.mqlocation = mqlocation;
		}
		this.setTimeout(60000);
		
		
		listQMGRS = MQCommandReference.LIST_QMGRS;
		startQMGR = MQCommandReference.START_QMGR;
		stopQMGR = MQCommandReference.STOP_QMGR;
		createQMGR = MQCommandReference.CREATE_QMGR;
		createTOPIC = MQCommandReference.CREATE_TOPIC;
		createIMSChannel = MQCommandReference.CREATE_CHANNEL;
		disableChannelAuth = MQCommandReference.DIS_CHLAUTH;
		createListener = MQCommandReference.CREATE_LISTENER;
		startListener = MQCommandReference.START_LISTENER;		
		isListenerRunning = MQCommandReference.LISTENER_STATUS;
		listenerExists = MQCommandReference.LISTENER_EXISTS;
		deleteQMGR = MQCommandReference.DELETE_QMGR;
		createQUEUE = MQCommandReference.CREATE_QUEUE;
		stopChannel = MQCommandReference.STOP_CHANNEL;
		startChannel = MQCommandReference.START_CHANNEL;
		displayChannel = MQCommandReference.LIST_CHANNEL;
		channelStatus = MQCommandReference.CHANNEL_STATUS;
		displayQueue = MQCommandReference.LIST_QUEUE;
		deleteQueue = MQCommandReference.DELETE_QUEUE;
		queueDepth = MQCommandReference.QUEUE_DEPTH;
		retainedMsgs = MQCommandReference.IS_RETAINED;
		clearRetained = MQCommandReference.CLEAR_RETAINED;
		clearQueue = MQCommandReference.CLEAR_QUEUE;
	}
	
	/**
	 * This method will determine whether a queue manager exists
	 * @param qmgrName - the name of the queue manager
	 * @return true if it exists; false otherwise
	 * @throws Exception thrown if the command fails
	 */
	public boolean qmgrExists(String qmgrName) throws Exception
	{
		isConnectedToServer();
		String command = listQMGRS.replace("{LOC}", mqlocation);
	
		command = command.replace("{QMGR}", qmgrName);
		String result = sshExecutor.exec(command, timeout);
		if(result != null && ! result.equals(""))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	
	/**
	 * This command determines is a queue manager is running
	 * @param qmgrName - the name of a queue manager
	 * @return true if running; false otherwise
	 * @throws Exception thrown if the queue manager does not exist or a problem was encountered running this command.
	 */
	public boolean isQmgrRunning(String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(qmgrExists(qmgrName))
		{
			String command = listQMGRS.replace("{LOC}", mqlocation);
			command = command.replace("{QMGR}", qmgrName);
			String result = sshExecutor.exec(command, timeout);
			
			Pattern p = Pattern.compile(isRunningRegEx);
		    Matcher m = p.matcher(result); // get a matcher object
		    return m.find();
		}
		else
		{
			throw new Exception("The queue manager does not exist");
		}
	}
	
	
	/**
	 * This command determines is a queue manager is stopped (Note: this will only return false
	 * if the qmgr has been stopped in an expected manner)
	 * 
	 * @param qmgrName - the name of a queue manager
	 * @return true if stopped; false otherwise
	 * @throws Exception thrown if the queue manager does not exist or a problem was encountered running this command.
	 */
	public boolean isQmgrStopped(String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(qmgrExists(qmgrName))
		{
			String command = listQMGRS.replace("{LOC}", mqlocation);
			command = command.replace("{QMGR}", qmgrName);
			String result = sshExecutor.exec(command, timeout);
			
			Pattern p = Pattern.compile(isStoppedRegEx);
		    Matcher m = p.matcher(result); // get a matcher object
		    return m.find();
		    
		}
		else
		{
			throw new Exception("The queue manager does not exist");
		}
	}
	
	/**
	 * This method will start a queue manager
	 * @param qmgrName
	 * @throws Exception thrown if the queue manager does not exist or a problem was encountered running this command.
	 */
	public void startQmgr(String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(qmgrExists(qmgrName))
		{
			if(isQmgrRunning(qmgrName))
			{
				throw new Exception("The qmgr was already started");
			}
			else
			{
				String command = startQMGR.replace("{LOC}", mqlocation);
				command = command.replace("{QMGR}", qmgrName);
				sshExecutor.exec(command, timeout);
			}
		}
		else
		{
			throw new Exception("The queue manager does not exist");
		}
	}
	
	/**
	 * This method will stop a queue manager
	 * @param qmgrName
	 * @throws Exception thrown if the queue manager does not exist or a problem was encountered running this command.
	 */
	public void stopQmgr(String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(qmgrExists(qmgrName))
		{
			if(isQmgrStopped(qmgrName))
			{
				throw new Exception("The qmgr was already stopped");
			}
			else
			{
				String command = stopQMGR.replace("{LOC}", mqlocation);
				command = command.replace("{QMGR}", qmgrName);
				sshExecutor.exec(command, timeout);
			}
		}
		else
		{
			throw new Exception("The queue manager does not exist");
		}
	}
	
	/**
	 * This method will create a queue manager
	 * @param qmgrName - the name of the queue manager to create
	 * @throws Exception thrown if the queue manager already exists or a problem was encountered running this command.
	 */
	public void createQmgr(String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(!qmgrExists(qmgrName))
		{
			
			String command = createQMGR.replace("{LOC}", mqlocation);
			command = command.replace("{QMGR}", qmgrName);
			sshExecutor.exec(command, timeout);
		}
		else
		{
			throw new Exception("The queue manager already exists");
		}
	}
	
	
	
	/**
	 * This method attempts to delete a qmgr. If the qmgr is running we will attempt to stop the qmgr first.
	 * @param qmgrName
	 * @throws Exception thrown if the queue manager does not exist or a problem was encountered running this command.
	 */
	public void deleteQmgr(String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(qmgrExists(qmgrName))
		{
			
			if(!isQmgrStopped(qmgrName))
			{
				try
				{
					
					stopQmgr(qmgrName);
				
				}
				catch(Exception e)
				{
					// do nothing but still attempt to delete the qmgr
				}
			}
			// we will make two attempts
			if(!isQmgrStopped(qmgrName))
			{
				try
				{
					
					stopQmgr(qmgrName);
				
				}
				catch(Exception e)
				{
					// do nothing but still attempt to delete the qmgr
				}
			}
			String command = deleteQMGR.replace("{LOC}", mqlocation);
			command = command.replace("{QMGR}", qmgrName);
			String result = sshExecutor.exec(command, timeout);
				
			if(! qmgrExists(qmgrName))
			{
				// everything went ok
				
			}
			else
			{
				throw new Exception("A problem occurred deleting the queue manager " + result);
			}
			
		}
		else
		{
			throw new Exception("The queue manager does not exist");
		}

	}
	
	
	
			
	
	/**
	 * This method will create a local queue
	 * @param qmgrName - the name of the queue manager to create
	 * @param name - the name of the queue
	 * @throws Exception
	 */
	public void createQueueLocal(String name, String qmgrName) throws Exception
	{
		isConnectedToServer();
		String command = createQUEUE.replace("{LOC}", mqlocation);
		command = command.replace("{QMGR}", qmgrName);
		command = command.replace("{NAME}", name);
		
		if(!isQmgrRunning(qmgrName))
		{
			try
			{
				
				startQmgr(qmgrName);
			}
			catch(Exception e)
			{
				// do nothing but still attempt to delete the qmgr
			}
		}
		
		String result = sshExecutor.exec(command, timeout);
		
		Pattern p = Pattern.compile(queueCreatedRegEx);
	    Matcher m = p.matcher(result); // get a matcher object
	    if( m.find())
	    {
	    	
	    }
	    else
	    {
	    	throw new Exception("Queue could not be created: " + result);
	    }
		
	
	}
	
	/**
	 * This method will create a topic
	 * @param topicName - the name of the topic
	 * @param topicString - the topic string
	 * @param qmgrName - the name of the qmgr
	 * @throws exception if the topic cannot be created
	 */
	public void createTopic(String topicName, String topicString, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		String command = createTOPIC.replace("{LOC}", mqlocation);
		command = command.replace("{QMGR}", qmgrName);
		command = command.replace("{TOPICSTR}", topicString);
		command = command.replace("{NAME}", topicName);
		String result =  sshExecutor.exec(command, timeout);
		
		Pattern p = Pattern.compile(topicCreateRegEx);
	    Matcher m = p.matcher(result); // get a matcher object
	    if(m.find())
	    {
	    	// it was created
	    }
	    else
	    {
	    	throw new Exception(result);
	    }
				
	}

	/**
	 * This method creates a SRVCON channel for IMS
	 * @param channelName -the name of the channel
	 * @param mcauser - the mcauser
	 * @param qmgrName - the qmgr name
	 * @throws Exception is thrown if the channgel cannot be created
	 */
	public void createIMSChannel(String channelName, String mcauser, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{

			startQmgr(qmgrName);
		}
		String command = createIMSChannel.replace("{LOC}", mqlocation);
		command = command.replace("{QMGR}", qmgrName);
		command = command.replace("{CHNL}", channelName);
		command = command.replace("{MCAUSER}", mcauser);
		
		String result = sshExecutor.exec(command, timeout);
		
		Pattern p = Pattern.compile(channelCreateRegEx);
		Matcher m = p.matcher(result); // get a matcher object
		if(m.find())
		{
			
		}
		else
		{
			throw new Exception("Channel could not be created: " + result);
		}
	}
	
	/**
	 * This disabled channel authentication on the qmgr
	 * @param qmgrName
	 * @throws Exception if the qmgr cannot be modified
	 */
	public void disableChannelAuth(String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		String command = disableChannelAuth.replace("{LOC}", mqlocation);
		command = command.replace("{QMGR}", qmgrName);
		
		String result = sshExecutor.exec(command, timeout);
		
		Pattern p = Pattern.compile(alterQMGRRegEx);
		Matcher m = p.matcher(result); // get a matcher object
		if(m.find())
		{
			
		}
		else
		{
			throw new Exception("Channelauth could not be disabled: " + result);
		}
		
	}
	
	
	/**
	 * This method creates a TCP listener
	 * @param listenerName - name to create
	 * @param port - the port of the listener
	 * @param qmgrName - the qmgr name
	 * @throws Exception is thrown if the listener could not be created
	 */
	public void createListener(String listenerName, int port, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		String command = createListener.replace("{LOC}", mqlocation);
		command = command.replace("{QMGR}", qmgrName);
		command = command.replace("{NAME}", listenerName);
		command = command.replace("{PORT}", Integer.toString(port));
		
		String result = sshExecutor.exec(command, timeout);
		
		Pattern p = Pattern.compile(listenerCreateRegEx);
		Matcher m = p.matcher(result); // get a matcher object
		if(m.find())
		{
			
		}
		else
		{
			throw new Exception("Listener could not be created: " + result);
		}
		
		
	}
	
	/**
	 * Starts the listener
	 * @param name of the listener to start
	 * @param qmgr - qmgr name
	 * @throws Exception
	 */
	public void startListener(String name, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		String command = startListener.replace("{LOC}", mqlocation);
		command = command.replace("{QMGR}", qmgrName);
		command = command.replace("{NAME}", name);
		
		String result = sshExecutor.exec(command, timeout);
		
		Pattern p = Pattern.compile(listenerStartRequestRegEx);
		Matcher m = p.matcher(result); // get a matcher object
		if(m.find())
		{
			
		}
		else
		{
			throw new Exception("listener request failed: " + result);
		}

	
	}
	
	
	/**
	 * This method checks to see if a listener is running
	 * @param name of the listener
	 * @param qmgrName name of the qmgr
	 * @return true if listener is running
	 * @throws Exception is thrown if the listener does not exist
	 */
	public boolean isListenerRunning(String name, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		if(listenerExists(name, qmgrName))
		{
		
			String command = isListenerRunning.replace("{LOC}", mqlocation);
			command = command.replace("{QMGR}", qmgrName);
			command = command.replace("{NAME}", name);

			String result = sshExecutor.exec(command, timeout);
		
			Pattern p = Pattern.compile(objectRunning);
			Matcher m = p.matcher(result); // get a matcher object
			if(m.find())
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			throw new Exception("Listener " + name  + " does not exist");
		}
		
	}
	
	
	/**
	 * This method checks to see if a listener exists
	 * @param name of the listener 
	 * @param qmgrName name of the qmgr
	 * @return true if it exists; false otherwise
	 * @throws Exception
	 */
	public boolean listenerExists(String name, String qmgrName) throws Exception
	{
		isConnectedToServer();
		String command = listenerExists.replace("{LOC}", mqlocation);
		command = command.replace("{QMGR}", qmgrName);
		command = command.replace("{NAME}", name);
		String result = sshExecutor.exec(command, timeout);	
		
		if(result != null && ! result.equals(""))
		{
			String notFound = objectNotFound.replace("{NAME}", name);
			
			Pattern p = Pattern.compile(notFound);
			Matcher m = p.matcher(result); // get a matcher object
			if(m.find())
			{
				return false;
			}
			else
			{
				return true;
			}
		}
		else
		{
			return false;
		}
		
	}
	
	
	/**
	 *  
	 * Configures an MQTT service for a qmgr. Note: This will only work on an MQ system
	 * that has been installed with the MQTT service. 
	 * 
	 * @param port of the mqtt channel
	 * @param qmgrName name of the qmgr
	 * @throws Exception
	 */
	public void createMqttService(int port, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		String command = MQCommandReference.MQTT_QUEUE.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		
		String result = sshExecutor.exec(command, timeout);	
		
		Pattern p = Pattern.compile(queueCreatedRegEx);
		Matcher m = p.matcher(result); // get a matcher object
		if( m.find())
		{
		   	
		}
		else
		{
		 	throw new Exception("Queue for the mqtt service could not be created: " + result);
		}

		
		command = MQCommandReference.MQTT_ALTER_QMGR.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);

		result = sshExecutor.exec(command, timeout);
		
		p = Pattern.compile(qmgrChangedRegEx);
		m = p.matcher(result); // get a matcher object
		if( m.find())
		{
		    	
		}
		else
		{
		 	throw new Exception("QMGR could not be changed for the MQTT service: " + result);
		}
		
		
		command = MQCommandReference.MQTT_CMD1.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		result = sshExecutor.exec(command, timeout);
				
		
		command = MQCommandReference.MQTT_CMD2.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		result = sshExecutor.exec(command, timeout);
		
		
		command = MQCommandReference.MQTT_SERVICE_CREATE.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		result = sshExecutor.exec(command, timeout);
		
		p = Pattern.compile(serviceCreatedRegEx);
		m = p.matcher(result); // get a matcher object
		if( m.find())
		{
			
		}
		else
		{
			throw new Exception("Channel could not be created for the MQTT service: " + result);
		}
		
		command = MQCommandReference.MQTT_SERVICE_START.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		
		result = sshExecutor.exec(command, timeout);
		
		p = Pattern.compile(serviceStartRequestRegEx);
		m = p.matcher(result); // get a matcher object
		if( m.find())
		{
			
		}
		else
		{
		  	throw new Exception("MQTT service start request was unsuccessful: " + result);
		}
	
		command = MQCommandReference.MQTT_SERVICE_CHANNEL.replace("{QMGR}", qmgrName);
		command = command.replace("{PORT}", Integer.toString(port));
		command = command.replace("{LOC}", mqlocation);
		
		
		result = sshExecutor.exec(command, timeout);
		
		p = Pattern.compile(channelCreateRegEx);
		m = p.matcher(result); // get a matcher object
		if( m.find())
		{
		    	
		}
		else
		{
		   	throw new Exception("Channel could not be created for the MQTT service: " + result);
		}
	
	}
	
	
	/**
	 * This method configures all the authentication records for the objects required by MQConnectivity.
	 * @param channel - the channel that is used 
	 * @param principalUser - the prinipal user
	 * @param qmgrName - name of the qmgr
	 * @throws Exception
	 */
	public void configureMQChannelAuth(String channel, String principalUser, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		String command = MQCommandReference.AUTH_CMD_1.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		command = command.replace("{USER}", principalUser);
		
		String result = sshExecutor.exec(command, timeout);	
		
		Pattern p = Pattern.compile(authorityCreateRegEx);
		Matcher m = p.matcher(result); // get a matcher object
		if( m.find())
		{
//			System.out.println("Result: "+result);
		}
		else
		{
		 	throw new Exception("An problem occurred setting up channel auth: " + result);
		}
		
		
		command = MQCommandReference.AUTH_CMD_2.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		command = command.replace("{USER}", principalUser);
		command = command.replace("{CHANNEL}", channel);
		
//		System.err.println("Running auth command: "+command);
		result = sshExecutor.exec(command, timeout);	
		
		p = Pattern.compile(authenticationRegEx);
		m = p.matcher(result); // get a matcher object
		if( m.find())
		{
//			System.err.println("Result: "+result);
		}
		else
		{
		 	throw new Exception("An problem occurred setting up channel auth: " + result);
		}
		
		command = MQCommandReference.AUTH_CMD_3.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		command = command.replace("{USER}", principalUser);
		command = command.replace("{CHANNEL}", channel);
		
//		System.err.println("Running auth command: "+command);
		result = sshExecutor.exec(command, timeout);	
		
		p = Pattern.compile(authenticationRegEx);
		m = p.matcher(result); // get a matcher object
		if( m.find())
		{
//			System.err.println("Result: "+result);
		}
		else
		{
			throw new Exception("An problem occurred setting up channel auth: " + result);
		}
	
		command = MQCommandReference.AUTH_CMD_4.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		command = command.replace("{USER}", principalUser);
		
//		System.err.println("Running auth command: "+command);
		result = sshExecutor.exec(command, timeout);	
		p = Pattern.compile(authorityCreateRegEx);
		m = p.matcher(result); // get a matcher object
		if( m.find())
		{
//			System.err.println("Result: "+result);
		}
		else
		{
			throw new Exception("An problem occurred setting up channel auth: " + result);
		}
		
		command = MQCommandReference.AUTH_CMD_5.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		command = command.replace("{USER}", principalUser);
		
//		System.err.println("Running auth command: "+command);
		result = sshExecutor.exec(command, timeout);	
		p = Pattern.compile(authorityCreateRegEx);
		m = p.matcher(result); // get a matcher object
		if( m.find())
		{
//			System.err.println("Result: "+result);
		}
		else
		{
			throw new Exception("An problem occurred setting up channel auth: " + result);
		}
	
		command = MQCommandReference.AUTH_CMD_6.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		command = command.replace("{USER}", principalUser);
		
//		System.err.println("Running auth command: "+command);
		result = sshExecutor.exec(command, timeout);	
		p = Pattern.compile(authorityCreateRegEx);
		m = p.matcher(result); // get a matcher object
		if( m.find())
		{
//			System.err.println("Result: "+result);
		}
		else
		{
		   	throw new Exception("An problem occurred setting up channel auth: " + result);
		}
		
		command = MQCommandReference.AUTH_CMD_7.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		command = command.replace("{USER}", principalUser);
		
//		System.err.println("Running auth command: "+command);
		result = sshExecutor.exec(command, timeout);	
		
		p = Pattern.compile(authorityCreateRegEx);
		m = p.matcher(result); // get a matcher object
		if(m.find())
		{
//			System.err.println("Result: "+result);
		}
		else
		{
			throw new Exception("An problem occurred setting up channel auth: " + result);
		}
		
//		System.err.println("Running auth command: "+command);
		command = MQCommandReference.AUTH_CMD_8.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		command = command.replace("{USER}", principalUser);
		
		result = sshExecutor.exec(command, timeout);	
		
		p = Pattern.compile(authorityCreateRegEx);
		m = p.matcher(result); // get a matcher object
		if(m.find())
		{
//			System.err.println("Result: "+result);
		}
		else
		{
			throw new Exception("An problem occurred setting up channel auth: " + result);
		}
	}
	
	/**
	 * This method can be used when channel authentication is enabled to set and auth record
	 * for a topic
	 * @param topic - name of topic
	 * @param principalUser - user
	 * @param qmgrName - name of qmgr
	 * @throws Exception
	 */
	public void setAuthRecordTopic(String topic, String principalUser, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		String command = MQCommandReference.AUTH_TOPIC.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		command = command.replace("{USER}", principalUser);
		command = command.replace("{TOPIC}", topic);
		
		
		String result = sshExecutor.exec(command, timeout);
		
		Pattern p = Pattern.compile(authorityCreateRegEx);
		Matcher m = p.matcher(result); // get a matcher object
		if(m.find())
		{
			
		}
		else
		{
			throw new Exception("An problem occurred setting up channel auth: " + result);
		}
	
	}
	
	/**
	 * This method can be used when channel authentication is enabled to set and auth record
	 * for a queue
	 * @param queue - name of queue
	 * @param principalUser - user
	 * @param qmgrName - name of qmgr
	 * @throws Exception
	 */
	public void setAuthRecordQueue(String queue, String principalUser, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			
			startQmgr(qmgrName);
		}
		
		String command = MQCommandReference.AUTH_QUEUE.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		command = command.replace("{USER}", principalUser);
		command = command.replace("{QUEUE}", queue);
		
		String result = sshExecutor.exec(command, timeout);
		
		Pattern p = Pattern.compile(authorityCreateRegEx);
		Matcher m = p.matcher(result); // get a matcher object
		if(m.find())
		{
			
		}
		else
		{
			throw new Exception("An problem occurred setting up channel auth: " + result);
		}
		
		
		
	}
	
	/**
	 * This method determines if a channel exists on the qmgr
	 * @param channel - name of the channel
	 * @param qmgrName - name of the qmgr
	 * @return true if the channel exists; false otherwise
	 * @throws Exception
	 */
	public boolean channelExists(String channel, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		String command = displayChannel.replace("{QMGR}", qmgrName);
		command = command.replace("{NAME}", channel);
		command = command.replace("{LOC}", mqlocation);
	
		String result = sshExecutor.exec(command, timeout);
				
		if(result != null && ! result.equals(""))
		{
			String notFound = objectNotFound.replace("{NAME}", channel);
			
			Pattern p = Pattern.compile(notFound);
			Matcher m = p.matcher(result); // get a matcher object
			if(m.find())
			{
				return false;
			}
			else
			{
				return true;
			}
		}
		else
		{
			return false;
		}
	}


	/**
	 * Returns true if the channel exists and is active; false otherwise
	 * @param channel -  name of the channel
	 * @param qmgrName - name of the qmgr
	 * @return
	 * @throws Exception is thrown if the channel does not exist
	 */
	public boolean isChannelActive(String channel, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		if(channelExists(channel, qmgrName))
		{
			String command = channelStatus.replace("{QMGR}", qmgrName);
			command = command.replace("{NAME}", channel);
			command = command.replace("{LOC}", mqlocation);
		
			String result = sshExecutor.exec(command, timeout);
	
			if(result != null && ! result.equals(""))
			{
				Pattern p = Pattern.compile(channelStopped);
				Matcher m = p.matcher(result); // get a matcher object
				if(m.find())
				{
					return false;
				}
				else
				{
					p = Pattern.compile(objectRunning);
					m = p.matcher(result); // get a matcher object
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
			else
			{
				return false;
			}
		}
		else
		{
			throw new Exception("The channel does not exist");
		}
	}
	
	/**
	 * This method stops an MQ channel using the force mode if it is active. If it is not active no
	 * attempt will be made to stop it.
	 * 
	 * @param channel
	 * @param qmgrName
	 * @return boolean - true if the channel was stopped; false if the channel was not active in the first place
	 * @throws Exception is thrown if the channel cannot be found
	 */
	public boolean stopChannel(String channel, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		if(isChannelActive(channel, qmgrName))
		{
			String command = stopChannel.replace("{QMGR}", qmgrName);
			command = command.replace("{NAME}", channel);
			command = command.replace("{LOC}", mqlocation);
			sshExecutor.exec(command, timeout);
			return true;
		}
		else
		{
			return false;
		}
	}
	
	/**
	 * This method start a channel
	 * @param channel
	 * @param qmgrName
	 * @return
	 * @throws Exception is thrown if the channel does not exist
	 */
	public void startChannel(String channel, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		if(channelExists(channel, qmgrName))
		{
			String command = startChannel.replace("{QMGR}", qmgrName);
			command = command.replace("{NAME}", channel);
			command = command.replace("{LOC}", mqlocation);
			sshExecutor.exec(command, timeout);
		}
		else
		{
			throw new Exception("Channel does not exist");
		}
	}

	/**
	 * Returns true if the queue exists; false otherwise
	 * @param name
	 * @param qmgrName
	 * @return
	 * @throws Exception
	 */
	public boolean qlocalExists(String name, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		String command = displayQueue.replace("{QMGR}", qmgrName);
		command = command.replace("{NAME}", name);
		command = command.replace("{LOC}", mqlocation);
	
		String result = sshExecutor.exec(command, timeout);
				
		if(result != null && ! result.equals(""))
		{
			String notFound = objectNotFound.replace("{NAME}", name);
			
			Pattern p = Pattern.compile(notFound);
			Matcher m = p.matcher(result); // get a matcher object
			if(m.find())
			{
				return false;
			}
			else
			{
				return true;
			}
		}
		else
		{
			return false;
		}
	}
	
	/**
	 * This method deletes a queue
	 * @param name
	 * @param qmgrName
	 * @throws Exception is thrown if the queue does not exist or it cannot be deleted
	 */
	public void deleteQueue(String name, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		if(qlocalExists(name, qmgrName))
		{
			
			String command = deleteQueue.replace("{LOC}", mqlocation);
			command = command.replace("{QMGR}", qmgrName);
			command = command.replace("{NAME}", name);
			sshExecutor.exec(command, timeout);
				
			if(! qlocalExists(name, qmgrName))
			{
				// everything went ok
				
			}
			else
			{
				throw new Exception("A problem occurred deleting the queue");
			}
			
		}
		else
		{
			throw new Exception("The queue does not exist");
		}
	}
	
	/**
	 * This method returns the current queue depth
	 * @param name
	 * @param qmgrName
	 * @return the current queue depth
	 * @throws Exception is thrown if the queue does not exist or we cannot calculate the depth
	 */
	public int getCurrentQueueDepth(String name, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		if(qlocalExists(name, qmgrName))
		{

			String command = queueDepth.replace("{QMGR}", qmgrName);
			command = command.replace("{NAME}", name);
			command = command.replace("{LOC}", mqlocation);
		
			String result = sshExecutor.exec(command, timeout);
			Pattern p = Pattern.compile("(CURDEPTH(.*)+)");
			Matcher m = p.matcher(result); // get a matcher object
			if(m.find())
			{
				String match = m.group(0).substring(m.group(0).indexOf("(")+1, m.group(0).indexOf(")"));
				try
				{
					int curDepth = new Integer(match);
					return curDepth;
				}
				catch(NumberFormatException mfe)
				{
					throw new Exception("Unable to find queue depth");
				}
				
				
			}
			else
			{
				throw new Exception("Unable to find queue depth");
			}
			
		}
		else
		{
			throw new Exception("Could not find queue");
		}
		
	}
	
	/**
	 * This method determines whether a topic has retained messages or not
	 * @param topicString
	 * @param qmgrName
	 * @return true if it contains retained messages; false otherwise
	 * @throws Exception
	 */
	public boolean hasRetainedMessages(String topicString, String qmgrName) throws Exception
	{

		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		String command = retainedMsgs.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		command = command.replace("{TOPICSTRING}", topicString);
	
		String result = sshExecutor.exec(command, timeout);
		Pattern p = Pattern.compile(retainedRegEx);
		Matcher m = p.matcher(result); // get a matcher object
	
		if(m.find())
		{
			return true;

		}
		else
		{
			p = Pattern.compile(notRetainedRegEx);
			m = p.matcher(result); // get a matcher object
			if(m.find())
			{
				return false;

			}
			else
			{
				throw new RuntimeException("We were unable to determine whether the topic contained retained messages: " + result);
			}
		}
	
	}
	
	/**
	 * This method clears all retained messages from a topic string
	 * @param topicString
	 * @param qmgrName
	 * @throws Exception
	 */
	public void clearRetained(String topicString, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		String command = clearRetained.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		command = command.replace("{TOPICSTRING}", topicString);
	
		sshExecutor.exec(command, timeout);
	}
	
	/**
	 * This method clears all messages from a Queue
	 * @param queueName
	 * @param qmgrName
	 * @throws Exception
	 */
	public void clearQueue(String queueName, String qmgrName) throws Exception
	{
		isConnectedToServer();
		if(! isQmgrRunning(qmgrName))
		{
			startQmgr(qmgrName);
		}
		
		String command = clearQueue.replace("{QMGR}", qmgrName);
		command = command.replace("{LOC}", mqlocation);
		command = command.replace("{NAME}", queueName);
		
		sshExecutor.exec(command, timeout);
	}
	
}

