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
 * This class kicks of a test to send in empty retained MQTT messages
 * to topics in the format <prefix>/i where i is a number that increments by 1 each time.
 * 
 * An empty MQTT message which is marked as retained has the effect or 'deleting' any
 * retained message on that particular topic.
 * 
 * Therefore if you have prefix /vehicles/car/ and you have number to start at as 0 and
 * number to end at is 1000 messages will be sent to:-
 * 
 * /vehicles/car/0
 * /vehicles/car/1
 * /vehicles/car/2
 * /vehicles/car/3
 * /vehicles/car/4
 * .....
 * /vehicles/car/998
 * /vehicles/car/1000
 * 
 * This test will use the default mqtt port of 16102 which is hardcoded into the test.
 * 
 * TODO make the port number a configurable value OR add more code into the test to switch on
 * the default port if it is disabled (which it is be default on a new machine).
 *
 */
public class GeneralDeleteRetainedMessages {

	private static String ipaddress = null;
	private static int min = 1;
	private static int max = 1;
	private static boolean trace = false;
	private static String prefix = null;
	
	
	public static void main(String[] args) {
		
		try{
			
			if(args.length != 5)
			{
				System.out.println("Please pass in the required parameters: <messaging ip address> <topic prefix> <number to start at> <number to end at> <trace test>\n");
				System.out.println("ExampleA: 9.8.7.6,5.4.3.2 /vehicle/car/ 0 1000 true \n");
				System.out.println("ExampleB: 9.8.7.6 /vehicle/car/ 500 500000 false \n");
				System.exit(0);
			}			
			else
			{
				ipaddress = args[0];
				prefix = args[1];
				min = new Integer(args[2]);
				max = new Integer(args[3]);
				trace = new Boolean(args[4]);
					
			}
			
			System.out.println("GeneralDeleteRetainedMessages Args: server1 mgt0 ipaddress: "+args[0]+" prefix:"+args[1]+" min:"+args[2]+" max:"+args[3]+" trace:"+args[4]+"\n");
			System.out.println("GeneralDeleteRetainedMessages started\n");

			
			DeleteRetainedMessages deleteMessages = new DeleteRetainedMessages(ipaddress, "16102", trace, prefix, min, max);
			Thread t = new Thread(deleteMessages);
			t.start();
			
			do
			{
				Thread.sleep(1200000);
				
			}while(t.isAlive());
			
			
			System.out.println("GeneralDeleteRetainedMessages completed");
			
			
			Thread.sleep(1200000);

	}
	catch(Exception e)
	{
		e.printStackTrace();
	}

	}

}
