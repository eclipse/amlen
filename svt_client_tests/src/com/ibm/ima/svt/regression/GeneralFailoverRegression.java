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
package com.ibm.ima.svt.regression;

import com.ibm.ima.svt.store.FillStore;

/**
 * This class will send messages to two topics. One topic will receive small messages
 * while the other receives large messages.
 * 
 *
 */
public class GeneralFailoverRegression {
	
	private static String ipaddress = null;
	private static String cliaddressA = null;
	private static String cliaddressB = null;
	private static long timeBetweenFailovers = 0;
	private static long resynchTime = 0;
	private static boolean forcestop = false;
	private static String prefix = null;
	private static boolean mixedForcedStop = false;
	private static boolean nofail = false;
	private static boolean tearDown = false;
	private static long timeToRun = -1;

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		

		
			try{
			
			if(args.length != 9)
			{
				System.out.println("Please pass in the required parameters: <messaging ipaddress> <mgt0 ip-applianceA> <mgt0 ip-applianceB OR null if standalone> <time between failovers> <time before resync or 0 if standalone> <stop method> <trace file prefix> <teardown> <timeTorun in milliseconds> \n");
				System.out.println("ExampleA uses forced stop HA: 9.8.7.6,5.4.3.2 1.2.3.4 5.6.7.8 1200000 360000 true MyTrace false 10000000 \n");
				System.out.println("ExampleB uses normal stop SA: 9.8.7.6 1.2.3.4 null 1200000 360000 false MyTrace false 10000000 \n");
				System.out.println("ExampleC uses mixed stop SA: 9.8.7.6 1.2.3.4 null 1200000 360000 mixed MyTrace false 10000000 \n");
				System.out.println("ExampleC does not stop SA: 9.8.7.6 1.2.3.4 null 1200000 360000 none MyTrace false 10000000 \n");
				System.exit(0);
			}			
			else
			{
				ipaddress = args[0];
				cliaddressA = args[1];
				cliaddressB = args[2];
				if(cliaddressB.equals("null"))
				{
					cliaddressB = null;
				}
				timeBetweenFailovers = new Long(args[3]);
				resynchTime = new Long(args[4]);
				String temp = args[5];
				if(temp.equalsIgnoreCase("mixed"))
				{
					mixedForcedStop = true;
				}
				else if(temp.equalsIgnoreCase("none"))
				{
					nofail = true;
				}
				else
				{
					forcestop = new Boolean(temp);
				}
				prefix = args[6];
				tearDown = new Boolean(args[7]);
				timeToRun = new Long(args[8]);
				
			}
			
			System.out.println("GeneralFailoverRegression Args: server(s) ipaddress: "+args[0]+" sever1_mgt0:"+args[1]+" server2_mgt1"+args[2]+" timebetweenfailovers:"+args[3]+" resynctime:"+args[4]+" stoptype:"+args[5]+" fileprefix:"+args[6]+" teardown:"+args[7]+" time2run:"+args[8]+"\n");
			System.out.println("GeneralFailoverRegression started\n");

			FillStore fillStore = new FillStore(100000, ipaddress, cliaddressA, cliaddressB, 16444);
			fillStore.setClientIds("ABCDE_B", "12345_B");
			fillStore.setTimeout(216000000);
			fillStore.setTimeBetweenFailovers(timeBetweenFailovers);
			fillStore.setResynchTime(resynchTime);
			fillStore.setFreeMemoryLimit(25);
			fillStore.setTearDown(tearDown);
			if(!nofail)
			{
				if(mixedForcedStop)
				{
					fillStore.setMixedForcedStop();
				}
				else
				{
					fillStore.setForceStop(forcestop);
				}
			}
			else
			{
				fillStore.setNoFail(true);
			}
			fillStore.setPrefix(prefix);
			fillStore.setup();
			new Thread(fillStore).start();
					
			long endTest = System.currentTimeMillis() + timeToRun;
			
			do
			{
				Thread.sleep(1200000);
				
			}while(System.currentTimeMillis() < endTest);
			
			System.out.println("GeneralFailoverRegression completed");
			fillStore.keepRunning = false;
			
			Thread.sleep(1200000);

	}
	catch(Exception e)
	{
		e.printStackTrace();
	}
	}
	
}
