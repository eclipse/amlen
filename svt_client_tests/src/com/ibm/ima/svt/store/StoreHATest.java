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



public class StoreHATest {
	
	private static String ipaddress = null;
	private static String cliaddressA = null;
	private static String cliaddressB = null;
	private static long timeBetweenFailovers = 0;
	private static long resynchTime = 0;


	/**
	 * @param args
	 */
	public static void main(String[] args) {
		try{
			
			if(args.length != 5)
			{
				System.out.println("Please pass in the required parameters");
				System.exit(0);
			}			
			else
			{
				ipaddress = args[0];
				cliaddressA = args[1];
				cliaddressB = args[2];
				timeBetweenFailovers = new Long(args[3]);
				resynchTime = new Long(args[4]);
				
				
			}
			
			FillStore fillStore = new FillStore(100000, ipaddress, cliaddressA, cliaddressB, 16444);
			fillStore.setClientIds("ABCDE_B", "12345_B");
			fillStore.setFreeMemoryLimit(35);
			fillStore.setPrefix("HA-Workload");
			fillStore.setTimeout(216000000);
			fillStore.setTimeBetweenFailovers(timeBetweenFailovers);
			fillStore.setResynchTime(resynchTime);
			fillStore.setup();
			
			new Thread(fillStore).start();	
			
			
			
			//long endTest = System.currentTimeMillis() + 57600000;
			
			long endTest = System.currentTimeMillis() + 1000000000;
			
			do
			{
				Thread.sleep(240000);
				
			}while(System.currentTimeMillis() < endTest);
			
			System.out.println("HA Store completed");
			fillStore.keepRunning = false;
		//	fillStore2.keepRunning = false;
			
			Thread.sleep(1200000);

	}
	catch(Exception e)
	{
		e.printStackTrace();
	}
	}
	
	


}
