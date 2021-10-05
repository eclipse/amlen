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
 * This class kicks off a test to create a number of mqtt connections, send a message then disconnect.
 * This is repeated until the test is stopped.
 * 
 * This test will use the default mqtt port of 16102 which is hardcoded into the test.
 * 
 * The prefix is used for the first part of the client ID and also for the topic name
 * 
 * TODO make the port number a configurable value OR add more code into the test to switch on
 * the default port if it is disabled (which it is be default on a new machine).
 *
 */
public class GeneralConnections {
	
	
	private static String ipaddress = null;
	private static long timeToRun = -1;
	private static String prefix = null;	
	private static int number = 1;
	private static int qos = 0;
	private static boolean trace = false;
	
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		
			try{
			
			if(args.length != 6)
			{
				System.out.println("Please pass in the required parameters: <messaging ip address> <timeToRun> <clientID prefix> <number of connections> <qos> <traceTest> \n");
				System.out.println("ExampleA: 1.2.3.4,5.6.7.8 60000000 MyClient 400 0 true \n");
				System.out.println("ExampleB: 1.2.3.4 60000000 MyClient 400 0 true \n");
				System.exit(0);
			}			
			else
			{
				ipaddress = args[0];
				timeToRun = new Long(args[1]);
				prefix = args[2];
				number = new Integer(args[3]);
				qos = new Integer(args[4]);
				trace = new Boolean(args[5]);
						
			}
			
			String uriA = null;
			String uriB = null;
			
			if(ipaddress.contains(",") || ipaddress.contains("+"))
			{	
				/*
				 * split either on "," "|" or "+"
				 */
				String[] temp = ipaddress.trim().split(",|\\||\\+", -1);
				uriA = "tcp://" + temp[0]  + ":16102";
				uriB = "tcp://" + temp[1]  + ":16102";
				System.out.println("Found 2 ip server addresses\n");
				System.out.println("uriA:"+uriA);
				System.out.println("uriB:"+uriB);
				
			}
			else
			{
				uriA = "tcp://"+ipaddress+ ":16102";
				System.out.println("Found 1 ip server address\n");
				System.out.println("uriA:"+uriA);
			}
		
			System.out.println("GeneralConnections Args: server(s) ipaddress: "+args[0]+ " time2run:"+args[1]+" prefix:"+args[2]+" numofconns:"+args[3]+" qos:"+args[4]+" trace:"+args[5]+"\n");
			System.out.println("GeneralConnections started\n");

			CreateMultipleConnections multipleConnections = new CreateMultipleConnections(uriA, uriB, prefix, false, qos, prefix, number, trace);
			new Thread(multipleConnections).start();
			
	
			long endTest = System.currentTimeMillis() + timeToRun;
			
			do
			{
				Thread.sleep(1200000);
				
			}while(System.currentTimeMillis() < endTest);
			
			multipleConnections.keepRunning = false;
			
			
			Thread.sleep(1200000);
			
			System.out.println("GeneralConnections completed the test");

	}
	catch(Exception e)
	{
		e.printStackTrace();
	}
	}

}
