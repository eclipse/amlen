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

/**
 * This class creates subscriptions on the appliance
 *  
 *
 */
public class GeneralSubscriptionClients {

	private static String ipaddress = null;
	private static String cliaddressA = null;
	private static String cliaddressB = null;
	private static boolean tearDown = false;
	private static int number = 1;
	private static boolean trace = false;
	
	public static void main(String[] args) {
		
		try{
			
			if(args.length != 6)
			{
				System.out.println("Please pass in the required parameters: <messaging ipaddress> <mgt0 ip-applianceA> <mgt0 ip-applianceB OR null if standalone> <teardown previous> <number to create> <traceTest> \n");
				System.out.println("ExampleA: 9.8.7.6,5.4.3.2 1.2.3.4 5.6.7.8 false 100000 false \n");
				System.out.println("ExampleB: 9.8.7.6 1.2.3.4 null false 100000 false \n");
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
				tearDown = new Boolean(args[3]);
				number = new Integer(args[4]);
				trace = new Boolean(args[5]);
					
			}
			
			System.out.println("GeneralSubscriptionClients Args: server(s) ipaddress: "+args[0]+" sever1_mgt0:"+args[1]+" server2_mgt1"+args[2]+" tear:"+args[3]+" numofsubs:"+args[4]+" trace:"+args[5]+"\n");
			System.out.println("GeneralSubscriptionClients started\n");

			if(cliaddressB == null)
			{
				CreateSubscriptions createSubscribers = new CreateSubscriptions("SVTSUB-", ipaddress, "16455", cliaddressA, number, tearDown, trace);
				new Thread(createSubscribers).start();
			}
			else
			{
				CreateSubscriptions createSubscribers = new CreateSubscriptions("SVTSUB-", ipaddress, "16455", cliaddressA, cliaddressB, number, tearDown, trace);
				new Thread(createSubscribers).start();
			}
		
					
			long endTest = System.currentTimeMillis() + 1000000000;
			
			do
			{
				Thread.sleep(1200000);
				
			}while(System.currentTimeMillis() < endTest);
			
			System.out.println("GeneralSubscriptionClients completed");
			
			
			Thread.sleep(1200000);

	}
	catch(Exception e)
	{
		e.printStackTrace();
	}

	}

}
