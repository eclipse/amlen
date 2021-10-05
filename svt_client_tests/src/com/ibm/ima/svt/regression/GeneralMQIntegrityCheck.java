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

public class GeneralMQIntegrityCheck  {
	
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
				System.out.println("Please pass in the required parameters");
				System.exit(0);
			}			
			else
			{
				ipaddress = args[0].replace("+", ",");
				cliaddressA = args[1];
				cliaddressB = args[2];
				if(cliaddressB.equals("null"))
				{
					cliaddressB = null;
				}
				traceClients = new Boolean(args[3]);
				timeToRun = new Long(args[4]);
					
			}
			SendAndCheckMQMessages generalMQInt = null;
			if(cliaddressB == null)
			{
				generalMQInt = new SendAndCheckMQMessages("SVTMQINT-", ipaddress, "16449", cliaddressA, traceClients);
				new Thread(generalMQInt).start();
			}
			else
			{
				generalMQInt = new SendAndCheckMQMessages("SVTMQINT-", ipaddress, "16449", cliaddressA, cliaddressB, traceClients);
				new Thread(generalMQInt).start();
			}
		
					
			long endTest = System.currentTimeMillis() + timeToRun;
			
			do
			{
				Thread.sleep(1200000);
				
			}while(System.currentTimeMillis() < endTest);
			
			generalMQInt.keepRunning = false;
			Thread.sleep(1200000);
			System.out.println("Regression completed");
			
			
			

	}
	catch(Exception e)
	{
		e.printStackTrace();
	}
	}
}
