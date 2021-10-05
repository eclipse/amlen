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
 * This class kicks off a test to delete disconnected MqttClients
 * 
 * This test will use the default mqtt port of 16102 which is hardcoded into the test.
 * 
 * TODO make the port number a configurable value OR add more code into the test to switch on
 * the default port if it is disabled (which it is be default on a new machine).
 *
 */
public class GeneralDeleteMqttClients {

	private static String cliaddressA = null;
	private static String cliaddressB = null;
	private static boolean trace = false;
	private static long timeToRun = -1;
	
	
	public static void main(String[] args) {
		
		try{
			
			if(args.length != 4)
			{
				System.out.println("Please pass in the required parameters: <mgt0 ip-applianceA> <mgt0 ip-applianceB OR null if standalone> <traceTest> <timeToRun>\n");
				System.out.println("ExampleA: 1.2.3.4 5.6.7.8 true 60000000 \n");
				System.out.println("ExampleB: 1.2.3.4 null false 36000000 \n");
				System.exit(0);
			}			
			else
			{
				cliaddressA = args[0];
				cliaddressB = args[1];
				if(cliaddressB.equals("null"))
				{
					cliaddressB = null;
				}
				trace = new Boolean(args[2]);
				timeToRun = new Long(args[3]);
					
			}
			
			System.out.println("GeneralDeleteMqttClients Args: server1 mgt0: "+args[0]+ " server2 mgt0:"+args[1]+"trace:"+args[2]+" time2run:"+args[3]+"\n");
			System.out.println("GeneralDeleteMqttClients started\n");

			
			DeleteDisconnectedClients deleteMqttClients = null;
			if(cliaddressB == null)
			{
				deleteMqttClients = new DeleteDisconnectedClients(cliaddressA, trace);
				new Thread(deleteMqttClients).start();
			}
			else
			{
				deleteMqttClients = new DeleteDisconnectedClients(cliaddressA, cliaddressB, trace);
				new Thread(deleteMqttClients).start();
			}
			long endTest = System.currentTimeMillis() + timeToRun;
			
			do
			{
				Thread.sleep(1200000);
				
			}while(System.currentTimeMillis() < endTest);
			
			deleteMqttClients.keepRunning = false;
			Thread.sleep(1200000);
			System.out.println("GeneralDeleteMqttClients test has finished");

	}
	catch(Exception e)
	{
		e.printStackTrace();
	}

	}

}
