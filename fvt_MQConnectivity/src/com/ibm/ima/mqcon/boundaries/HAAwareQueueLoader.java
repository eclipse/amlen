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
package com.ibm.ima.mqcon.boundaries;

import java.io.IOException;
import javax.jms.JMSException;

import com.ibm.ima.mqcon.rules.ImaQueueToMqDestination;
import com.ibm.ima.mqcon.utils.StaticUtilities;

/**
 * This test sends a number of messages to a named IMA queue 
 * and retrieves them from a named MQ topic
 */
public class HAAwareQueueLoader extends ImaQueueToMqDestination {
	int megaBytes = 8;
	String ipMachine1 = "9.3.177.39";
	String ipMachine2 = "9.3.177.40";
	public HAAwareQueueLoader() {
		super();
	}
	
	public static void main(String[] args) {
		boolean success = HAAwareQueueLoader.run(args);	
		if (success==false) {
			StaticUtilities.printFail();
		} else {
			StaticUtilities.printSuccess();
		}	
	}
		
		public static boolean run(String[] args) {	
		HAAwareQueueLoader test = null;		
		try {
			test = new HAAwareQueueLoader();
			test.setNumberOfMessages(5200);
			test.setNumberOfPublishers(1);
			test.imaQueueName = "Queue3";
			if (args.length > 0) { //TODO ensure args length is correct
				test.parseCommandLineArguments(args);
			}			
			test.execute();
		} catch (IOException e) {
			System.out.println("IO exception - TEST FAILED");
			e.printStackTrace();
			test.setSuccess(false);
		} catch (JMSException e) {
			System.out.println("JMS exception - TEST FAILED");
			e.printStackTrace();
			test.setSuccess(false);
		} catch (Exception e) {
			System.out.println("Exception - TEST FAILED");
			e.printStackTrace();
			test.setSuccess(false);
		} finally {
		}
		return test.isSuccess();
	}
	
	private void execute() throws  IOException, JMSException {
			try {
				sendLargeHaMessages(imaQueueName, testName, ipMachine1 , ipMachine2, imaServerPort,1,megaBytes);
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
	}
	
	public void parseCommandLineArguments(String [] inputArgs) throws NumberFormatException {
		super.parseCommandLineArguments(inputArgs);
		for (int x=0;x < inputArgs.length; x++) {
			if (inputArgs[x].equalsIgnoreCase("-ipMachine1")) {
				x++;
				ipMachine1 = inputArgs[x];
			}
			if (inputArgs[x].equalsIgnoreCase("-ipMachine2")) {
				x++;
				ipMachine2 = inputArgs[x];
			}
			if (inputArgs[x].equalsIgnoreCase("-megaBytes")) {
				x++;
				megaBytes = Integer.valueOf(inputArgs[x]);
			}
			
		}
	}
}

