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
package com.ibm.ima.mqcon.msgconversion;

import com.ibm.ima.mqcon.basetest.MQConnectivityTest;
import com.ibm.ima.mqcon.utils.JmsSession;
import com.ibm.ima.mqcon.utils.MqttConnection;

public abstract class MqttToJms extends MQConnectivityTest{
	
	protected static String imahostname = null;
	protected static int imaport = 0;
	protected static String mqhostname = null;
	protected static int mqport = 0;
	protected static String imaDestinationName = null;
	protected static String mqDestinationName = null;
	protected static int timeout = 1;
	protected static int RC = ReturnCodes.INCOMPLETE_TEST;
	protected static String mqqueuemanager = null;
	protected JmsSession consumerJMSSession = new JmsSession();
	protected MqttConnection mqttConnection = null;
	protected boolean completed = false;

	
	public void closeConnections()
	{
		try
		{
			consumerJMSSession.closeConnection();
		}
		catch(Exception e)
		{
			// do nothing
		}
		try
		{
			mqttConnection.destroyClient();
		}
		catch(Exception e)
		{
			// do nothing
		}
	}
	

}
