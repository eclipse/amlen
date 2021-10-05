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

import com.ibm.ima.test.cli.imaserver.Version;

/**
 * This kicks of a test which sends messages to topics using well and badly behaved clients
 *
 */
public class GeneralTopicRegression {
	private static String ipaddress = null;
	private static String cliaddressA = null;
	private static String cliaddressB = null;
	private static boolean traceClients = false;
	private static long timeToRun = -1;


	/**
	 * @param args
	 */
	public static void main(String[] args) {
		

		
			try{
			
			if(args.length != 5)
			{
				System.out.println("Please pass in the required parameters: <messaging ipaddress> <mgt0 ip-applianceA> <mgt0 ip-applianceB OR null if standalone> <trace clients> <timeTorun in milliseconds> \n");
				System.out.println("ExampleA: 9.8.7.6,5.4.3.2 1.2.3.4 5.6.7.8 false 10000000 \n");
				System.out.println("ExampleB: 9.8.7.6 1.2.3.4 null false 10000000 \n");
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
				traceClients = new Boolean(args[3]);
				timeToRun = new Long(args[4]);


				
			}
			
			System.out.println("GeneralTopicRegression Args: server(s) ipaddress: "+args[0]+" sever1_mgt0:"+args[1]+" server2_mgt1"+args[2]+" trace:"+args[3]+" time2run:"+args[4]+"\n");
			System.out.println("GeneralTopicRegression started\n");
			
			SendMessagesToTopics generalLoadT = null;
			//Lets setup the version object base on server 1 mgt0
			Version.setVersion(cliaddressA,"admin","admin");

			if(cliaddressB == null)
			{
				generalLoadT = new SendMessagesToTopics("SVTREG-", ipaddress, "16446", cliaddressA, traceClients);
				new Thread(generalLoadT).start();
			}
			else
			{
				generalLoadT = new SendMessagesToTopics("SVTREG-", ipaddress, "16446", cliaddressA, cliaddressB, traceClients);
				new Thread(generalLoadT).start();
			}
		
					
			long endTest = System.currentTimeMillis() + timeToRun;
			
			do
			{
				Thread.sleep(240000);
				
			}while(System.currentTimeMillis() < endTest);
			
			generalLoadT.keepRunning = false;
			Thread.sleep(1200000);
			System.out.println("GeneralTopicRegression completed");
			
			
			

	}
	catch(Exception e)
	{
		e.printStackTrace();
	}
	}

}
