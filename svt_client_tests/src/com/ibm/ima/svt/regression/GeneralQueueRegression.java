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
 * This kicks of a test which sends messages to queue using well and badly behaved clients
 *
 */
public class GeneralQueueRegression {
	
	private static String ipaddress = null;
	private static String cliaddressA = null;
	private static String cliaddressB = null;
	private static boolean tearDown = false;
	private static boolean traceClients = false;
	private static long timeToRun = -1;
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		

		
			try{
			
			if(args.length != 6)
			{
				System.out.println("Please pass in the required parameters: <messaging ipaddress> <mgt0 ip-applianceA> <mgt0 ip-applianceB OR null if standalone> <tear down previous config> <trace clients> <timeTorun in milliseconds> \n");
				System.out.println("ExampleA: 9.8.7.6,5.4.3.2 1.2.3.4 5.6.7.8 true false 10000000 \n");
				System.out.println("ExampleB: 9.8.7.6 1.2.3.4 null false false 10000000 \n");
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
				traceClients = new Boolean(args[4]);
				timeToRun = new Long(args[5]);
				
			}
			System.out.println("GeneralQueueRegression Args: server(s) ipaddress: "+args[0]+" sever1_mgt0:"+args[1]+" server2_mgt1"+args[2]+" tear:"+args[3]+" trace:"+args[4]+" time2run:"+args[5]+"\n");
			System.out.println("GeneralQueueRegression started\n");
			
			SendMessagesToQueues generalLoadQ = null;
			if(cliaddressB == null)
			{
				generalLoadQ = new SendMessagesToQueues("SVTREG-", ipaddress, "16445", cliaddressA, tearDown, traceClients);
				new Thread(generalLoadQ).start();
			}
			else
			{
				generalLoadQ = new SendMessagesToQueues("SVTREG-", ipaddress, "16445", cliaddressA, cliaddressB, tearDown, traceClients);
				new Thread(generalLoadQ).start();
			}
		
					
			long endTest = System.currentTimeMillis() + timeToRun;
			
			do
			{
				Thread.sleep(1200000);
				
			}while(System.currentTimeMillis() < endTest);
			
			generalLoadQ.keepRunning = false;
			Thread.sleep(1200000);
			System.out.println("GeneralQueueRegression completed");
			
			
			

	}
	catch(Exception e)
	{
		e.printStackTrace();
	}
	}

}
