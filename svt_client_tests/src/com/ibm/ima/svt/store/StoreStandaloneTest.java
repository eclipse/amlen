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

import com.ibm.ima.svt.regression.SendMessagesToQueues;
import com.ibm.ima.svt.regression.SendMessagesToTopics;


public class StoreStandaloneTest {
	
	private static String ipaddress = null;
	private static String cliaddressA = null;
	private static long timeBetweenFailovers = 0;
	private static boolean teardown = true;
	private static boolean traceClients = false;
	private static String release=null;


	/**
	 * @param args
	 */
	public static void main(String[] args) {
		try{
			
			if(args.length != 6)
			{
				System.out.println("Please pass in the required parameters");
				System.exit(0);
			}			
			else
			{
				ipaddress = args[0];
				cliaddressA = args[1];
				timeBetweenFailovers = new Long(args[2]);	
				teardown = new Boolean(args[3]);
				traceClients = new Boolean(args[4]);
				release = args[5];

				
			}
				
			SendMessagesToQueues generalLoadQ = new SendMessagesToQueues("SVTREG-", ipaddress, "16445", cliaddressA, null, teardown, traceClients);
			new Thread(generalLoadQ).start();
			SendMessagesToTopics generalLoadT = new SendMessagesToTopics("SVTREG-", ipaddress, "16446", cliaddressA, null, traceClients);
			new Thread(generalLoadT).start();
			FillStore fillStore = new FillStore(10000, ipaddress, cliaddressA, null, 16444);
			fillStore.setClientIds("ABCDE_B", "12345_B");
			fillStore.setTimeout(216000000);
			fillStore.setTimeBetweenFailovers(timeBetweenFailovers);
			fillStore.setFreeMemoryLimit(35);
			fillStore.setPrefix("SA-Workload");
			fillStore.setup();
			new Thread(fillStore).start();
			
			
			//long endTest = System.currentTimeMillis() + 57600000;
			
			long endTest = System.currentTimeMillis() + 1000000000;
			
			do
			{
				Thread.sleep(1200000);
				
			}while(System.currentTimeMillis() < endTest);
			
			System.out.println("SA Store completed");
			fillStore.keepRunning = false;
			
			Thread.sleep(1200000);

	}
	catch(Exception e)
	{
		e.printStackTrace();
	}
	}
	
	


}
