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
package com.ibm.ima.svt.utils;

import java.util.HashMap;
import java.util.Iterator;

import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.monitor.ImaMonitoring;
import com.ibm.ima.test.cli.monitor.SubscriptionStatResult;

public class ClearAllSubscriptions {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		
		ImaMonitoring monitor = null;
		ImaConfig config = null;
		try
		{
			if(args.length != 1)
			{
				System.out.println("Please pass in the ima ipaddress and port");
				System.exit(0);
			}
			String ipaddress = args[0];
			monitor = new ImaMonitoring(ipaddress, "admin", "admin" );
			monitor.connectToServer();
			config = new ImaConfig(ipaddress, "admin", "admin" );
			config.connectToServer();
			HashMap<String,SubscriptionStatResult> monitorResults = monitor.getSubStatisticsByTopic(null);
			
			Iterator<String> iter = monitorResults.keySet().iterator();
			while(iter.hasNext())
			{
				SubscriptionStatResult result = monitorResults.get(iter.next());
				String sub = result.getSubName();
				String client = result.getclientId();
				try
				{
					config.deleteSubscription(sub, client);
				}
				catch(Exception e)
				{
					e.printStackTrace();
				}
			}
			monitor.disconnectServer();
			config.disconnectServer();
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}

	}

}
