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
package com.ibm.ima.mqcon.enabledediting;

import com.ibm.ima.mqcon.basetest.MQConnectivityTest;
import com.ibm.ima.mqcon.msgconversion.ReturnCodes;
import com.ibm.ima.mqcon.utils.JmsSession;

public abstract class JmsToJms extends MQConnectivityTest{
	
	protected static String imahostname = null;
	protected static int imaport = 0;
	protected static String mqhostname = null;
	protected static int mqport = 0;
	protected static String imadestinationName = null;
	protected static String mqDestinationName = null;
	protected static int timeout = 1;
	protected static int RC = ReturnCodes.INCOMPLETE_TEST;
	protected static String mqqueuemanager = null;
	protected static JmsSession consumerJMSSession = new JmsSession();
	protected static JmsSession producerJMSSession = new JmsSession();

	
	protected void closeConnections()
	{
		try
		{
			producerJMSSession.closeConnection();
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			consumerJMSSession.closeConnection();
		}
		catch(Exception e)
		{
			// do nothing
		}
	}

}
