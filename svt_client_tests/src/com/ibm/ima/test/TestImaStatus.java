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
package com.ibm.ima.test;

import com.ibm.ima.test.cli.imaserver.ImaServerStatus;

import junit.framework.TestCase;

public class TestImaStatus extends TestCase
{
	private ImaServerStatus imaStatusHelper;
	
	public void setUp()
	{
		System.out.println("Setting up done");
	}
	
	public void tearDown()
	{
		System.out.println("Teardown completed");
	}
	
	public void testSample1()
	{
		try 
		{
			imaStatusHelper = new ImaServerStatus("9.3.177.15", "admin", "admin");
			imaStatusHelper.connectToServer();
			assertTrue("Checking JSCH is connected to the appliance." , imaStatusHelper.isConnected());
			assertTrue("Checking if Server is running.", imaStatusHelper.isServerRunning());
			System.out.println(imaStatusHelper.showStatus());
			
		} 
		catch (Exception e) 
		{
			assertFalse(true);
			e.printStackTrace();
		}
	}	
}
