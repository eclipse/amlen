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
package com.ibm.ism.mqbridge.test;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.PrintStream;

public class CreateMappingsFile {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		String configFileName = "C:\\mqcbridge127.cfg";
        FileOutputStream fos = null;
		try {
			fos = new FileOutputStream(configFileName);
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
        PrintStream log = new PrintStream(fos);
        log.println("# ISM");
        log.println("MQConnectivity.QueueManagerConnection.Name.QM1 = QM1");
        log.println("MQConnectivity.QueueManagerConnection.QueueManagerName.QM1 = dttest");
        log.println("MQConnectivity.QueueManagerConnection.ConnectionName.QM1 = 9.20.230.234(1415)");
        log.println("MQConnectivity.QueueManagerConnection.ChannelName.QM1 = SYSTEM.ISM.SVRCONN");
        log.println("MQConnectivity.Internal.QueueManagerConnection.Index.QM1 = 1");
        
        for (int count = 1; count < 200 ; count++) {
        	log.println("");
        	log.println(String.format("MQConnectivity.DestinationMappingRule.Name.Rule%s = Rule%s", count, count));
        	log.println(String.format("MQConnectivity.DestinationMappingRule.QueueManagerConnection.Rule%s = QM1", count));
        	log.println(String.format("MQConnectivity.DestinationMappingRule.RuleType.Rule%s = 2", count));
        	log.println(String.format("MQConnectivity.DestinationMappingRule.Source.Rule%s = Test/Test1/%s", count, count));
        	log.println(String.format("MQConnectivity.DestinationMappingRule.Destination.Rule%s = Test/Test1/%s", count, count));   
        	log.println(String.format("MQConnectivity.DestinationMappingRule.Enabled.Rule%s = True", count));
        	log.println(String.format("MQConnectivity.Internal.DestinationMappingRule.Index.Rule%s = %s", count, count));
        	if (count % 10 ==0) {
        	System.out.println("count is " + count);
        	}
        }
        log.flush();
        log.close();
	}

}
