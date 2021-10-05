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


public class ControlFailover {
	
	private long timeout = 5000000;	
	private String cliaddressA = null;
	private String cliaddressB = null;
	private boolean forceStop = false;
	private long resynchTime = 0;
	private ServerControl control = null;
	
	public ControlFailover(String ipaddressA, String ipaddressB, long timeout, boolean force, long resynchTime)
	{
				this.cliaddressA = ipaddressA;
				if(ipaddressB != null)
				{
					this.cliaddressB = ipaddressB;
				}
				this.timeout = timeout;
				this.forceStop = force;
				this.resynchTime = resynchTime;
	}
	
	
	public void triggerFail()
	{
		control = new ServerControl(this.cliaddressA, this.cliaddressB, this.timeout, this.forceStop, this.resynchTime);
		new Thread(control).start();
	}
	
	public void triggerFail(boolean force)
	{
		control = new ServerControl(this.cliaddressA, this.cliaddressB, this.timeout, force, this.resynchTime);
		new Thread(control).start();
	}
	
	public boolean isStillRecovering()
	{
		if(control != null)
		{
			return control.isStillRecovering();
		}
		else
		{
			return false;
		}
			
	}
	
	public long returnTimeCompleted()
	{
		return control.returnTimeCompleted();
	}

}
