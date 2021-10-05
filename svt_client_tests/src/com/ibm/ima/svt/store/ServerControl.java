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
package com.ibm.ima.svt.store;

import com.ibm.ima.test.cli.imaserver.ImaHAServerStatus;
import com.ibm.ima.test.cli.imaserver.ImaServer;
import com.ibm.ima.test.cli.imaserver.ImaServerStatus;
import com.ibm.ims.svt.clients.Audit;

public class ServerControl implements Runnable{
	
	private long timeout = 5000000;	
	private ImaServer server = null;
	private Audit trace = null;
	private String cliaddressB = null;
	private boolean isStillRecovering = false;
	private boolean forceStop = false;
	private long timeCompleted = 0;
	private long resynchTime = 0;
	
	public ServerControl(String ipaddressA, String ipaddressB, long timeout, boolean force, long resynchTime)
	{
		if(ipaddressB!= null)
		{
			server = new ImaHAServerStatus(ipaddressA,ipaddressB, "admin", "admin");
			this.timeout = timeout;
			
			
		}
		else
		{

			server = new ImaServerStatus(ipaddressA, "admin", "admin");
			
		
		}
		this.resynchTime = resynchTime;
		this.forceStop = force;
		server.setTimeout(timeout);
		trace = new Audit("stdout", true);
		cliaddressB = ipaddressB;
		
		try
		{
			server.disconnectServer();
		}
		catch(Exception ez)
		{
			// do nothing
		}
		
	}

	public void run() {
		
		try
		{
			server.connectToServer();
			trace.trace("Going to stop the server now. If HA configuration this will cause a failover");
			
			isStillRecovering = true;
			server.stopServer(timeout, forceStop, true, resynchTime);
			
			
			
			if(cliaddressB== null)
			{
				if(server.isServerStopped())
				{
					trace.trace("The server has stopped");
				}
				else
				{
					// lets just try again in case we get an exception
					int i=0;
					do
					{
						i++;
						Thread.sleep(10000);
						
					}while(!server.isServerStopped() || i==10);
					
					
					if(server.isServerStopped())
					{
						trace.trace("The server has stopped");
					}
					else
					{
						trace.trace("Server status is stopped=" + server.isServerStopped());
						throw new RuntimeException("The server could not be stopped in time");
					}
				}
			
				trace.trace("Now starting the server");
				
				server.startServer(timeout);
			
				if(server.isServerRunning())
				{
					trace.trace("The server has started");
				}
				else
				{
					// lets just try again in case we get an exception
					int i=0;
					do
					{
						i++;
						Thread.sleep(10000);
						
					}while(!server.isServerRunning() || i==10);
					
					if(server.isServerRunning())
					{
						trace.trace("The server has started");
					}
					else
					{
						throw new RuntimeException("The server could not be started in time");
					}
				}
			}
			isStillRecovering = false;
			timeCompleted = System.currentTimeMillis();
			try
			{
				
				server.disconnectServer();
			}
			catch(Exception ez)
			{
				trace.trace("Exception occurred closing the connections");
				ez.printStackTrace();
			}
			
		}
		catch(Exception e)
		{
			trace.trace("Exception occurred stopping and starting server");
			e.printStackTrace();
		}
		
	}
	
	public boolean isStillRecovering()
	{
		return this.isStillRecovering;
	}
	
	public long returnTimeCompleted()
	{
		return timeCompleted;
	}

}
