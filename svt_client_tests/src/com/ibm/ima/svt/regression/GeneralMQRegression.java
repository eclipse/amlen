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
 * This kicks of a test which sends messages across MQConnectivity to create
 * general messaging on the system
 *
 */
public class GeneralMQRegression {
	
	private static String prefix = null;
	private static String ipaddress = null;
	private static String cliaddressA = null;
	private static String cliaddressB = null;
	private static String mqipaddress = null;
	private static int mqjmsport = 1;
	private static int mqmqttport = 1;
	private static String mqlocation = null;
	private static String mqusername = null;
	private static String mquser = null;
	private static String mqpassword = null;
	private static boolean teardown = false;
	private static boolean traceClients = false;
	private static long timeToRun = -1;
	

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		
		
		try{
			
			if(args.length != 14)
			{
				System.out.println("Please pass in the required parameters: <messaging ipaddress> <mgt0 ip-applianceA> <mgt0 ip-applianceB OR null if standalone> <mqipaddress> \n");
				System.out.println("<mq tcp port> <mq mqtt port> <mq installation> <mquser> <non mq user> <mquser password> <qmgr prefix> <teardown previous config> <trace clients> <time to run in milliseconds> \n");
				System.out.println("ExampleA: 9.8.7.6,5.4.3.2 1.2.3.4 5.6.7.8 5.5.5.5 1214 1883 /opt/mqm/bin mqm myuser mypassword QM false true 10000000 \n");
				System.out.println("ExampleA: 9.8.7.6 1.2.3.4 null 5.5.5.5 1214 1883 /opt/mqm/bin mqm myuser mypassword QM false true 10000000 \n");
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
				mqipaddress = args[3];
				mqjmsport = new Integer(args[4]);
				mqmqttport = new Integer(args[5]);
				mqlocation = args[6];
				mqusername = args[7];
				mquser = args[8];
				mqpassword = args[9];
				prefix = args[10];
				teardown = new Boolean(args[11]);
				traceClients = new Boolean(args[12]);
				timeToRun = new Long(args[13]);
				
		
				
			}
			System.out.println("Prepping worker thread");
			SendMessagesToMq generalMQ =  null;
			if(cliaddressB == null)
			{
				System.out.println("Starting worker thread without HA");
				generalMQ = new SendMessagesToMq(prefix, ipaddress, "16450", cliaddressA, mqipaddress, mqjmsport, mqmqttport, mqlocation, mqusername, mquser, mqpassword, teardown, traceClients);
				new Thread(generalMQ).start();
			}
			else
			{
				System.out.println("Starting worker thread with HA");
				generalMQ = new SendMessagesToMq(prefix, ipaddress, "16450", cliaddressA, cliaddressB, mqipaddress, mqjmsport, mqmqttport, mqlocation, mqusername, mquser, mqpassword, teardown, traceClients);
				
				new Thread(generalMQ).start();
			}


			System.out.println("Finished config ");
			long endTest = System.currentTimeMillis() + timeToRun;
			
			do
			{
				Thread.sleep(1200);
				
			}while(System.currentTimeMillis() < endTest);
			System.out.println("Finished running tests, telling tests to stop");
			generalMQ.keepRunning = false;
			Thread.sleep(360000);
			System.out.println("Finished waiting for tests to stop");
			System.out.println("General MQ Regression completed");
			
			
			

	}
	catch(Exception e)
	{
		e.printStackTrace();
	}

	}

}
